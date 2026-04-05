/* ================================================================
 * wm.c – NeuroSpark Desktop Window Manager
 *
 * Provides a desktop environment with:
 *   • Dark teal gradient desktop background
 *   • Bottom taskbar / dock with app icons + clock
 *   • Draggable, minimisable, maximisable, closable windows
 *   • Z-order management and focus tracking
 * ================================================================ */
#include "wm.h"
#include "launcher.h"

/* External graphics primitives (graphics.c) */
extern void put_pixel(int x, int y, uint32_t color);
extern void clear_region(int x0, int y0, int x1, int y1, uint32_t color);
extern void draw_hline(int y, int x0, int x1, uint32_t color);
extern void draw_vline(int x, int y0, int y1, uint32_t color);
extern void gprint(char *str, uint32_t color);
extern void gprint_dec(int val, uint32_t color);
extern void draw_char(char c, int x, int y, uint32_t color);
extern int cursor_x;
extern int cursor_y;

extern uint32_t tick;
extern int potentials[16];
extern volatile char input_buffer[32];
extern volatile int buffer_idx;
extern char cmd_output[256];
extern int cmd_output_valid;
extern volatile int mouse_x;
extern volatile int mouse_y;
extern volatile int mouse_buttons;
extern uint32_t backbuffer[];

extern int pmm_get_free_pages(void);
extern int storage_snapshot_used_count(void);
extern int storage_snapshot_capacity(void);

typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

typedef struct {
    int voltage;
    int spike_count;
    int id;
    int synaptic_weight;
    int refractory_timer;
    int dynamic_threshold;
} WmNeuron;

typedef struct {
    WmNeuron neurons[5];
    uint8_t current_phase;
    int pixel_recent_spikes;
} WmNeuralPixel;

typedef struct {
    int task_id;
    int priority;
    int target_pixel;
    int base_integration;
    int fire_threshold;
    const char *task_name;
    int saved_voltages[5];
    int saved_weights[5];
    int saved_thresholds[5];
    int last_spike_count;
    int spikes_per_second;
    int state;
} WmTaskControlBlock;

extern WmTaskControlBlock task_list[2];
extern WmNeuralPixel os_memory_map[2];

/* Runtime timezone offset from UTC in minutes; configurable via shell. */
static int wm_tz_offset_minutes = 0;

#define WM_TZ_CMOS_SIG_A_REG 0x38
#define WM_TZ_CMOS_SIG_B_REG 0x39
#define WM_TZ_CMOS_LO_REG    0x3A
#define WM_TZ_CMOS_HI_REG    0x3B
#define WM_TZ_CMOS_SIG_A     0x54  /* 'T' */
#define WM_TZ_CMOS_SIG_B     0x5A  /* 'Z' */

/* ------------------------------------------------------------ */
/*  Window state                                                 */
/* ------------------------------------------------------------ */
WmWindow wm_windows[WM_MAX_WINDOWS];
int wm_window_count = 0;
int wm_focused = -1;

static int z_order[WM_MAX_WINDOWS];
static int z_count = 0;
static int power_menu_open = 0;


/* Taskbar icon labels */
static const char *icon_labels[WM_ICON_SLOTS] = {
    "OS", "Launcher", "Neural Viz", "Console", "Profiler", "Settings"
};

/* ------------------------------------------------------------ */
/*  Helpers                                                      */
/* ------------------------------------------------------------ */
static void wm_strcpy(char *dst, const char *src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static int pt_in_rect(int px, int py, int rx, int ry, int rw, int rh) {
    return px >= rx && px < rx + rw && py >= ry && py < ry + rh;
}

static void draw_rect(int x, int y, int w, int h, uint32_t c) {
    draw_hline(y, x, x + w, c);
    draw_hline(y + h - 1, x, x + w, c);
    draw_vline(x, y, y + h - 1, c);
    draw_vline(x + w - 1, y, y + h - 1, c);
}

static void draw_filled_rect(int x, int y, int w, int h, uint32_t c) {
    clear_region(x, y, x + w, y + h, c);
}

static void draw_line_segment(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = -((y1 > y0) ? (y1 - y0) : (y0 - y1));
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        {
            int e2 = err << 1;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    }
}

static uint32_t blend_50(uint32_t a, uint32_t b) {
    uint32_t ar = (a >> 16) & 0xFF;
    uint32_t ag = (a >> 8) & 0xFF;
    uint32_t ab = a & 0xFF;
    uint32_t br = (b >> 16) & 0xFF;
    uint32_t bg = (b >> 8) & 0xFF;
    uint32_t bb = b & 0xFF;
    return (((ar + br) >> 1) << 16) | (((ag + bg) >> 1) << 8) | ((ab + bb) >> 1);
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static uint8_t cmos_read(uint8_t reg) {
    outb(0x70, (uint8_t)(reg & 0x7F));
    return inb(0x71);
}

static void cmos_write(uint8_t reg, uint8_t value) {
    outb(0x70, (uint8_t)(reg & 0x7F));
    outb(0x71, value);
}

static void wm_load_timezone_from_cmos(void) {
    uint8_t sig_a = cmos_read(WM_TZ_CMOS_SIG_A_REG);
    uint8_t sig_b = cmos_read(WM_TZ_CMOS_SIG_B_REG);
    int minutes;

    if (sig_a != WM_TZ_CMOS_SIG_A || sig_b != WM_TZ_CMOS_SIG_B) {
        return;
    }

    minutes = (int)(short)((unsigned short)cmos_read(WM_TZ_CMOS_LO_REG) |
                           ((unsigned short)cmos_read(WM_TZ_CMOS_HI_REG) << 8));
    if (minutes < -720 || minutes > 840) {
        return;
    }
    wm_tz_offset_minutes = minutes;
}

static void wm_save_timezone_to_cmos(void) {
    unsigned short raw = (unsigned short)(short)wm_tz_offset_minutes;
    cmos_write(WM_TZ_CMOS_SIG_A_REG, WM_TZ_CMOS_SIG_A);
    cmos_write(WM_TZ_CMOS_SIG_B_REG, WM_TZ_CMOS_SIG_B);
    cmos_write(WM_TZ_CMOS_LO_REG, (uint8_t)(raw & 0xFF));
    cmos_write(WM_TZ_CMOS_HI_REG, (uint8_t)((raw >> 8) & 0xFF));
}

static int bcd_to_bin(int v) {
    return ((v >> 4) * 10) + (v & 0x0F);
}

static void system_reboot(void) {
    __asm__ volatile("cli");

    /* 8042 keyboard controller reset (common legacy path). */
    outb(0x64, 0xFE);

    /* PCI reset control fallback (QEMU/real chipsets). */
    outb(0xCF9, 0x02);
    outb(0xCF9, 0x06);

    /* Last resort: force triple fault reset. */
    {
        struct {
            uint16_t limit;
            uint32_t base;
        } __attribute__((packed)) null_idt = {0, 0};
        __asm__ volatile("lidt %0" : : "m"(null_idt));
        __asm__ volatile("int3");
    }

    for (;;) {
        __asm__ volatile("hlt");
    }
}

static void system_shutdown(void) {
    __asm__ volatile("cli");
    /* QEMU/Bochs/ISA debug shutdown fallbacks. */
    outw(0x604, 0x2000);   /* QEMU ACPI poweroff */
    outw(0xB004, 0x2000);  /* Bochs/QEMU legacy */
    outw(0x4004, 0x3400);  /* VirtualBox/others */
    for (;;) {
        __asm__ volatile("hlt");
    }
}

static void read_rtc_hm(int *out_h, int *out_m) {
    int h = 0, h2 = 0;
    int m = 0, m2 = 0;
    int s = 0, s2 = 0;
    int reg_b = 0;
    int tries = 0;

    do {
        tries = 0;
        while ((cmos_read(0x0A) & 0x80) && tries < 100000) {
            tries++;
        }
        s = cmos_read(0x00);
        m = cmos_read(0x02);
        h = cmos_read(0x04);

        tries = 0;
        while ((cmos_read(0x0A) & 0x80) && tries < 100000) {
            tries++;
        }
        s2 = cmos_read(0x00);
        m2 = cmos_read(0x02);
        h2 = cmos_read(0x04);
    } while (s != s2 || m != m2 || h != h2);

    reg_b = cmos_read(0x0B);

    if ((reg_b & 0x04) == 0) {
        s = bcd_to_bin(s);
        m = bcd_to_bin(m);
        h = bcd_to_bin(h & 0x7F) | (h & 0x80);
    }

    if ((reg_b & 0x02) == 0) {
        int pm = h & 0x80;
        h &= 0x7F;
        if (pm && h < 12) h += 12;
        if (!pm && h == 12) h = 0;
    }

    if (h < 0 || h > 23) h = 0;
    if (m < 0 || m > 59) m = 0;

    /* Apply timezone offset and wrap to local wall-clock time. */
    {
        int total = (h * 60) + m + wm_tz_offset_minutes;
        while (total < 0) total += 24 * 60;
        while (total >= 24 * 60) total -= 24 * 60;
        h = total / 60;
        m = total % 60;
    }

    (void)s;
    *out_h = h;
    *out_m = m;
}

/* ACPI/APM battery backend is not present yet; return -1 when unavailable. */
static int read_battery_percent(void) {
    return -1;
}

int wm_set_timezone_offset_minutes(int minutes) {
    if (minutes < -720 || minutes > 840) {
        return 0;
    }
    wm_tz_offset_minutes = minutes;
    wm_save_timezone_to_cmos();
    return 1;
}

int wm_get_timezone_offset_minutes(void) {
    return wm_tz_offset_minutes;
}

static void draw_glass_rect(int x, int y, int w, int h, uint32_t tint) {
    int yy, xx;
    for (yy = y; yy < y + h; yy++) {
        if (yy < 0 || yy >= 600) continue;
        for (xx = x; xx < x + w; xx++) {
            if (xx < 0 || xx >= 800) continue;
            int idx = yy * 800 + xx;
            backbuffer[idx] = blend_50(backbuffer[idx], tint);
        }
    }

    draw_hline(y, x + 1, x + w - 1, 0x66AFC6);
    draw_vline(x, y + 1, y + h - 1, 0x66AFC6);
    draw_hline(y + h - 1, x + 1, x + w - 1, 0x08131A);
    draw_vline(x + w - 1, y + 1, y + h - 1, 0x08131A);
}

static void draw_content_launcher(int x, int y, int w, int h) {
    draw_filled_rect(x + 8, y + 8, w - 16, h - 16, 0x102232);
    draw_rect(x + 8, y + 8, w - 16, h - 16, 0x3A627B);
    cursor_x = x + 12;
    cursor_y = y + 12;
    gprint("Launcher", 0xAEDDFF);
    cursor_x = x + 12;
    cursor_y = y + 28;
    gprint("Neural Viz: live spike raster", 0x8FB8CC);
    cursor_x = x + 12;
    cursor_y = y + 42;
    gprint("Console: interactive shell input", 0x8FB8CC);
    cursor_x = x + 12;
    cursor_y = y + 56;
    gprint("Profiler: spike intensity bars", 0x8FB8CC);
    cursor_x = x + 12;
    cursor_y = y + 70;
    gprint("Settings: timezone and storage", 0x8FB8CC);
}

static void draw_content_neuroviz(int x, int y, int w, int h) {
    int px = x + 6;
    int py = y + 6;
    int pw = w - 12;
    int ph = h - 12;
    int hud_w = pw / 3;
    int graph_x = px + hud_w + 8;
    int graph_w = pw - hud_w - 14;
    int graph_y = py + 26;
    int graph_h = ph - 36;
    int i;
    int recent_spike = 0;
    int prev_x = 0;
    int prev_y = 0;

    if (pw < 180 || ph < 110) {
        draw_filled_rect(px, py, pw, ph, 0x071726);
        cursor_x = px + 8;
        cursor_y = py + 8;
        gprint("Neural Viz", 0xAEDDFF);
        return;
    }

    draw_filled_rect(px, py, pw, ph, 0x061425);
    draw_rect(px, py, pw, ph, 0x2A526D);
    draw_hline(py + 18, px + 1, px + pw - 1, 0x1F4962);

    cursor_x = px + 8;
    cursor_y = py + 4;
    gprint("NEURAL WAVEFORM HUD CORE", 0x84D8FF);

    /* Left HUD panel */
    draw_filled_rect(px + 6, py + 26, hud_w - 8, graph_h, 0x01233D);
    draw_rect(px + 6, py + 26, hud_w - 8, graph_h, 0x1E4F6A);
    cursor_x = px + 12;
    cursor_y = py + 30;
    gprint("SPIKE HUD", 0x88D8FF);
    cursor_x = px + 12;
    cursor_y = py + 46;
    gprint("P0 SPS:", 0xA8C8DC);
    gprint_dec(task_list[0].spikes_per_second, 0x6CFFA4);
    cursor_x = px + 12;
    cursor_y = py + 60;
    gprint("P1 SPS:", 0xA8C8DC);
    gprint_dec(task_list[1].spikes_per_second, 0x6CFFA4);

    for (i = 0; i < 16; i++) {
        if (potentials[i] >= 900) {
            recent_spike++;
        }
    }
    cursor_x = px + 12;
    cursor_y = py + 74;
    gprint("RECENT:", 0xA8C8DC);
    gprint_dec(recent_spike, 0xFFD766);
    gprint("/16", 0xA8C8DC);

    cursor_x = px + 12;
    cursor_y = py + 92;
    gprint("PH0:", 0xA8C8DC);
    gprint(os_memory_map[0].current_phase ? "RIGID" : "FLUID",
           os_memory_map[0].current_phase ? 0xFF8F7D : 0x74FFAE);
    cursor_x = px + 12;
    cursor_y = py + 106;
    gprint("PH1:", 0xA8C8DC);
    gprint(os_memory_map[1].current_phase ? "RIGID" : "FLUID",
           os_memory_map[1].current_phase ? 0xFF8F7D : 0x74FFAE);

    /* Main waveform panel */
    draw_filled_rect(graph_x, graph_y, graph_w, graph_h, 0x000A33);
    draw_rect(graph_x, graph_y, graph_w, graph_h, 0x204A63);
    for (i = 1; i < 4; i++) {
        int gy = graph_y + (i * graph_h) / 4;
        draw_hline(gy, graph_x + 1, graph_x + graph_w - 1, 0x102B46);
    }
    draw_hline(graph_y + graph_h - 10, graph_x + 1, graph_x + graph_w - 1, 0x2D5E7A);

    for (i = 0; i < 16; i++) {
        int x0 = graph_x + 10 + (i * (graph_w - 20)) / 15;
        int pot = potentials[i];
        int pulse = ((((int)tick >> 1) + (i * 11)) & 15);
        int hgt = 10 + (pot / 32) + (pulse * 5);
        int max_h = graph_h - 26;
        uint32_t c;
        int y0;

        if (hgt > max_h) {
            hgt = max_h;
        }
        y0 = graph_y + graph_h - 10 - hgt;
        c = (pot > 850) ? 0xFFDD55 : ((i & 1) ? 0x58E6FF : 0x59FFC1);

        draw_filled_rect(x0 - 2, y0, 5, hgt, c);
        draw_line_segment(x0, graph_y + graph_h - 10, x0, y0, c);

        if (i == 0) {
            prev_x = x0;
            prev_y = y0;
        } else {
            draw_line_segment(prev_x, prev_y, x0, y0, 0x7CC8FF);
            prev_x = x0;
            prev_y = y0;
        }

        if ((i & 3) == 0) {
            cursor_x = x0 - 4;
            cursor_y = graph_y + graph_h - 6;
            gprint_dec(i, 0x8FB8CC);
        }
    }
}

static void draw_console_line_clipped(int x, int y, int max_chars, const char *text, uint32_t color) {
    int i = 0;
    cursor_x = x;
    cursor_y = y;
    while (text[i] && i < max_chars) {
        char ch[2];
        ch[0] = text[i];
        ch[1] = '\0';
        gprint(ch, color);
        i++;
    }
}

static void draw_content_console(int x, int y, int w, int h) {
    int tx = x + 8;
    int ty = y + 8;
    int tw = w - 16;
    int th = h - 16;
    int chars = (tw - 14) / 8;
    int i;

    draw_filled_rect(tx, ty, tw, th, 0x071621);
    draw_rect(tx, ty, tw, th, 0x3A627B);

    cursor_x = tx + 6;
    cursor_y = ty + 6;
    gprint("Axiom Console", 0xAEDDFF);

    if (cmd_output_valid) {
        draw_console_line_clipped(tx + 6, ty + 24, chars, cmd_output, 0x68E8A3);
    }

    cursor_x = tx + 6;
    cursor_y = ty + th - 22;
    gprint("> ", 0x55FF88);
    for (i = 0; i < buffer_idx && i < 31 && i < chars - 3; i++) {
        char ch[2];
        ch[0] = (char)input_buffer[i];
        ch[1] = '\0';
        gprint(ch, 0xEAF2F8);
    }

    if ((tick / 20) % 2 == 0) {
        int cx = tx + 22 + (buffer_idx * 8);
        if (cx < tx + tw - 8) {
            draw_filled_rect(cx, ty + th - 22, 6, 10, 0x8FD3FF);
        }
    }
}

static void draw_content_profiler(int x, int y, int w, int h) {
    int i;
    int tx = x + 8;
    int ty = y + 8;
    int tw = w - 16;
    int th = h - 16;
    int peak = 0;
    int total = 0;
    int bars_x = tx + 10;
    int bars_h = th - 54;

    draw_filled_rect(tx, ty, tw, th, 0x101E2A);
    draw_rect(tx, ty, tw, th, 0x3A627B);
    draw_hline(ty + 18, tx + 1, tx + tw - 1, 0x294E67);

    cursor_x = x + 10;
    cursor_y = y + 10;
    gprint("Neural Spike Profiler", 0xAEDDFF);

    if (tw < 180 || th < 90) {
        return;
    }

    for (i = 0; i < 16; i++) {
        if (potentials[i] > peak) {
            peak = potentials[i];
        }
        total += potentials[i];
    }

    cursor_x = tx + 10;
    cursor_y = ty + 24;
    gprint("Peak:", 0x9BC7DF);
    gprint_dec(peak, 0xFFFFFF);
    gprint(" Avg:", 0x9BC7DF);
    gprint_dec(total / 16, 0xFFFFFF);

    draw_hline(ty + th - 10, tx + 10, tx + tw - 10, 0x2A4255);
    for (i = 0; i < 6; i++) {
        int src = (i * 2) % 16;
        int base = potentials[src] + potentials[src + 1];
        int pulse = ((((int)tick / 2) + i * 17) & 31) * 12;
        int v = base + pulse;
        int bar_h = (v > 2000 ? bars_h : (v * bars_h) / 2000);
        int bx = bars_x + 8 + i * ((tw - 26) / 6);
        if (bar_h < 4) bar_h = 4;
        draw_filled_rect(bx, ty + th - 10 - bar_h, 20, bar_h,
                         (i & 1) ? 0x4AA7DF : 0x56D8B2);
        cursor_x = bx + 4;
        cursor_y = ty + th - 8;
        gprint_dec(src / 2, 0x8FB8CC);

        cursor_x = bx + 1;
        cursor_y = ty + th - 20 - bar_h;
        gprint_dec(v / 20, 0xA8CDE2);
    }

    cursor_x = tx + 10;
    cursor_y = ty + th - 14;
    gprint("Group spike load", 0x8FB8CC);
}

static void draw_content_settings(int x, int y, int w, int h) {
    int tz = wm_get_timezone_offset_minutes();
    int abs_tz = tz < 0 ? -tz : tz;
    int hh = abs_tz / 60;
    int mm = abs_tz % 60;
    int used = storage_snapshot_used_count();
    int cap = storage_snapshot_capacity();
    int free_pages = pmm_get_free_pages();

    draw_filled_rect(x + 8, y + 8, w - 16, h - 16, 0x111E2A);
    draw_rect(x + 8, y + 8, w - 16, h - 16, 0x3A627B);

    cursor_x = x + 10;
    cursor_y = y + 10;
    gprint("Settings", 0xAEDDFF);

    cursor_x = x + 10;
    cursor_y = y + 28;
    gprint("TZ: ", 0x9EC4D8);
    gprint(tz < 0 ? "-" : "+", 0xEAF2F8);
    if (hh < 10) gprint("0", 0xEAF2F8);
    gprint_dec(hh, 0xEAF2F8);
    gprint(":", 0xEAF2F8);
    if (mm < 10) gprint("0", 0xEAF2F8);
    gprint_dec(mm, 0xEAF2F8);

    cursor_x = x + 10;
    cursor_y = y + 44;
    gprint("Snapshots: ", 0x9EC4D8);
    gprint_dec(used, 0xEAF2F8);
    gprint("/", 0x9EC4D8);
    gprint_dec(cap, 0xEAF2F8);

    cursor_x = x + 10;
    cursor_y = y + 60;
    gprint("Free pages: ", 0x9EC4D8);
    gprint_dec(free_pages, 0xEAF2F8);

    cursor_x = x + 10;
    cursor_y = y + 76;
    gprint("Use: tz show | tz set +/-HH:MM", 0x8FB8CC);
}

/* ------------------------------------------------------------ */
/*  Initialisation                                               */
/* ------------------------------------------------------------ */
void wm_init(void) {
    int i;
    for (i = 0; i < WM_MAX_WINDOWS; i++) {
        wm_windows[i].visible = 0;
        wm_windows[i].state = WM_STATE_NORMAL;
        wm_windows[i].dragging = 0;
        wm_windows[i].draw_fn = 0;
        wm_windows[i].needs_keyboard = 0;
        wm_windows[i].title[0] = '\0';
        z_order[i] = i;
    }
    wm_window_count = 0;
    z_count = 0;
    wm_focused = -1;
    wm_load_timezone_from_cmos();
    launcher_init();

    /* Register default desktop windows bound to dock icon slots. */
    wm_add_window(62, 78, 300, 170, "Launcher", draw_content_launcher, 0, 1);
    wm_add_window(110, 120, 430, 240, "Neural Viz", draw_content_neuroviz, 0, 2);
    wm_add_window(156, 92, 360, 180, "Console", draw_content_console, 1, 3);
    wm_add_window(450, 110, 280, 210, "Profiler", draw_content_profiler, 0, 4);
    wm_add_window(480, 150, 250, 150, "Settings", draw_content_settings, 0, 5);
}

int wm_add_window(int x, int y, int w, int h, const char *title,
                   wm_draw_fn fn, int needs_kb, int icon_slot) {
    if (wm_window_count >= WM_MAX_WINDOWS) return -1;
    int idx = wm_window_count;
    WmWindow *win = &wm_windows[idx];
    win->x = x;  win->y = y;  win->w = w;  win->h = h;
    win->ox = x; win->oy = y; win->ow = w; win->oh = h;
    wm_strcpy(win->title, title, 48);
    win->state = WM_STATE_NORMAL;
    win->visible = 0;   /* closed on boot */
    win->dragging = 0;
    win->draw_fn = fn;
    win->needs_keyboard = needs_kb;
    win->icon_slot = icon_slot;
    z_order[z_count++] = idx;
    wm_window_count++;
    return idx;
}

int wm_focused_needs_keyboard(void) {
    if (wm_focused < 0 || wm_focused >= wm_window_count) return 0;
    WmWindow *w = &wm_windows[wm_focused];
    return w->visible && w->state != WM_STATE_MINIMIZED && w->needs_keyboard;
}

/* ------------------------------------------------------------ */
/*  Z-order management                                           */
/* ------------------------------------------------------------ */
static void bring_to_front(int idx) {
    int i, pos = -1;
    for (i = 0; i < z_count; i++) {
        if (z_order[i] == idx) { pos = i; break; }
    }
    if (pos < 0) return;
    for (i = pos; i < z_count - 1; i++) z_order[i] = z_order[i + 1];
    z_order[z_count - 1] = idx;
    wm_focused = idx;
}

/* ------------------------------------------------------------ */
/*  Desktop background                                           */
/* ------------------------------------------------------------ */
static void draw_desktop_bg(void) {
    int y;
    for (y = 0; y < WM_TASKBAR_Y; y++) {
        int r = 4 + (52 * y) / WM_TASKBAR_Y;
        int g = 34 + (126 * y) / WM_TASKBAR_Y;
        int b = 62 + (96 * y) / WM_TASKBAR_Y;
        uint32_t row_color = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        draw_hline(y, 0, 800, row_color);
    }
}

/* ------------------------------------------------------------ */
/*  Taskbar icon drawing (simple procedural pixel art)           */
/* ------------------------------------------------------------ */
static void draw_icon_os(int cx, int cy) {
    /* "OS" text - small monochrome */
    draw_char('O', cx - 7, cy - 4, 0x00DDEE);
    draw_char('S', cx + 1, cy - 4, 0x00DDEE);
}

static void draw_icon_launcher(int cx, int cy) {
    /* 3x3 grid of small squares */
    int i, j;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            draw_filled_rect(cx - 9 + j * 7, cy - 9 + i * 7, 5, 5, 0x00CCDD);
}

static void draw_icon_neuroviz(int cx, int cy) {
    /* Zigzag waveform */
    static const int pts[][2] = {{-10,4},{-6,-2},{-2,6},{2,-6},{6,2},{10,-4}};
    int i;
    for (i = 0; i < 5; i++) {
        int x0 = cx + pts[i][0], y0 = cy + pts[i][1];
        int x1 = cx + pts[i+1][0], y1 = cy + pts[i+1][1];
        int dx = x1 - x0;
        int steps = dx > 0 ? dx : -dx;
        if (steps == 0) steps = 1;
        int s;
        for (s = 0; s <= steps; s++) {
            int px = x0 + s * (x1 - x0) / steps;
            int py = y0 + s * (y1 - y0) / steps;
            put_pixel(px, py, 0x44FF88);
            put_pixel(px, py + 1, 0x44FF88);
        }
    }
}

static void draw_icon_console(int cx, int cy) {
    /* ">_" prompt */
    draw_char('>', cx - 7, cy - 4, 0x00FFCC);
    draw_char('_', cx + 1, cy - 4, 0x00FFCC);
}

static void draw_icon_profiler(int cx, int cy) {
    /* Bar chart */
    draw_filled_rect(cx - 8, cy + 2, 4, 8, 0x44AAFF);
    draw_filled_rect(cx - 3, cy - 4, 4, 14, 0x44FF88);
    draw_filled_rect(cx + 2, cy - 1, 4, 11, 0xFFCC44);
    draw_filled_rect(cx + 7, cy - 8, 4, 18, 0xFF6644);
}

static void draw_icon_settings(int cx, int cy) {
    /* Gear - simple cross + box */
    draw_filled_rect(cx - 2, cy - 2, 5, 5, 0xAABBCC);
    draw_hline(cy, cx - 9, cx + 10, 0xAABBCC);
    draw_vline(cx, cy - 9, cy + 9, 0xAABBCC);
    draw_hline(cy - 6, cx - 6, cx + 7, 0x889AAA);
    draw_hline(cy + 6, cx - 6, cx + 7, 0x889AAA);
    draw_vline(cx - 6, cy - 6, cy + 6, 0x889AAA);
    draw_vline(cx + 6, cy - 6, cy + 6, 0x889AAA);
}

static void draw_taskbar_icon_shape(int slot, int cx, int cy) {
    switch (slot) {
        case 0: draw_icon_os(cx, cy); break;
        case 1: draw_icon_launcher(cx, cy); break;
        case 2: draw_icon_neuroviz(cx, cy); break;
        case 3: draw_icon_console(cx, cy); break;
        case 4: draw_icon_profiler(cx, cy); break;
        case 5: draw_icon_settings(cx, cy); break;
    }
}

/* ------------------------------------------------------------ */
/*  Taskbar rendering                                            */
/* ------------------------------------------------------------ */
static int icon_x_pos(int slot) {
    /* 6 icons, 80px each, centered in 800px */
    int start_x = (800 - WM_ICON_SLOTS * 80) / 2;
    return start_x + slot * 80 + 40; /* center of slot */
}

void wm_launch_icon_slot(int slot) {
    int j;
    for (j = 0; j < wm_window_count; j++) {
        if (wm_windows[j].icon_slot == slot) {
            wm_windows[j].visible = 1;
            wm_windows[j].state = WM_STATE_NORMAL;
            bring_to_front(j);
            return;
        }
    }
}

void wm_get_taskbar_icon_center(int slot, int *out_x, int *out_y) {
    if (slot < 0) slot = 0;
    if (slot >= WM_ICON_SLOTS) slot = WM_ICON_SLOTS - 1;
    if (out_x) *out_x = icon_x_pos(slot);
    if (out_y) *out_y = WM_TASKBAR_Y + 14;
}

static void draw_taskbar(void) {
    int i;
    int panel_x = (800 - WM_ICON_SLOTS * 80) / 2 - 10;
    int panel_w = WM_ICON_SLOTS * 80 + 20;

    /* Keep a subtle footer strip but float glass elements over it. */
    draw_filled_rect(0, WM_TASKBAR_Y, 800, WM_TASKBAR_H, 0x071421);
    draw_hline(WM_TASKBAR_Y, 0, 800, 0x123647);

    /* Glassmorphic app drawer */
    draw_glass_rect(panel_x, WM_TASKBAR_Y + 3, panel_w, WM_TASKBAR_H - 6, 0x284763);

    /* Icons */
    for (i = 0; i < WM_ICON_SLOTS; i++) {
        int cx = icon_x_pos(i);
        int icon_y = WM_TASKBAR_Y + 14;

        /* Highlight if any window with this icon_slot is open */
        int highlighted = 0;
        int j;
        for (j = 0; j < wm_window_count; j++) {
            if (wm_windows[j].icon_slot == i && wm_windows[j].visible &&
                wm_windows[j].state != WM_STATE_MINIMIZED) {
                highlighted = 1; break;
            }
        }
        if (highlighted) {
            draw_glass_rect(cx - 18, WM_TASKBAR_Y + 6, 36, 36, 0x2B5A77);
        }

        /* Hover effect */
        if (pt_in_rect(mouse_x, mouse_y, cx - 20, WM_TASKBAR_Y + 4, 40, 46)) {
            draw_glass_rect(cx - 18, WM_TASKBAR_Y + 6, 36, 36, 0x3D6E8A);
        }

        draw_taskbar_icon_shape(i, cx, icon_y);

        /* Label */
        const char *lbl = icon_labels[i];
        int lbl_len = 0;
        while (lbl[lbl_len]) lbl_len++;
        int lx = cx - (lbl_len * 4); /* rough center */
        cursor_x = lx;
        cursor_y = WM_TASKBAR_Y + 30;
        gprint((char *)lbl, 0xBFD6E4);
    }

    /* Right-side glass status widget: real RTC time + real snapshot space. */
    {
        int wx = 646;
        int wy = WM_TASKBAR_Y - 2;
        int ww = 150;
        int wh = 50;
        int hrs = 0;
        int mins = 0;
        int battery = read_battery_percent();
        int on_ac = 0;
        int used = storage_snapshot_used_count();
        int cap = storage_snapshot_capacity();
        int space_pct = (cap > 0) ? ((used * 100) / cap) : 0;

        char tbuf[6];
        char bbuf[8];
        read_rtc_hm(&hrs, &mins);

        tbuf[0] = (char)('0' + (hrs % 100) / 10);
        tbuf[1] = (char)('0' + hrs % 10);
        tbuf[2] = ':';
        tbuf[3] = (char)('0' + (mins / 10));
        tbuf[4] = (char)('0' + mins % 10);
        tbuf[5] = '\0';

        if (battery >= 0 && battery <= 100) {
            bbuf[0] = (char)('0' + (battery / 10));
            bbuf[1] = (char)('0' + (battery % 10));
            bbuf[2] = '%';
            bbuf[3] = '\0';
        } else {
            /* No battery telemetry exposed: treat as line-powered desktop. */
            battery = 100;
            on_ac = 1;
            bbuf[0] = '1';
            bbuf[1] = '0';
            bbuf[2] = '0';
            bbuf[3] = '%';
            bbuf[4] = '\0';
        }

        draw_glass_rect(wx, wy, ww, wh, 0x263F56);

        cursor_x = wx + 12;
        cursor_y = wy + 8;
        gprint(tbuf, 0xE8F4FF);

        /* Battery gauge */
        draw_rect(wx + 66, wy + 9, 32, 10, 0x86B5CF);
        draw_filled_rect(wx + 98, wy + 12, 2, 4, 0x86B5CF);
        draw_filled_rect(wx + 68, wy + 11, (battery * 28) / 100, 6, 0x3CC56B);

        cursor_x = wx + 106;
        cursor_y = wy + 8;
        gprint(bbuf, 0xCDE3F2);
        if (on_ac) {
            cursor_x = wx + 130;
            cursor_y = wy + 8;
            gprint("AC", 0xA9C7D9);
        }

        cursor_x = wx + 12;
        cursor_y = wy + 28;
        gprint("Space:", 0x9FC2D8);
        gprint_dec(used, 0xBDEBFF);
        gprint("/", 0x9FC2D8);
        gprint_dec(cap, 0xBDEBFF);
        gprint(" ", 0x9FC2D8);
        gprint_dec(space_pct, 0xBDEBFF);
        gprint("%", 0x9FC2D8);

        if (power_menu_open) {
            int pmx = wx;
            int pmy = wy - 54;
            draw_glass_rect(pmx, pmy, ww, 50, 0x223749);
            cursor_x = pmx + 10;
            cursor_y = pmy + 7;
            gprint("Power", 0xE8F4FF);

            clear_region(pmx + 8, pmy + 22, pmx + 68, pmy + 38, 0x2B3E51);
            draw_rect(pmx + 8, pmy + 22, 60, 16, 0x7AA7BF);
            cursor_x = pmx + 12;
            cursor_y = pmy + 25;
            gprint("Reboot", 0xDDEEFF);

            clear_region(pmx + 76, pmy + 22, pmx + 142, pmy + 38, 0x5A2830);
            draw_rect(pmx + 76, pmy + 22, 66, 16, 0xC56A72);
            cursor_x = pmx + 98;
            cursor_y = pmy + 25;
            gprint("Off", 0xFFD6D6);
        }
    }
}

/* ------------------------------------------------------------ */
/*  Window chrome                                                */
/* ------------------------------------------------------------ */
static void draw_window_chrome(WmWindow *win) {
    int x = win->x, y = win->y, w = win->w, h = win->h;
    int is_focused = (&wm_windows[wm_focused] == win);

    uint32_t border_color = is_focused ? 0x00AACC : 0x224455;
    uint32_t title_bg     = is_focused ? 0x0C2636 : 0x0A1A28;
    uint32_t title_fg     = is_focused ? 0x66EEFF : 0x88AABB;
    uint32_t body_bg      = 0x08141E;

    /* Window body background */
    draw_filled_rect(x, y, w, h, body_bg);

    /* Title bar */
    draw_filled_rect(x, y, w, WM_TITLEBAR_H, title_bg);
    /* Title bar bottom line */
    draw_hline(y + WM_TITLEBAR_H - 1, x, x + w, border_color);

    /* Title text */
    cursor_x = x + 8;
    cursor_y = y + 7;
    gprint(win->title, title_fg);

    /* Buttons: minimize, maximize, close */
    int btn_y = y + WM_BTN_PAD;
    int close_x   = x + w - WM_BTN_W - WM_BTN_PAD;
    int max_x     = close_x - WM_BTN_W - 2;
    int min_x     = max_x - WM_BTN_W - 2;

    /* Minimize _ */
    draw_filled_rect(min_x, btn_y, WM_BTN_W, WM_BTN_H, 0x1A2A3A);
    draw_hline(btn_y + WM_BTN_H - 5, min_x + 3, min_x + WM_BTN_W - 3, 0x88CCDD);

    /* Maximize □ */
    draw_filled_rect(max_x, btn_y, WM_BTN_W, WM_BTN_H, 0x1A2A3A);
    draw_rect(max_x + 3, btn_y + 3, WM_BTN_W - 6, WM_BTN_H - 6, 0x88CCDD);

    /* Close × */
    draw_filled_rect(close_x, btn_y, WM_BTN_W, WM_BTN_H, 0x3A1A1A);
    {
        int cx0 = close_x + 4, cy0 = btn_y + 4;
        int s;
        for (s = 0; s < 8; s++) {
            put_pixel(cx0 + s,     cy0 + s, 0xFF6666);
            put_pixel(cx0 + s + 1, cy0 + s, 0xFF6666);
            put_pixel(cx0 + 7 - s, cy0 + s, 0xFF6666);
            put_pixel(cx0 + 8 - s, cy0 + s, 0xFF6666);
        }
    }

    /* Border around entire window */
    draw_rect(x, y, w, h, border_color);
    /* Outer glow for focused */
    if (is_focused) {
        draw_rect(x - 1, y - 1, w + 2, h + 2, 0x005566);
    }
}

/* ------------------------------------------------------------ */
/*  Mouse handling                                               */
/* ------------------------------------------------------------ */
void wm_handle_mouse(int mx, int my, int buttons, int prev_buttons) {
    int clicked   = (buttons & 1) && !(prev_buttons & 1);
    int held      = (buttons & 1);
    int released  = !(buttons & 1) && (prev_buttons & 1);
    int i;

    /* Route to launcher if it's open or animating */
    if (launcher_state.is_open || launcher_state.animation_state > 0) {
        launcher_handle_mouse(mx, my, buttons, prev_buttons);
        return;
    }

    /* Handle active drag */
    for (i = 0; i < wm_window_count; i++) {
        WmWindow *w = &wm_windows[i];
        if (w->dragging) {
            if (held) {
                w->x = mx - w->drag_ox;
                w->y = my - w->drag_oy;
                /* Clamp to screen */
                if (w->x < 0) w->x = 0;
                if (w->y < 0) w->y = 0;
                if (w->x + w->w > 800) w->x = 800 - w->w;
                if (w->y + w->h > WM_TASKBAR_Y) w->y = WM_TASKBAR_Y - w->h;
            }
            if (released) {
                w->dragging = 0;
            }
            return; /* Don't process other clicks while dragging */
        }
    }

    if (!clicked) return;

    if (power_menu_open) {
        int pmx = 646;
        int pmy = WM_TASKBAR_Y - 56;
        if (pt_in_rect(mx, my, pmx + 8, pmy + 20, 60, 16)) {
            system_reboot();
            return;
        }
        if (pt_in_rect(mx, my, pmx + 76, pmy + 20, 66, 16)) {
            system_shutdown();
            return;
        }
        power_menu_open = 0;
    }

    /* 1. Check taskbar clicks */
    if (my >= WM_TASKBAR_Y) {
        if (pt_in_rect(mx, my, 646, WM_TASKBAR_Y - 2, 150, 50)) {
            power_menu_open = !power_menu_open;
            return;
        }
        for (i = 0; i < WM_ICON_SLOTS; i++) {
            int cx = icon_x_pos(i);
            if (pt_in_rect(mx, my, cx - 20, WM_TASKBAR_Y + 4, 40, 46)) {
                if (i == 0) {
                    /* OS logo - launch NeuroDock launcher */
                    launcher_toggle();
                    return;
                }
                if (i == 1) {
                    /* App Launcher - toggle all windows */
                    int any_open = 0;
                    int j;
                    for (j = 0; j < wm_window_count; j++)
                        if (wm_windows[j].visible) any_open = 1;
                    for (j = 0; j < wm_window_count; j++) {
                        wm_windows[j].visible = any_open ? 0 : 1;
                        if (!any_open) wm_windows[j].state = WM_STATE_NORMAL;
                    }
                    return;
                }
                /* Toggle windows with matching icon_slot */
                {
                    int j;
                    for (j = 0; j < wm_window_count; j++) {
                        if (wm_windows[j].icon_slot == i) {
                            if (wm_windows[j].visible) {
                                if (wm_windows[j].state == WM_STATE_MINIMIZED) {
                                    wm_windows[j].state = WM_STATE_NORMAL;
                                    bring_to_front(j);
                                } else {
                                    wm_windows[j].visible = 0;
                                }
                            } else {
                                wm_windows[j].visible = 1;
                                wm_windows[j].state = WM_STATE_NORMAL;
                                bring_to_front(j);
                            }
                        }
                    }
                }
                return;
            }
        }
        return;
    }

    /* 2. Check windows from topmost to bottommost */
    for (i = z_count - 1; i >= 0; i--) {
        int idx = z_order[i];
        WmWindow *w = &wm_windows[idx];
        if (!w->visible || w->state == WM_STATE_MINIMIZED) continue;
        if (!pt_in_rect(mx, my, w->x, w->y, w->w, w->h)) continue;

        /* Found the window under cursor */
        bring_to_front(idx);

        /* Check title bar buttons */
        int btn_y = w->y + WM_BTN_PAD;
        int close_x = w->x + w->w - WM_BTN_W - WM_BTN_PAD;
        int max_x   = close_x - WM_BTN_W - 2;
        int min_x   = max_x - WM_BTN_W - 2;

        /* Close */
        if (pt_in_rect(mx, my, close_x, btn_y, WM_BTN_W, WM_BTN_H)) {
            w->visible = 0;
            if (wm_focused == idx) wm_focused = -1;
            return;
        }
        /* Maximize */
        if (pt_in_rect(mx, my, max_x, btn_y, WM_BTN_W, WM_BTN_H)) {
            if (w->state == WM_STATE_MAXIMIZED) {
                w->x = w->ox; w->y = w->oy;
                w->w = w->ow; w->h = w->oh;
                w->state = WM_STATE_NORMAL;
            } else {
                w->ox = w->x; w->oy = w->y;
                w->ow = w->w; w->oh = w->h;
                w->x = 0; w->y = 0;
                w->w = 800; w->h = WM_TASKBAR_Y;
                w->state = WM_STATE_MAXIMIZED;
            }
            return;
        }
        /* Minimize */
        if (pt_in_rect(mx, my, min_x, btn_y, WM_BTN_W, WM_BTN_H)) {
            w->state = WM_STATE_MINIMIZED;
            return;
        }

        /* Title bar drag */
        if (my < w->y + WM_TITLEBAR_H && w->state != WM_STATE_MAXIMIZED) {
            w->dragging = 1;
            w->drag_ox = mx - w->x;
            w->drag_oy = my - w->y;
            return;
        }

        /* Clicked in content area - just focus */
        return;
    }

    /* Click on desktop - unfocus */
    power_menu_open = 0;
    wm_focused = -1;
}

/* ------------------------------------------------------------ */
/*  Arrow mouse cursor (replaces crosshair)                      */
/* ------------------------------------------------------------ */
static const uint8_t arrow_cursor[16][12] = {
    {1,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0,0,0},
    {1,2,2,2,2,2,2,1,0,0,0,0},
    {1,2,2,2,2,2,2,2,1,0,0,0},
    {1,2,2,2,2,1,1,1,1,0,0,0},
    {1,2,2,1,2,2,1,0,0,0,0,0},
    {1,2,1,0,1,2,2,1,0,0,0,0},
    {1,1,0,0,1,2,2,1,0,0,0,0},
    {1,0,0,0,0,1,2,2,1,0,0,0},
    {0,0,0,0,0,1,2,2,1,0,0,0},
    {0,0,0,0,0,0,1,1,0,0,0,0},
};

static void wm_draw_cursor(void) {
    int mx = mouse_x;
    int my = mouse_y;
    int r, c;
    uint32_t btn_tint = (mouse_buttons & 0x1) ? 0xFFCC44 : 0xFFFFFF;
    for (r = 0; r < 16; r++) {
        for (c = 0; c < 12; c++) {
            uint8_t v = arrow_cursor[r][c];
            if (v == 1) put_pixel(mx + c, my + r, 0x000000);
            else if (v == 2) put_pixel(mx + c, my + r, btn_tint);
        }
    }
}


/* ------------------------------------------------------------ */
/*  Main render call                                             */
/* ------------------------------------------------------------ */
void wm_render(void) {
    int i;

    /* 1. Desktop background */
    draw_desktop_bg();

    /* 2. Windows in z-order (bottom to top) */
    for (i = 0; i < z_count; i++) {
        int idx = z_order[i];
        WmWindow *w = &wm_windows[idx];
        if (!w->visible || w->state == WM_STATE_MINIMIZED) continue;

        draw_window_chrome(w);

        /* Content area: inside title bar and border */
        if (w->draw_fn) {
            int cx = w->x + WM_BORDER;
            int cy = w->y + WM_TITLEBAR_H;
            int cw = w->w - WM_BORDER * 2;
            int ch = w->h - WM_TITLEBAR_H - WM_BORDER;
            w->draw_fn(cx, cy, cw, ch);
        }
    }

    /* 3. Taskbar on top */
    draw_taskbar();

    /* 4. NeuroDock launcher overlay */
    launcher_render();

    /* 5. Mouse cursor on very top */
    wm_draw_cursor();
}
