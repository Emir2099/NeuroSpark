extern int screen_w; extern int screen_h;
#include "task.h"
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
int storage_get_snapshot_tag(int slot, char *out, int out_len);

int handle_replay_control_click(WmWindow *w, int mx, int my);
void draw_spike_glow_dot(int x, int y);
void draw_content_telemetry(int cx, int cy, int cw, int ch);

extern void process_command(char *);

#include "launcher.h"
#include "model_manager.h"
#include "net.h"

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
static uint32_t wm_frames_total = 0;
static uint32_t wm_last_fps_tick = 0;
static uint32_t wm_last_fps_frames = 0;
static uint32_t wm_fps_x10 = 0;


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
        if (yy < 0 || yy >= screen_h) continue;
        for (xx = x; xx < x + w; xx++) {
            if (xx < 0 || xx >= screen_w) continue;
            int idx = yy * screen_w + xx;
            backbuffer[idx] = blend_50(backbuffer[idx], tint);
        }
    }

    draw_hline(y, x + 1, x + w - 1, 0x66AFC6);
    draw_vline(x, y + 1, y + h - 1, 0x66AFC6);
    draw_hline(y + h - 1, x + 1, x + w - 1, 0x08131A);
    draw_vline(x + w - 1, y + 1, y + h - 1, 0x08131A);
}

typedef struct {
    int selected_task;
    int lr_fast[2];
    int lr_slow[2];
    int refractory_ticks[2];
    int tau[2];
    int toggles[2][4];
    int base_lr_fast[2];
    int base_lr_slow[2];
    int base_refractory_ticks[2];
    int base_tau[2];
    int base_toggles[2][4];
    int dirty_mask;
    uint32_t last_apply_tick;
    uint32_t last_sync_tick;
    uint32_t last_graph_tick;
    int graph_head;
    int graph_hist[64];
    int initialized;
} WmModelManagerUiState;

static WmModelManagerUiState g_model_ui = {
    0,
    {4, 4}, {8, 8}, {10, 10}, {2, 2},
    {{1, 1, 1, 1}, {1, 1, 1, 1}},
    {4, 4}, {8, 8}, {10, 10}, {2, 2},
    {{1, 1, 1, 1}, {1, 1, 1, 1}},
    0,
    0, 0, 0,
    0,
    {0},
    0
};

static int wm_clamp_i32(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int wm_str_eq(const char *a, const char *b) {
    int i = 0;
    if (!a || !b) return 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return a[i] == '\0' && b[i] == '\0';
}

static void gprint_clipped(const char *text, int max_chars, uint32_t color) {
    int i = 0;
    if (!text || max_chars <= 0) return;
    while (text[i] && i < max_chars) {
        char ch[2];
        ch[0] = text[i];
        ch[1] = '\0';
        gprint(ch, color);
        i++;
    }
}

static int model_ui_selected_task(void) {
    return wm_clamp_i32(g_model_ui.selected_task, 0, 1);
}

static int model_ui_task_dirty(int t) {
    return (g_model_ui.dirty_mask & (1 << t)) != 0;
}

static void model_ui_mark_dirty(int t) {
    g_model_ui.dirty_mask |= (1 << t);
}

static void model_ui_clear_dirty(int t) {
    g_model_ui.dirty_mask &= ~(1 << t);
}

static int model_ui_lr_ref(int t) {
    int tf = wm_clamp_i32(t, 0, 1);
    return (g_model_ui.lr_fast[tf] * 8) + (g_model_ui.lr_slow[tf] * 3);
}

static void model_ui_capture_task_baseline(int t) {
    int tf = wm_clamp_i32(t, 0, 1);
    int i;
    g_model_ui.base_lr_fast[tf] = g_model_ui.lr_fast[tf];
    g_model_ui.base_lr_slow[tf] = g_model_ui.lr_slow[tf];
    g_model_ui.base_refractory_ticks[tf] = g_model_ui.refractory_ticks[tf];
    g_model_ui.base_tau[tf] = g_model_ui.tau[tf];
    for (i = 0; i < 4; i++) {
        g_model_ui.base_toggles[tf][i] = g_model_ui.toggles[tf][i];
    }
}

static void model_ui_restore_task_baseline(int t) {
    int tf = wm_clamp_i32(t, 0, 1);
    int i;
    g_model_ui.lr_fast[tf] = g_model_ui.base_lr_fast[tf];
    g_model_ui.lr_slow[tf] = g_model_ui.base_lr_slow[tf];
    g_model_ui.refractory_ticks[tf] = g_model_ui.base_refractory_ticks[tf];
    g_model_ui.tau[tf] = g_model_ui.base_tau[tf];
    for (i = 0; i < 4; i++) {
        g_model_ui.toggles[tf][i] = g_model_ui.base_toggles[tf][i];
    }
    model_ui_clear_dirty(tf);
}

static void model_ui_update_graph_history(void) {
    if (!g_model_ui.initialized) return;
    if (g_model_ui.last_graph_tick == tick) return;

    while (g_model_ui.last_graph_tick < tick) {
        int i;
        int peak = 0;
        g_model_ui.last_graph_tick++;
        for (i = 0; i < 16; i++) {
            int p = wm_clamp_i32(potentials[i], 0, 1000);
            if (p > peak) peak = p;
        }
        g_model_ui.graph_hist[g_model_ui.graph_head] = peak;
        g_model_ui.graph_head = (g_model_ui.graph_head + 1) & 63;
    }
}

static void model_ui_sync_task_from_kernel(int t) {
    int tf = wm_clamp_i32(t, 0, 1);
    ModelManifestData mf;

    g_model_ui.toggles[tf][0] = model_get_stdp() ? 1 : 0;

    if (model_manifest_capture(&mf)) {
        int lr = wm_clamp_i32(mf.learning_rate[tf][0], 1, 80);
        g_model_ui.lr_fast[tf] = wm_clamp_i32((lr + 3) / 4, 1, 10);
        g_model_ui.lr_slow[tf] = wm_clamp_i32((lr + 7) / 8, 1, 10);
        g_model_ui.refractory_ticks[tf] = wm_clamp_i32(mf.refractory_ticks[tf][0], 1, 40);
        g_model_ui.tau[tf] = wm_clamp_i32(mf.tau[tf][0], 1, 8);
    } else {
        g_model_ui.refractory_ticks[tf] = wm_clamp_i32(model_get_refractory_ticks(tf, 0), 1, 40);
        g_model_ui.tau[tf] = 2;
    }

    model_ui_clear_dirty(tf);
    model_ui_capture_task_baseline(tf);
    g_model_ui.last_sync_tick = tick;
}

static void model_ui_sync_from_kernel(void) {
    int i;
    g_model_ui.selected_task = model_ui_selected_task();
    g_model_ui.last_graph_tick = tick;
    g_model_ui.graph_head = 0;
    for (i = 0; i < 64; i++) {
        g_model_ui.graph_hist[i] = 0;
    }
    for (i = 0; i < 2; i++) {
        model_ui_sync_task_from_kernel(i);
        g_model_ui.toggles[i][1] = 1;
        g_model_ui.toggles[i][2] = 1;
        g_model_ui.toggles[i][3] = 1;
        model_ui_capture_task_baseline(i);
    }
    g_model_ui.initialized = 1;
}

static void model_ui_apply_to_kernel(void) {
    int t = model_ui_selected_task();
    int lr = wm_clamp_i32(model_ui_lr_ref(t), 1, 80);

    model_set_stdp(g_model_ui.toggles[t][0] ? 1 : 0);
    if (g_model_ui.toggles[t][1]) {
        (void)model_set_param(t, -1, "lr", lr);
    }
    if (g_model_ui.toggles[t][2]) {
        (void)model_set_param(t, -1, "refractory", wm_clamp_i32(g_model_ui.refractory_ticks[t], 1, 40));
    }
    if (g_model_ui.toggles[t][3]) {
        (void)model_set_param(t, -1, "tau", wm_clamp_i32(g_model_ui.tau[t], 1, 8));
    }

    model_ui_capture_task_baseline(t);
    model_ui_clear_dirty(t);
    g_model_ui.last_apply_tick = tick;
    g_model_ui.last_sync_tick = tick;
}

static const char *model_display_name(const char *name) {
    if (wm_str_eq(name, "adapt")) return "Adaptive LIF";
    if (wm_str_eq(name, "lif")) return "LIF";
    if (wm_str_eq(name, "stdp")) return "STDP";
    return name;
}

static const char *workload_display_name(int t) {
    if (t >= 0 && t < 2 && task_list[t].task_name && task_list[t].task_name[0]) {
        if (wm_str_eq(task_list[t].task_name, "IO")) {
            return "ardent_simulator";
        }
        if (wm_str_eq(task_list[t].task_name, "COMP")) {
            return "logger";
        }
        return task_list[t].task_name;
    }
    return (t == 0) ? "ardent_simulator" : "logger";
}

static void draw_model_slider(int x, int y, int w, const char *label,
                              int value, int min_v, int max_v,
                              uint32_t fill_color) {
    int track_x = x;
    int track_y = y + 13;
    int track_h = 8;
    int span = (max_v - min_v) > 0 ? (max_v - min_v) : 1;
    int fill_w = ((value - min_v) * (w - 2)) / span;
    int knob_x = track_x + 1 + fill_w;

    if (fill_w < 0) fill_w = 0;
    if (fill_w > w - 2) fill_w = w - 2;
    if (knob_x < track_x + 2) knob_x = track_x + 2;
    if (knob_x > track_x + w - 4) knob_x = track_x + w - 4;

    cursor_x = x;
    cursor_y = y;
    gprint((char *)label, 0xB6E2F2);

    draw_filled_rect(track_x, track_y, w, track_h, 0x0A2436);
    draw_rect(track_x, track_y, w, track_h, 0x2F7A97);
    put_pixel(track_x - 1, track_y + (track_h / 2), 0x52EFFF);
    put_pixel(track_x - 2, track_y + (track_h / 2), 0x52EFFF);
    put_pixel(track_x + w + 1, track_y + (track_h / 2), 0x52EFFF);
    draw_filled_rect(track_x + 1, track_y + 1, fill_w, track_h - 2, fill_color);
    draw_filled_rect(knob_x - 2, track_y - 1, 5, track_h + 2, 0x8EF6FF);

}

static void draw_model_value_chip(int x, int y, int value, int show_lr_format, int show_ms) {
    draw_filled_rect(x, y, 44, 14, 0x123C4F);
    draw_rect(x, y, 44, 14, 0x4BE8F1);
    draw_rect(x + 1, y + 1, 42, 12, 0x1B6D86);

    cursor_x = x + 4;
    cursor_y = y + 1;
    if (show_lr_format) {
        gprint("0.", 0x9AF4F9);
        if (value < 10) gprint("0", 0x9AF4F9);
        gprint_dec(value, 0x9AF4F9);
    } else {
        gprint_dec(value, 0x9AF4F9);
        if (show_ms) {
            gprint("ms", 0x9AF4F9);
        }
    }
}

static void draw_model_toggle(int x, int y, int on) {
    uint32_t bg = on ? 0x22D1C7 : 0x1A2D3A;
    uint32_t knob = 0xB5FCFF;
    draw_filled_rect(x, y, 44, 14, bg);
    draw_rect(x, y, 44, 14, 0x69F1F5);
    draw_rect(x + 1, y + 1, 42, 12, on ? 0x2CE7DD : 0x2C4A59);
    draw_filled_rect(x + 2, y + 2, 40, 10, on ? 0x28CFC5 : 0x223A48);
    if (on) {
        draw_filled_rect(x + 29, y + 2, 12, 10, knob);
        draw_rect(x + 29, y + 2, 12, 10, 0x4AB5C5);
    } else {
        draw_filled_rect(x + 3, y + 2, 12, 10, knob);
        draw_rect(x + 3, y + 2, 12, 10, 0x4AB5C5);
    }
}

static void draw_model_health_gauge(int x, int y, int w, const char *label,
                                    int value, uint32_t color) {
    int v = wm_clamp_i32(value, 0, 100);
    int fill_w = (v * (w - 2)) / 100;

    cursor_x = x;
    cursor_y = y;
    gprint((char *)label, 0xC6E6F5);

    draw_filled_rect(x, y + 12, w, 10, 0x0D2232);
    draw_rect(x, y + 12, w, 10, 0x2A617B);
    draw_filled_rect(x + 1, y + 13, fill_w, 8, color);
    if (wm_str_eq(label, "Jitter Latency")) {
        int seg;
        static const uint32_t jitter_cols[14] = {
            0x5BEFF5, 0x58EEE9, 0x54EDD8, 0x5CECA5,
            0x79E57D, 0xA9DF63, 0xDAD961, 0xE7CC58,
            0xC9C55C, 0x8EC965, 0x52C679, 0x2FAE88,
            0x2B8B8E, 0x246A81
        };
        for (seg = 0; seg < 14; seg++) {
            int sx0 = x + 1 + (seg * (w - 2)) / 14;
            int sx1 = x + 1 + ((seg + 1) * (w - 2)) / 14;
            draw_filled_rect(sx0, y + 13, sx1 - sx0 - 1, 8, jitter_cols[seg]);
        }
        {
            int marker_x = x + (w * 10) / 14;
            draw_vline(marker_x, y + 11, y + 22, 0x3DE9F7);
        }
    } else {
        int seg;
        for (seg = 1; seg < 12; seg++) {
            int sx = x + (seg * (w - 2)) / 12;
            draw_vline(sx, y + 13, y + 20, 0x163E55);
        }
    }
    draw_filled_rect(x + w + 4, y + 10, 40, 14, 0x153A4D);
    draw_rect(x + w + 4, y + 10, 40, 14, 0x4BDDEB);
    draw_rect(x + w + 5, y + 11, 38, 12, 0x206683);
    cursor_x = x + w + 10;
    cursor_y = y + 12;
    gprint_dec(v, 0xEAFBFF);
    gprint("%", 0xEAFBFF);
}

static void draw_content_model_manager(int x, int y, int w, int h) {
    int px = x + 6;
    int py = y + 6;
    int pw = w - 12;
    int ph = h - 12;

    int left_w = 214;
    int right_w = 190;
    int mid_x;
    int mid_w;
    int jitter;
    int integrity;
    int cluster;
    int t;
    int task_idx;
    int slider_w;
    int graph_x;
    int graph_y;
    int graph_w;
    int graph_h;
    int toggle_x;
    int value_x;
    int control_x;
    int row1_y;
    int row2_y;
    int row3_y;
    int row4_y;

    if (!g_model_ui.initialized) {
        model_ui_sync_from_kernel();
    }
    model_ui_update_graph_history();
    task_idx = model_ui_selected_task();

    if (left_w + right_w + 36 > pw) {
        left_w = pw / 3;
        right_w = pw / 3;
    }
    mid_x = px + left_w + 14;
    mid_w = pw - left_w - right_w - 30;
    graph_x = mid_x + mid_w - 140;
    graph_y = py + 214;
    graph_w = 126;
    graph_h = 72;
    row1_y = py + 64;
    row2_y = row1_y + 48;
    row3_y = row2_y + 48;
    row4_y = graph_y + 8;
    slider_w = (graph_x - 16) - (mid_x + 10);
    if (slider_w < 120) slider_w = 120;
    control_x = mid_x + 10;
    value_x = control_x;
    toggle_x = control_x + 50;

    for (t = 0; t < 56; t++) {
        int sx = px + ((t * 37 + (int)tick) % (pw - 8)) + 4;
        int sy = py + ((t * 19 + (int)(tick >> 1)) % (ph - 8)) + 4;
        put_pixel(sx, sy, 0x10304A);
    }

    draw_filled_rect(px, py, pw, ph, 0x071224);
    draw_rect(px, py, pw, ph, 0x2F6D89);
    draw_hline(py + 18, px + 1, px + pw - 1, 0x2BA8C3);

    cursor_x = px + 8;
    cursor_y = py + 4;
    gprint("MODEL MANAGER", 0x86EAFF);

    draw_filled_rect(px + 6, py + 22, left_w, ph - 28, 0x061C30);
    draw_rect(px + 6, py + 22, left_w, ph - 28, 0x3AC2D9);
    draw_hline(py + 50, px + 7, px + left_w + 5, 0x1E6A86);
    cursor_x = px + 12;
    cursor_y = py + 28;
    gprint("ACTIVE WORKLOADS - PID", 0xBFEEFF);
    cursor_x = px + 12;
    cursor_y = py + 42;
    gprint("Strict user processes", 0x86BDD2);

    for (t = 0; t < 2; t++) {
        int row_y = py + 58 + t * 26;
        uint32_t row_bg = (t == task_idx) ? 0x0C3C57 : 0x0A1D2D;
        uint32_t row_fg = (t == task_idx) ? 0xCFFFFF : 0xB2D8E8;
        draw_filled_rect(px + 10, row_y, left_w - 8, 20, row_bg);
        draw_rect(px + 10, row_y, left_w - 8, 20, 0x2A6D88);
        cursor_x = px + 14;
        cursor_y = row_y + 4;
        gprint_dec(t + 1, row_fg);
        gprint(". ", row_fg);
        gprint_clipped((char *)workload_display_name(t), 13, row_fg);
        gprint(" (R3)", 0x8FBDD2);
    }

    draw_hline(py + ph - 50, px + 7, px + left_w + 5, 0x1E6A86);
    cursor_x = px + 12;
    cursor_y = py + ph - 38;
    gprint("Active model name:", 0x9CC8DC);
    cursor_x = px + 12;
    cursor_y = py + ph - 22;
    gprint((char *)model_display_name(model_name(task_idx)), 0xD1F7FF);
    gprint(" baseline", 0x9CC8DC);

    draw_filled_rect(mid_x, py + 22, mid_w, ph - 28, 0x071A2A);
    draw_rect(mid_x, py + 22, mid_w, ph - 28, 0x46D3E7);
    draw_hline(py + 50, mid_x + 1, mid_x + mid_w - 1, 0x1E6A86);
    cursor_x = mid_x + 8;
    cursor_y = py + 28;
    gprint("STDP PARAMETER TUNING", 0xC5F5FF);
    gprint(" - Model: ", 0x84B7CB);
    gprint((char *)model_display_name(model_name(task_idx)), 0x9CF3FF);
    cursor_x = mid_x + 8;
    cursor_y = py + 42;
    gprint("Phase 10 STDP parameter definition", 0x84B7CB);

    draw_model_slider(mid_x + 10, row1_y, slider_w,
                      "STDP learning rates (0.01 - 0.10)",
                      g_model_ui.lr_fast[task_idx], 1, 10, 0x39D8E8);
    draw_model_value_chip(value_x, row1_y + 24, g_model_ui.lr_fast[task_idx], 1, 0);
    draw_model_toggle(toggle_x, row1_y + 24, g_model_ui.toggles[task_idx][0]);

    draw_model_slider(mid_x + 10, row2_y, slider_w,
                      "STDP learning rates (0.05 - 0.10)",
                      g_model_ui.lr_slow[task_idx], 1, 10, 0x3AB8F2);
    draw_model_value_chip(value_x, row2_y + 24, g_model_ui.lr_slow[task_idx], 1, 0);
    draw_model_toggle(toggle_x, row2_y + 24, g_model_ui.toggles[task_idx][1]);

    draw_model_slider(mid_x + 10, row3_y, slider_w,
                      "Refractory periods (1ms)",
                      g_model_ui.refractory_ticks[task_idx], 1, 40, 0x49EEC4);
    draw_model_value_chip(value_x, row3_y + 24, g_model_ui.refractory_ticks[task_idx], 0, 1);
    draw_model_toggle(toggle_x, row3_y + 24, g_model_ui.toggles[task_idx][2]);

    draw_model_slider(mid_x + 10, row4_y, slider_w,
                      "Time constants (1m)",
                      g_model_ui.tau[task_idx], 1, 8, 0x62E5C1);
    draw_model_value_chip(value_x, row4_y + 24, g_model_ui.tau[task_idx], 0, 0);
    draw_model_toggle(toggle_x, row4_y + 24, g_model_ui.toggles[task_idx][3]);

    draw_filled_rect(graph_x, graph_y, graph_w, graph_h, 0x0A2235);
    draw_rect(graph_x, graph_y, graph_w, graph_h, 0x3AC2D9);
    cursor_x = graph_x + 12;
    cursor_y = graph_y + 4;
    gprint("Potentials", 0xBEEFFD);
    {
        int gx = graph_x + 4;
        int gy = graph_y + 16;
        int gw = graph_w - 8;
        int gh = 52;
        int i;
        int px0 = gx + 2;
        int py0 = gy + gh - 3;
        int pmax = 0;
        int start = (g_model_ui.graph_head + 64 - 32) & 63;
        draw_rect(gx, gy, gw, gh, 0x2E6F8A);
        for (i = 1; i < 4; i++) {
            draw_hline(gy + (i * gh) / 4, gx + 1, gx + gw - 1, 0x1A465F);
        }
        for (i = 0; i < 32; i++) {
            int p = wm_clamp_i32(g_model_ui.graph_hist[(start + i) & 63], 0, 1000);
            if (p > pmax) pmax = p;
        }
        pmax = wm_clamp_i32(pmax, 120, 1000);
        for (i = 0; i < 32; i++) {
            int p = wm_clamp_i32(g_model_ui.graph_hist[(start + i) & 63], 0, pmax);
            int xx = gx + 2 + (i * (gw - 5)) / 31;
            int yy = gy + gh - 3 - (p * (gh - 8)) / pmax;
            draw_line_segment(px0, py0, xx, yy, 0x89E7FF);
            px0 = xx;
            py0 = yy;
        }
    }

    draw_filled_rect(mid_x + 12, py + ph - 108, mid_w - 24, 58, 0x0A2234);
    draw_rect(mid_x + 12, py + ph - 108, mid_w - 24, 58, 0x3A7992);
    cursor_x = mid_x + 18;
    cursor_y = py + ph - 102;
    gprint("STDP: ", 0xA8D8E8);
        gprint(g_model_ui.toggles[task_idx][0] ? "ON" : "OFF",
            g_model_ui.toggles[task_idx][0] ? 0x7BFFBA : 0xFFCC88);
    cursor_x = mid_x + 18;
    cursor_y = py + ph - 88;
    gprint("Task model: ", 0xA8D8E8);
    gprint((char *)model_display_name(model_name(task_idx)), 0xD3F8FF);
    cursor_x = mid_x + 18;
    cursor_y = py + ph - 74;
    gprint("LR(ref): ", 0xA8D8E8);
    gprint_dec(model_ui_lr_ref(task_idx), 0xD3F8FF);

    if (model_ui_task_dirty(task_idx)) {
        cursor_x = mid_x + 120;
        cursor_y = py + ph - 102;
        gprint("pending changes", 0xFFCC66);
    }

    draw_filled_rect(px + pw - right_w - 6, py + 22, right_w, ph - 28, 0x081F31);
    draw_rect(px + pw - right_w - 6, py + 22, right_w, ph - 28, 0x3AC2D9);
    draw_hline(py + 50, px + pw - right_w - 5, px + pw - 7, 0x1E6A86);
    cursor_x = px + pw - right_w + 2;
    cursor_y = py + 28;
    gprint("MODEL HEALTH & STATUS", 0xCCF5FF);

    {
        int pmin = 10000;
        int pmax = 0;
        int psum = 0;
        for (t = 0; t < 16; t++) {
            int p = wm_clamp_i32(potentials[t], 0, 1000);
            if (p < pmin) pmin = p;
            if (p > pmax) pmax = p;
            psum += p;
        }
        jitter = wm_clamp_i32(((pmax - pmin) * 100) / 1000, 12, 95);
        integrity = wm_clamp_i32(95 - (jitter / 4), 72, 96);
        cluster = wm_clamp_i32((psum * 100) / (16 * 1000), 28, 90);
    }

    draw_model_health_gauge(px + pw - right_w + 2, py + 50, right_w - 54,
                            "Jitter Latency", jitter, 0xF2BE4A);
    draw_model_health_gauge(px + pw - right_w + 2, py + 84, right_w - 54,
                            "Experiment Integrity", integrity, 0x45E8D7);
    draw_model_health_gauge(px + pw - right_w + 2, py + 118, right_w - 54,
                            "Cluster Health", cluster, 0x6EEA78);

    draw_filled_rect(px + pw - right_w + 2, py + 152, right_w - 18, right_w - 40, 0x0A2031);
    draw_rect(px + pw - right_w + 2, py + 152, right_w - 18, right_w - 40, 0x2A607B);
    {
        int gx = px + pw - right_w + 18;
        int gy = py + 166;
        int gw = right_w - 50;
        int gh = right_w - 72;
        int i;
        draw_filled_rect(gx, gy, gw, gh, 0x102439);
        draw_rect(gx, gy, gw, gh, 0x386D88);
        for (i = 1; i < 8; i++) {
            draw_hline(gy + (i * gh) / 8, gx + 1, gx + gw - 1, 0x1C445C);
            draw_vline(gx + (i * gw) / 8, gy + 1, gy + gh - 1, 0x1C445C);
        }
        {
            int cols = 14;
            int rows = 14;
            int usable_w = gw - 10;
            int usable_h = gh - 10;
            int cx0 = gx + 5;
            int cy0 = gy + 5;
            int cy;
            int cx;
            int center_r = 4 + (cluster / 18);
            for (cy = 0; cy < rows; cy++) {
                for (cx = 0; cx < cols; cx++) {
                    int dx = cx - (cols / 2);
                    int dy = cy - (rows / 2);
                    int d2 = dx * dx + dy * dy;
                    int intensity = (cluster + 25) - (d2 * 3) + (center_r * 2);
                    uint32_t c = 0x1A4358;
                    int x0 = cx0 + (cx * usable_w) / cols;
                    int x1 = cx0 + ((cx + 1) * usable_w) / cols;
                    int y0 = cy0 + (cy * usable_h) / rows;
                    int y1 = cy0 + ((cy + 1) * usable_h) / rows;
                    int cw = x1 - x0 - 1;
                    int ch = y1 - y0 - 1;
                    if (intensity > 72) c = 0xE2CE67;
                    else if (intensity > 56) c = 0x95D86F;
                    else if (intensity > 42) c = 0x56C68E;
                    else if (intensity > 30) c = 0x3398A9;
                    else if (intensity > 20) c = 0x286E8D;
                    if (cw < 1) cw = 1;
                    if (ch < 1) ch = 1;
                    draw_filled_rect(x0, y0, cw, ch, c);
                }
            }
            for (cx = 0; cx <= cols; cx++) {
                int gxv = cx0 + (cx * usable_w) / cols;
                draw_vline(gxv, cy0, cy0 + usable_h - 1, 0x1C445C);
            }
            for (cy = 0; cy <= rows; cy++) {
                int gyh = cy0 + (cy * usable_h) / rows;
                draw_hline(gyh, cx0, cx0 + usable_w - 1, 0x1C445C);
            }
        }
    }

    draw_filled_rect(px + pw - 172, py + ph - 66, 154, 24, 0x6A3F10);
    draw_rect(px + pw - 172, py + ph - 66, 154, 24, 0xF6A947);
    draw_rect(px + pw - 173, py + ph - 67, 156, 26, 0x7A4A12);
    cursor_x = px + pw - 154;
    cursor_y = py + ph - 60;
    gprint("APPLY TUNING", 0xFFE4B8);

    draw_filled_rect(px + pw - 172, py + ph - 36, 154, 22, 0x3A1C20);
    draw_rect(px + pw - 172, py + ph - 36, 154, 22, 0xC0707D);
    cursor_x = px + pw - 143;
    cursor_y = py + ph - 31;
    gprint("REVERT", 0xFFD2D8);

    if ((tick - g_model_ui.last_apply_tick) < 80u && g_model_ui.last_apply_tick != 0u) {
        cursor_x = px + pw - 168;
        cursor_y = py + ph - 86;
        gprint("APPLIED", 0x77FFAA);
    }
}

static int handle_model_manager_click(WmWindow *w, int mx, int my) {
    int cx = w->x + WM_BORDER;
    int cy = w->y + WM_TITLEBAR_H;
    int cw = w->w - WM_BORDER * 2;
    int ch = w->h - WM_TITLEBAR_H - WM_BORDER;

    int px = cx + 6;
    int py = cy + 6;
    int pw = cw - 12;
    int left_w = 214;
    int right_w = 190;
    int ph = ch - 12;
    int mid_x;
    int mid_w;
    int slider_w;
    int graph_x;
    int graph_y;
    int toggle_x;
    int value_x;
    int control_x;
    int task_idx;
    int row1_y;
    int row2_y;
    int row3_y;
    int row4_y;

    int rel;

    if (left_w + right_w + 36 > pw) {
        left_w = pw / 3;
        right_w = pw / 3;
    }
    mid_x = px + left_w + 14;
    mid_w = pw - left_w - right_w - 30;
    graph_x = mid_x + mid_w - 140;
    graph_y = py + 214;
    row1_y = py + 64;
    row2_y = row1_y + 48;
    row3_y = row2_y + 48;
    row4_y = graph_y + 8;
    slider_w = (graph_x - 16) - (mid_x + 10);
    if (slider_w < 120) slider_w = 120;
    control_x = mid_x + 10;
    value_x = control_x;
    toggle_x = control_x + 50;
    (void)value_x;
    task_idx = model_ui_selected_task();

    if (pt_in_rect(mx, my, px + 10, py + 58, left_w - 8, 20)) {
        g_model_ui.selected_task = 0;
        if (!model_ui_task_dirty(0)) {
            model_ui_sync_task_from_kernel(0);
        }
        return 1;
    }
    if (pt_in_rect(mx, my, px + 10, py + 84, left_w - 8, 20)) {
        g_model_ui.selected_task = 1;
        if (!model_ui_task_dirty(1)) {
            model_ui_sync_task_from_kernel(1);
        }
        return 1;
    }

    if (pt_in_rect(mx, my, mid_x + 10, row1_y + 13, slider_w, 8)) {
        rel = ((mx - (mid_x + 10)) * 9) / (slider_w > 0 ? slider_w : 1);
        g_model_ui.lr_fast[task_idx] = wm_clamp_i32(rel + 1, 1, 10);
        model_ui_mark_dirty(task_idx);
        return 1;
    }
    if (pt_in_rect(mx, my, mid_x + 10, row2_y + 13, slider_w, 8)) {
        rel = ((mx - (mid_x + 10)) * 9) / (slider_w > 0 ? slider_w : 1);
        g_model_ui.lr_slow[task_idx] = wm_clamp_i32(rel + 1, 1, 10);
        model_ui_mark_dirty(task_idx);
        return 1;
    }
    if (pt_in_rect(mx, my, mid_x + 10, row3_y + 13, slider_w, 8)) {
        rel = ((mx - (mid_x + 10)) * 39) / (slider_w > 0 ? slider_w : 1);
        g_model_ui.refractory_ticks[task_idx] = wm_clamp_i32(rel + 1, 1, 40);
        model_ui_mark_dirty(task_idx);
        return 1;
    }
    if (pt_in_rect(mx, my, mid_x + 10, row4_y + 13, slider_w, 8)) {
        rel = ((mx - (mid_x + 10)) * 7) / (slider_w > 0 ? slider_w : 1);
        g_model_ui.tau[task_idx] = wm_clamp_i32(rel + 1, 1, 8);
        model_ui_mark_dirty(task_idx);
        return 1;
    }

    if (pt_in_rect(mx, my, toggle_x, row1_y + 24, 44, 14)) {
        g_model_ui.toggles[task_idx][0] ^= 1;
        model_ui_mark_dirty(task_idx);
        return 1;
    }
    if (pt_in_rect(mx, my, toggle_x, row2_y + 24, 44, 14)) {
        g_model_ui.toggles[task_idx][1] ^= 1;
        model_ui_mark_dirty(task_idx);
        return 1;
    }
    if (pt_in_rect(mx, my, toggle_x, row3_y + 24, 44, 14)) {
        g_model_ui.toggles[task_idx][2] ^= 1;
        model_ui_mark_dirty(task_idx);
        return 1;
    }
    if (pt_in_rect(mx, my, toggle_x, row4_y + 24, 44, 14)) {
        g_model_ui.toggles[task_idx][3] ^= 1;
        model_ui_mark_dirty(task_idx);
        return 1;
    }

    if (pt_in_rect(mx, my, px + pw - 172, py + ph - 66, 154, 24)) {
        model_ui_apply_to_kernel();
        return 1;
    }
    if (pt_in_rect(mx, my, px + pw - 172, py + ph - 36, 154, 22)) {
        model_ui_restore_task_baseline(task_idx);
        return 1;
    }

    return 0;
}

static int handle_model_manager_drag(WmWindow *w, int mx, int my) {
    int cx = w->x + WM_BORDER;
    int cy = w->y + WM_TITLEBAR_H;
    int cw = w->w - WM_BORDER * 2;
    int ch = w->h - WM_TITLEBAR_H - WM_BORDER;

    int px = cx + 6;
    int py = cy + 6;
    int pw = cw - 12;
    int left_w = 214;
    int right_w = 190;
    int mid_x;
    int mid_w;
    int slider_w;
    int graph_x;
    int graph_y;
    int task_idx;
    int row1_y;
    int row2_y;
    int row3_y;
    int row4_y;
    int rel;

    (void)ch;

    if (left_w + right_w + 36 > pw) {
        left_w = pw / 3;
        right_w = pw / 3;
    }
    mid_x = px + left_w + 14;
    mid_w = pw - left_w - right_w - 30;
    graph_x = mid_x + mid_w - 140;
    graph_y = py + 214;
    row1_y = py + 64;
    row2_y = row1_y + 48;
    row3_y = row2_y + 48;
    row4_y = graph_y + 8;
    slider_w = (graph_x - 16) - (mid_x + 10);
    if (slider_w < 120) slider_w = 120;
    task_idx = model_ui_selected_task();

    if (pt_in_rect(mx, my, mid_x + 10, row1_y + 13, slider_w, 8)) {
        rel = ((mx - (mid_x + 10)) * 9) / (slider_w > 0 ? slider_w : 1);
        g_model_ui.lr_fast[task_idx] = wm_clamp_i32(rel + 1, 1, 10);
        model_ui_mark_dirty(task_idx);
        return 1;
    }
    if (pt_in_rect(mx, my, mid_x + 10, row2_y + 13, slider_w, 8)) {
        rel = ((mx - (mid_x + 10)) * 9) / (slider_w > 0 ? slider_w : 1);
        g_model_ui.lr_slow[task_idx] = wm_clamp_i32(rel + 1, 1, 10);
        model_ui_mark_dirty(task_idx);
        return 1;
    }
    if (pt_in_rect(mx, my, mid_x + 10, row3_y + 13, slider_w, 8)) {
        rel = ((mx - (mid_x + 10)) * 39) / (slider_w > 0 ? slider_w : 1);
        g_model_ui.refractory_ticks[task_idx] = wm_clamp_i32(rel + 1, 1, 40);
        model_ui_mark_dirty(task_idx);
        return 1;
    }
    if (pt_in_rect(mx, my, mid_x + 10, row4_y + 13, slider_w, 8)) {
        rel = ((mx - (mid_x + 10)) * 7) / (slider_w > 0 ? slider_w : 1);
        g_model_ui.tau[task_idx] = wm_clamp_i32(rel + 1, 1, 8);
        model_ui_mark_dirty(task_idx);
        return 1;
    }
    return 0;
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

typedef struct {
    int initialized;
    uint32_t last_tick;
    int col;
    uint8_t raster[48][160];
    uint8_t heat[24][120];
    int trend_col;
    int trend_a[120];
    int trend_b[120];
    int avg_hz;
    int max_hz;
    int active_neurons;
    int sync_score;
    int sync_alert;
} WmSpikeMonitorState;

static WmSpikeMonitorState g_spike_mon = {0};

static void spike_monitor_init_if_needed(void) {
    int y, x;
    if (g_spike_mon.initialized) return;
    g_spike_mon.initialized = 1;
    g_spike_mon.last_tick = tick;
    g_spike_mon.col = 0;
    for (y = 0; y < 48; y++) {
        for (x = 0; x < 160; x++) {
            g_spike_mon.raster[y][x] = 0;
        }
    }
    for (y = 0; y < 24; y++) {
        for (x = 0; x < 120; x++) {
            g_spike_mon.heat[y][x] = 0;
        }
    }
    for (x = 0; x < 120; x++) {
        g_spike_mon.trend_a[x] = 0;
        g_spike_mon.trend_b[x] = 0;
    }
}

static void spike_monitor_update(void) {
    int dt;
    spike_monitor_init_if_needed();
    dt = (int)(tick - g_spike_mon.last_tick);
    if (dt <= 0) return;

    while (dt-- > 0) {
        int y;
        int xcol = g_spike_mon.col;
        int max_sps = 0;
        int active = 0;
        int n_idx, p_idx;

        int sps0 = wm_clamp_i32(task_list[0].spikes_per_second, 0, 600);
        int sps1 = wm_clamp_i32(task_list[1].spikes_per_second, 0, 600);
        int sps_mix = (sps0 + sps1) / 2;

        if (sps0 > max_sps) max_sps = sps0;
        if (sps1 > max_sps) max_sps = sps1;

        g_spike_mon.avg_hz = wm_clamp_i32(sps_mix, 0, 420);
        g_spike_mon.max_hz = wm_clamp_i32(max_sps, 0, 540);

        for (p_idx = 0; p_idx < 2; p_idx++) {
            for (n_idx = 0; n_idx < 5; n_idx++) {
                if (os_memory_map[p_idx].neurons[n_idx].voltage > 350) active++;
            }
        }
        g_spike_mon.active_neurons = active;

        for (y = 0; y < 48; y++) {
            int src = (y * 10) / 48; /* Map 48 vertical lines to the 10 real neurons */
            int p2_idx = src >= 5 ? 1 : 0;
            int n2_idx = src % 5;
            int v = os_memory_map[p2_idx].neurons[n2_idx].voltage;
            int thresh = os_memory_map[p2_idx].neurons[n2_idx].dynamic_threshold;
            int rf = os_memory_map[p2_idx].neurons[n2_idx].refractory_timer;
            
            /* ONLY real spikes from the kernel structures */
            int fire = (v >= thresh || rf > 6) ? 1 : 0; 
            
            g_spike_mon.raster[y][xcol] = (uint8_t)fire;
        }

        g_spike_mon.trend_a[g_spike_mon.trend_col] = g_spike_mon.avg_hz;
        g_spike_mon.trend_b[g_spike_mon.trend_col] = wm_clamp_i32(g_spike_mon.max_hz - 40, 0, 540);

        g_spike_mon.sync_score = wm_clamp_i32((g_spike_mon.active_neurons * 24) + (g_spike_mon.max_hz - g_spike_mon.avg_hz) / 2, 0, 240);
        g_spike_mon.sync_alert = (g_spike_mon.sync_score >= 120) ? 1 : 0;

        g_spike_mon.col = (g_spike_mon.col + 1) % 160;
        g_spike_mon.trend_col = (g_spike_mon.trend_col + 1) % 120;
        g_spike_mon.last_tick++;
    }
}

static uint32_t spike_phase_color(int v) {
    int x = wm_clamp_i32(v, 0, 255);
    if (x < 45) {
        int t = x;
        return (uint32_t)((3 + (t * 9) / 45) << 16) |
               (uint32_t)((22 + (t * 28) / 45) << 8) |
               (uint32_t)(44 + (t * 40) / 45);
    }
    if (x < 95) {
        int t = x - 45;
        return (uint32_t)((12 + (t * 16) / 50) << 16) |
               (uint32_t)((50 + (t * 64) / 50) << 8) |
               (uint32_t)(84 + (t * 64) / 50);
    }
    if (x < 145) {
        int t = x - 95;
        return (uint32_t)((28 + (t * 28) / 50) << 16) |
               (uint32_t)((114 + (t * 82) / 50) << 8) |
               (uint32_t)(146 + (t * 44) / 50);
    }
    if (x < 195) {
        int t = x - 145;
        return (uint32_t)((56 + (t * 42) / 50) << 16) |
               (uint32_t)((196 + (t * 44) / 50) << 8) |
               (uint32_t)(190 + (t * 24) / 50);
    }
    {
        int t = x - 195;
        return (uint32_t)((98 + (t * 54) / 60) << 16) |
               (uint32_t)((240 + (t * 15) / 60) << 8) |
               (uint32_t)(214 - (t * 24) / 60);
    }
}


static void draw_spike_soft_blob(int cx, int cy, uint32_t core, uint32_t ring) {
    int dx, dy;
    for (dy = -4; dy <= 4; dy++) {
        for (dx = -4; dx <= 4; dx++) {
            int d = dx * dx + dy * dy;
            if (d <= 4) put_pixel(cx + dx, cy + dy, core);
            else if (d <= 12) put_pixel(cx + dx, cy + dy, ring);
        }
    }
}

static void draw_content_spike_monitor(int x, int y, int w, int h) {
    int px = x + 6;
    int py = y + 6;
    int pw = w - 12;
    int ph = h - 12;
    int right_w = 220;
    int left_w = pw - right_w - 12;
    int t;

    int raster_x = px + 34;
    int raster_y = py + 38;
    int raster_w = left_w - 44;
    int raster_h = 160;

    int heat_x = raster_x;
    int heat_y = py + 238;
    int heat_w = raster_w;
    int heat_h = 126;

    int panel_x = px + left_w + 6;
    int panel_y = py + 22;
    int panel_w = right_w;
    int panel_h = ph - 28;

    spike_monitor_update();

    draw_filled_rect(px, py, pw, ph, 0x081122);
    draw_rect(px, py, pw, ph, 0x2D7C98);
    draw_hline(py + 18, px + 1, px + pw - 1, 0x2AA9C0);
    cursor_x = px + 8;
    cursor_y = py + 4;
    gprint("NEUROSPARK OS v0.7", 0x89DAF4);
    cursor_x = px + pw - 150;
    cursor_y = py + 4;
    gprint("SPIKE MONITOR APP", 0x86EAFF);

    draw_filled_rect(px + 4, py + 22, left_w, ph - 28, 0x05162B);
    draw_rect(px + 4, py + 22, left_w, ph - 28, 0x3AC2D9);

    cursor_x = raster_x + (raster_w / 2) - 68;
    cursor_y = raster_y - 14;
    gprint("Rolling Raster Plot", 0x9DEBFF);
    draw_filled_rect(raster_x, raster_y, raster_w, raster_h, 0x000A33);
    draw_rect(raster_x, raster_y, raster_w, raster_h, 0x2B6A86);
    for (t = 1; t < 8; t++) {
        draw_hline(raster_y + (t * raster_h) / 8, raster_x + 1, raster_x + raster_w - 1, 0x113350);
    }
    for (t = 0; t < 160; t++) {
        int xx = raster_x + 2 + (t * (raster_w - 4)) / 159;
        int ry;
        for (ry = 0; ry < 48; ry++) {
            int src_col = (g_spike_mon.col + t + (ry * 3)) % 160;
            if (g_spike_mon.raster[ry][src_col]) {
                int yy = raster_y + 2 + (ry * (raster_h - 4)) / 47;
                uint32_t core = (os_memory_map[(ry >= 24) ? 1 : 0].current_phase == 1) ? 0x9DFF7E : 0x88FF66;
                uint32_t ring = (os_memory_map[(ry >= 24) ? 1 : 0].current_phase == 1) ? 0x2EE0A4 : 0x2CB86E;
                draw_spike_soft_blob(xx, yy, core, ring);
            }
        }
    }

    {
        const char *yv[5] = {"2000", "1500", "1000", "500", "0"};
        int yi;
        for (yi = 0; yi < 5; yi++) {
            cursor_x = raster_x - 30;
            cursor_y = raster_y + 1 + (yi * (raster_h - 12)) / 4;
            gprint((char *)yv[yi], 0x9ED6E8);
        }
        cursor_x = raster_x - 32;
        cursor_y = raster_y + (raster_h / 2);
        gprint("Neuron ID", 0x9ED6E8);
    }

    cursor_x = raster_x + raster_w - 34;
    cursor_y = raster_y + raster_h + 4;
    gprint("X", 0x80D7EF);

    cursor_x = heat_x + (heat_w / 2) - 110;
    cursor_y = heat_y - 14;
    gprint("Activity Density - PHASE HEATMAP", 0x69D9EE);
    draw_filled_rect(heat_x, heat_y, heat_w, heat_h, 0x02152A);
    draw_rect(heat_x, heat_y, heat_w, heat_h, 0x2B6A86);
    {
        int iw = heat_w - 2;
        int ih = heat_h - 2;
        int xx;
        int yy;
        for (yy = 0; yy < ih; yy++) {
            for (xx = 0; xx < iw; xx++) {
                int p;
                int v = 10;

                for (p = 0; p < 2; p++) {
                    int n;
                    WmNeuralPixel *pxp = &os_memory_map[p];
                    int phase_boost = pxp->current_phase ? 25 : 0;
                    int recent_boost = wm_clamp_i32(pxp->pixel_recent_spikes * 3, 0, 50);

                    for (n = 0; n < 5; n++) {
                        /* Arrange the 10 neurons evenly across the entire heatmap area */
                        int cx = (iw / 10) + (n * (iw / 5));
                        int cy = (ih / 4) + (p * (ih / 2));
                        
                        int dx = xx - cx;
                        int dy = yy - cy;
                        int dist2 = (dx * dx) + (dy * dy) * 2; /* Stretch horizontally a bit */
                        
                        int voltage = pxp->neurons[n].voltage;
                        int thresh = pxp->neurons[n].dynamic_threshold;
                        
                        /* Calculate intensity based on how close to threshold it is */
                        int base_e = (voltage * 255) / (thresh > 1 ? thresh : 100);
                        int energy = wm_clamp_i32(base_e + phase_boost + recent_boost, 0, 500);
                        
                        /* Smooth inverse-square falloff for a continuous glowing topological map */
                        int contrib = (energy * 1500) / (1500 + dist2);
                        v += contrib;
                    }
                }

                /* Boost overall intensity and map into the cyan/green/yellow spectrum */
                v = wm_clamp_i32(v, 0, 255);
                put_pixel(heat_x + 1 + xx, heat_y + 1 + yy, spike_phase_color(v));
            }
        }
    }

    draw_filled_rect(panel_x, panel_y, panel_w, panel_h, 0x092033);
    draw_rect(panel_x, panel_y, panel_w, panel_h, 0x3AC2D9);

    draw_model_health_gauge(panel_x + 10, panel_y + 16, panel_w - 70,
                            "Avg Firing Rate", wm_clamp_i32((g_spike_mon.avg_hz * 100) / 420, 0, 100), 0x5AEBD1);
    cursor_x = panel_x + panel_w - 88;
    cursor_y = panel_y + 18;
    gprint_dec(g_spike_mon.avg_hz, 0xCFFFFF);
    gprint(" Hz", 0x9ED6E8);

    draw_model_health_gauge(panel_x + 10, panel_y + 52, panel_w - 70,
                            "Max Spike Load", wm_clamp_i32((g_spike_mon.max_hz * 100) / 540, 0, 100), 0x80E85A);

        draw_filled_rect(panel_x + 10, panel_y + 88, panel_w - 20, 26,
                g_spike_mon.sync_alert ? 0x3A2A17 : 0x17342A);
        draw_rect(panel_x + 10, panel_y + 88, panel_w - 20, 26,
            g_spike_mon.sync_alert ? 0xD19145 : 0x5ACB96);
    cursor_x = panel_x + 20;
    cursor_y = panel_y + 96;
        gprint(g_spike_mon.sync_alert ? "**HIGH SYNCHRONY WARNING**" : "Synchrony: nominal",
            g_spike_mon.sync_alert ? 0xFFD89E : 0xB9FFD9);

    draw_model_health_gauge(panel_x + 10, panel_y + 126, panel_w - 70,
                            "Active Neurons", wm_clamp_i32((g_spike_mon.active_neurons * 100) / 10, 0, 100), 0x5ACDFF);
    cursor_x = panel_x + panel_w - 84;
    cursor_y = panel_y + 128;
    gprint_dec(g_spike_mon.active_neurons, 0xCFFFFF);
    gprint("/10", 0x9ED6E8);

    draw_filled_rect(panel_x + 10, panel_y + 164, panel_w - 20, 104, 0x06182A);
    draw_rect(panel_x + 10, panel_y + 164, panel_w - 20, 104, 0x2F799A);
    cursor_x = panel_x + 16;
    cursor_y = panel_y + 170;
    gprint("Trending", 0xAEEBFF);
    cursor_x = panel_x + panel_w - 88;
    cursor_y = panel_y + 170;
    gprint("Past 120", 0x8FB8CC);

    {
        int sx = panel_x + 12;
        int sy = panel_y + 186;
        int sw = panel_w - 24;
        int sh = 74;
        int i;
        int px0 = sx + 1;
        int py0 = sy + sh - 2;
        draw_rect(sx, sy, sw, sh, 0x255E79);
        for (i = 1; i < 4; i++) {
            draw_hline(sy + (i * sh) / 4, sx + 1, sx + sw - 1, 0x14384F);
        }
        for (i = 0; i < 120; i++) {
            int idx = (g_spike_mon.trend_col + i) % 120;
            int v = wm_clamp_i32(g_spike_mon.trend_a[idx], 0, 540);
            int xx = sx + 1 + (i * (sw - 3)) / 119;
            int yy = sy + sh - 2 - (v * (sh - 6)) / 540;
            draw_line_segment(px0, py0, xx, yy, 0x7DFF85);
            px0 = xx;
            py0 = yy;
        }
        px0 = sx + 1;
        py0 = sy + sh - 2;
        for (i = 0; i < 120; i++) {
            int idx = (g_spike_mon.trend_col + i) % 120;
            int v = wm_clamp_i32(g_spike_mon.trend_b[idx], 0, 540);
            int xx = sx + 1 + (i * (sw - 3)) / 119;
            int yy = sy + sh - 2 - (v * (sh - 6)) / 540;
            draw_line_segment(px0, py0, xx, yy, 0xA7D9FF);
            px0 = xx;
            py0 = yy;
        }
    }
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

typedef struct {
    uint32_t last_tick;
    int scroll_offset;
    int live_trace[200];
    int base_trace[200];
    int snapshots_count;
    char snap_tags[8][16];
    int selected_snapshot;
    int diff_view_idx;
    int active_btn_pulse;
    int initialized;
    int diff_sliders[4];
    int param_scroll;
    char status_msg[64];
} WmSnapshotBrowserState;

static WmSnapshotBrowserState g_snap_browser = {0};

static void snapshot_browser_update(void) {
    int dt = (int)(tick - g_snap_browser.last_tick);
    if (dt <= 0) return;

    if (!g_snap_browser.initialized) {
        g_snap_browser.selected_snapshot = -1;
        int _i; for(_i=0;_i<4;_i++) g_snap_browser.diff_sliders[_i] = (_i*20)%100;
        g_snap_browser.param_scroll = 0;
        wm_strcpy(g_snap_browser.status_msg, "System Ready.", 63);
        g_snap_browser.initialized = 1;
    }

    g_snap_browser.snapshots_count = storage_snapshot_used_count();
    
    int i;
    for (i = 0; i < 8 && i < g_snap_browser.snapshots_count; i++) {
        storage_get_snapshot_tag(i, g_snap_browser.snap_tags[i], 15);
    }

    while (dt-- > 0) {
        int idx = g_snap_browser.scroll_offset % 200;
        
        int live_volt = 0;
        for (i = 0; i < 5; i++) {
            live_volt += os_memory_map[0].neurons[i].voltage;
        }
        live_volt /= 5;
        
        int base_volt = 0;
        for (i = 0; i < 5; i++) {
            base_volt += task_list[0].saved_voltages[i];
        }
        base_volt /= 5;

        g_snap_browser.live_trace[idx] = wm_clamp_i32(live_volt, 0, 2000);
        g_snap_browser.base_trace[idx] = wm_clamp_i32(base_volt, 0, 2000);

        g_snap_browser.scroll_offset = (g_snap_browser.scroll_offset + 1) % 200;
        g_snap_browser.last_tick++;
    }
}

static void handle_snapshot_browser_drag(WmWindow *win, int mx, int my) {
    int px = win->x + 6;
    int py = win->y + 6;
    int pw = win->w - 12;
    int left_w = 420;
    int right_w = pw - left_w - 6;

    int p2_x = px + left_w + 6;
    int p2_y = py + 22;
    int g_w = right_w - 220;
    int rp_x = p2_x + 6 + g_w + 6;
    int rp_y = p2_y + 24;
    int rp_w = right_w - g_w - 18;
    int i;

    /* 1. Diff View options */
    for (i = 0; i < 4; i++) {
        int opt_y = rp_y + 24 + i*13;
        if (mx >= rp_x && mx <= rp_x + 60 && my >= opt_y - 4 && my <= opt_y + 12) {
            int track_x = rp_x + 6;
            int track_w = 28;
            int val = ((mx - track_x) * 100) / track_w;
            if (val < 0) val = 0;
            if (val > 100) val = 100;
            g_snap_browser.diff_sliders[i] = val;
            return;
        }
    }

    /* 2. Metadata/Params Scrollbar */
    int cp_y = rp_y + 86;
    if (mx >= rp_x + rp_w - 15 && mx <= rp_x + rp_w && my >= cp_y + 10 && my <= cp_y + 86) {
        int t_y = cp_y + 14;
        int t_h = 30;
        int val = ((my - t_y - 15) * 100) / t_h;
        if (val < 0) val = 0;
        if (val > 100) val = 100;
        g_snap_browser.param_scroll = val;
        return;
    }
}

static void handle_snapshot_browser_click(WmWindow *win, int mx, int my) {
    int px = win->x + 6;
    int py = win->y + 6;
    int pw = win->w - 12;
    int ph = win->h - 12;
    int left_w = 420;
    int right_w = pw - left_w - 6;

    int p1_x = px;
    int p1_y = py + 22;
    
    int p2_x = px + left_w + 6;
    int p2_y = p1_y;
    int p2_h = ph - 28;

    /* Snapshots List Items */
    int lst_y = p1_y + 74;
    int i;
    for (i = 0; i < g_snap_browser.snapshots_count; i++) {
        int row_y = lst_y - 2;
        if (mx >= p1_x + 22 && mx <= p1_x + 22 + left_w - 90 &&
            my >= row_y && my <= row_y + 12) {
            g_snap_browser.selected_snapshot = i;
            return;
        }
        lst_y += 14;
    }

    /* Diff View options */
    int g_w = right_w - 220;
    int rp_x = p2_x + 6 + g_w + 6;
    int rp_y = p2_y + 24;
    for (i = 0; i < 4; i++) {
        int opt_y = rp_y + 24 + i*13;
        if (mx >= rp_x && mx <= rp_x + right_w - g_w - 18 &&
            my >= opt_y && my <= opt_y + 12) {
            g_snap_browser.diff_view_idx = i;
            return;
        }
    }

    /* Bottom Action Buttons */
    int by = p2_y + p2_h - 44;
    int bw = (right_w - 30) / 3;

    if (mx >= p2_x + 10 && mx <= p2_x + 10 + bw && my >= by && my <= by + 32) {
        g_snap_browser.active_btn_pulse = 1;
        wm_strcpy(g_snap_browser.status_msg, "DIFF SELECTION Triggered!", 63);
        return;
    }
    if (mx >= p2_x + 10 + bw + 5 && mx <= p2_x + 10 + bw + 5 + bw && my >= by && my <= by + 32) {
        g_snap_browser.active_btn_pulse = 2;
        wm_strcpy(g_snap_browser.status_msg, "TAG MENU Displayed!", 63);
        return;
    }
    if (mx >= p2_x + 10 + bw*2 + 10 && mx <= p2_x + 10 + bw*2 + 10 + bw && my >= by && my <= by + 32) {
        g_snap_browser.active_btn_pulse = 3;
        wm_strcpy(g_snap_browser.status_msg, "DATASET EXPORTED successfully!", 63);
        return;
    }
}

static void draw_content_snapshot_browser(int x, int y, int w, int h) {
    int px = x + 6;
    int py = y + 6;
    int pw = w - 12;
    int ph = h - 12;
    int left_w = 420;
    int right_w = pw - left_w - 6;

    int p1_x = px;
    int p1_y = py + 22;
    int p1_h = ph - 28;
    
    int p2_x = px + left_w + 6;
    int p2_y = p1_y;
    int p2_h = p1_h;

    draw_filled_rect(px, py, pw, ph, 0x081122);
    draw_rect(px, py, pw, ph, 0x2D7C98);
    draw_hline(py + 18, px + 1, px + pw - 1, 0x2AA9C0);
    
    cursor_x = px + 8; cursor_y = py + 4;
    gprint("NEUROSPARK OS v0.7", 0x89DAF4);
    cursor_x = px + pw - 146; cursor_y = py + 4;
    gprint("SNAPSHOT BROWSER", 0x86EAFF);

    draw_filled_rect(p1_x, p1_y, left_w, p1_h, 0x05162B);
    draw_rect(p1_x, p1_y, left_w, p1_h, 0x3AC2D9);
    cursor_x = p1_x + (left_w / 2) - 86; cursor_y = p1_y + 4;
    gprint("SNAPSHOTS & REPLAYS - VFS", 0x9DEBFF);
    draw_hline(p1_y + 20, p1_x + 1, p1_x + left_w - 1, 0x113350);
    
    cursor_x = p1_x + 8; cursor_y = p1_y + 24; gprint("Name", 0x4D8E9E);
    draw_vline(p1_x + left_w - 80, p1_y + 20, p1_y + p1_h - 1, 0x113350);
    cursor_x = p1_x + left_w - 76; cursor_y = p1_y + 24; gprint("Metadata", 0x4D8E9E);
    draw_hline(p1_y + 40, p1_x + 1, p1_x + left_w - 1, 0x113350);

    snapshot_browser_update();

    int lst_y = p1_y + 46;
    int i;
    cursor_x=p1_x+8; cursor_y=lst_y; gprint("> [~] SYSTEM_RAM_MOUNT", 0xDEFFFF);
    cursor_x=p1_x+left_w-76; cursor_y=lst_y; gprint("(SYS_DEF)", 0x8FB8CC); lst_y += 14;

    cursor_x=p1_x+8; cursor_y=lst_y; gprint("v [D] SNAPSHOT_VFS", 0xDEFFFF);
    cursor_x=p1_x+left_w-76; cursor_y=lst_y; gprint("(MNT)", 0x8FB8CC); lst_y += 14;

    if (g_snap_browser.snapshots_count == 0) {
        cursor_x=p1_x+24; cursor_y=lst_y; gprint("... [No active snapshots]", 0x557788);
    } else {
        for(i = 0; i < g_snap_browser.snapshots_count; i++) {
            if (i == g_snap_browser.selected_snapshot) {
                draw_filled_rect(p1_x + 22, lst_y - 2, left_w - 90, 12, 0x1A445C);
            }
            cursor_x=p1_x+24; cursor_y=lst_y; 
            gprint("[S] Slot ", (i == g_snap_browser.selected_snapshot) ? 0xFFFFFF : 0x8FB8CC); 
            gprint_dec(i, (i == g_snap_browser.selected_snapshot) ? 0xFFFFFF : 0x8FB8CC);
            gprint(" -> ", 0x8FB8CC); gprint(g_snap_browser.snap_tags[i], (i == g_snap_browser.selected_snapshot) ? 0xFFE082 : 0xA7E0EB);
            cursor_x=p1_x+left_w-76; cursor_y=lst_y; gprint("#saved...", 0x7CA6BD); lst_y += 14;
        }
    }

    draw_filled_rect(p1_x + left_w - 6, p1_y + 42, 6, (g_snap_browser.snapshots_count > 0 ? 30*g_snap_browser.snapshots_count : 20), 0x2EE0A4);

    draw_filled_rect(p2_x, p2_y, right_w, p2_h, 0x05162B);
    draw_rect(p2_x, p2_y, right_w, p2_h, 0x3AC2D9);
    cursor_x = p2_x + (right_w / 2) - 100; cursor_y = p2_y + 4;
    gprint("COMPARISON SURFACE - AXIS", 0x9DEBFF);
    draw_hline(p2_y + 20, p2_x + 1, p2_x + right_w - 1, 0x113350);

    int g_x = p2_x + 6;
    int g_y = p2_y + 26;
    int g_w = right_w - 220;
    int g_h = p2_h - 75;
    
    cursor_x = g_x; cursor_y = g_y - 4; gprint("User Summary", 0xDEFFFF);
    draw_filled_rect(g_x + 46, g_y + 16, g_w - 46, g_h - 16, 0x02152A);
    draw_rect(g_x + 46, g_y + 16, g_w - 46, g_h - 16, 0x2B6A86);

    cursor_x = g_x + 50; cursor_y = g_y + 20; gprint("Potentials", 0x5ACB96);
    draw_line_segment(g_x + g_w - 120, g_y + 24, g_x + g_w - 100, g_y + 24, 0xFFE082);
    cursor_x = g_x + g_w - 94; cursor_y = g_y + 20; gprint("Compared State", 0xFFE082);
    
    for(i=0; i<10; i++) put_pixel(g_x + g_w - 120 + i*2, g_y + 36, 0x9DEBFF);
    cursor_x = g_x + g_w - 94; cursor_y = g_y + 32; gprint("Baseline State", 0x9DEBFF);

    const char *y_lbl[] = {"+40mV", "+20mV", "  0mV", "-20mV", "-40mV", "-60mV", "-80mV"};
    for (i = 0; i < 7; i++) {
        int ly = g_y + 36 + i * ((g_h - 36) / 6);
        cursor_x = g_x; cursor_y = ly - 4; gprint((char *)y_lbl[i], i==2 ? 0xFFFFFF : 0xDEFFFF);
        draw_hline(ly, g_x + 47, g_x + g_w - 2, 0x14384F);
    }
    for (i = 1; i < 10; i++) {
        int lx = g_x + 46 + i * ((g_w - 46) / 10);
        draw_vline(lx, g_y + 17, g_y + g_h - 1, 0x14384F);
    }
    
    int px_base = g_x + 47;
    int py1_base = g_y + 36 + ((g_h - 36) / 6);
    int py2_base = g_y + 36 + 3 * ((g_h - 36) / 6);
    int py3_base = g_y + 36 + 4 * ((g_h - 36) / 6);
    int py4_base = g_y + 36 + 5 * ((g_h - 36) / 6);
    int prev1_x = px_base, prev1_y = py1_base;
    
    int prev2_x = px_base, prev2_y = py1_base;
    int prev3_x = px_base, prev3_y = py2_base;
    int prev4_x = px_base, prev4_y = py3_base;
    int prev5_x = px_base, prev5_y = py4_base;

    for (i = 1; i < (g_w - 48); i++) {
        int idx = (g_snap_browser.scroll_offset + (i * 200) / (g_w - 48)) % 200;
        int live_y = py1_base - (g_snap_browser.live_trace[idx] / 15);
        int base_y = py1_base - (g_snap_browser.base_trace[idx] / 15);

        int env2 = (i > 140 && i < 180) ? ((180 - i)*(i - 140)/8) : 0;
        int noise1 = (i*13 % 17) - 8;
        int noise2 = (i*7 % 11) - 5;
        
        int n_y3 = py2_base + (noise1 * env2 / 24) + (noise1 / 2);
        int n_y4 = py3_base + (noise2 * env2 / 22) + (noise2 / 2);
        int n_y5 = py4_base + ((noise1+noise2) * env2 / 30);
        
        /* The main "Potentials" line follows actual memory mapping now */
        draw_line_segment(prev1_x, prev1_y, px_base + i, live_y, 0xFFE082);
        
        draw_line_segment(prev3_x, prev3_y, px_base + i, n_y3, 0xFFE082);
        draw_line_segment(prev4_x, prev4_y, px_base + i, n_y4, 0xFFE082);
        draw_line_segment(prev5_x, prev5_y, px_base + i, n_y5, 0xFFE082);
        
        /* Baseline State is dashed overlay following the saved snapshot trace */
        if (i % 3 != 0) {
            put_pixel(px_base + i, base_y, 0x9DEBFF);
        }
        
        prev1_x = px_base + i; prev1_y = live_y;
        prev2_x = px_base + i; prev2_y = base_y;
        prev3_x = px_base + i; prev3_y = n_y3;
        prev4_x = px_base + i; prev4_y = n_y4;
        prev5_x = px_base + i; prev5_y = n_y5;
    }

    int rp_x = g_x + g_w + 6;
    int rp_y = p2_y + 24;
    int rp_w = right_w - g_w - 18;

    cursor_x = rp_x; cursor_y = rp_y - 2; gprint("Diff View", 0xDEFFFF);
    draw_rect(rp_x, rp_y + 12, rp_w, 64, 0x2B6A86);
    for (i = 0; i < 4; i++) {
        int ly = rp_y + 24 + i*13;
        draw_hline(ly, rp_x + 6, rp_x + 40, 0x3AC2D9);
        int sx = rp_x + 6 + (g_snap_browser.diff_sliders[i] * 28) / 100;
        if(sx < rp_x + 6) sx = rp_x + 6;
        if(sx > rp_x + 34) sx = rp_x + 34;
        draw_filled_rect(sx, ly - 3, 6, 6, (i == g_snap_browser.diff_view_idx) ? 0xFFFFFF : 0x5AEBD1);
    }
    cursor_x = rp_x + 48; cursor_y = rp_y + 18; gprint("Diffs View", (g_snap_browser.diff_view_idx == 0) ? 0xFFFFFF : 0x8FB8CC);
    cursor_x = rp_x + 48; cursor_y = rp_y + 31; gprint("L:tine line", (g_snap_browser.diff_view_idx == 1) ? 0xFFFFFF : 0x8FB8CC);
    cursor_x = rp_x + 48; cursor_y = rp_y + 44; gprint("Best View", (g_snap_browser.diff_view_idx == 2) ? 0xFFFFFF : 0x8FB8CC);
    cursor_x = rp_x + 48; cursor_y = rp_y + 57; gprint("Mix View", (g_snap_browser.diff_view_idx == 3) ? 0xFFFFFF : 0x8FB8CC);

    int cp_y = rp_y + 86;
    cursor_x = rp_x; cursor_y = cp_y - 2; gprint("Creation Parameters", 0xDEFFFF);
    draw_rect(rp_x, cp_y + 12, rp_w, 74, 0x2B6A86);
    cursor_x = rp_x+4; cursor_y = cp_y+16; gprint("Parameters:", 0x8FB8CC);
    cursor_x = rp_x+4; cursor_y = cp_y+28; gprint("clamp_alpha_20s", 0x8FB8CC);
    cursor_x = rp_x+4; cursor_y = cp_y+40; gprint("stdp_learn_ra=0.05", 0x8FB8CC);
    cursor_x = rp_x+4; cursor_y = cp_y+52; gprint("stdp_learn_ra=0.05", 0x8FB8CC);
    cursor_x = rp_x+4; cursor_y = cp_y+64; gprint("stdp_learn_ra=0.05", 0x8FB8CC);
    int sy = cp_y + 14 + (g_snap_browser.param_scroll * 30) / 100;
    if(sy < cp_y + 14) sy = cp_y + 14;
    if(sy > cp_y + 44) sy = cp_y + 44;
    draw_filled_rect(rp_x + rp_w - 6, sy, 4, 30, 0x5AEBD1);

    int md_y = cp_y + 96;
    cursor_x = rp_x; cursor_y = md_y - 2; gprint("Metadata", 0xDEFFFF);
    draw_rect(rp_x, md_y + 12, rp_w, 64, 0x2B6A86);
    cursor_x = rp_x+4; cursor_y = md_y+16; gprint("Name  alpha_...", 0x8FB8CC);
    cursor_x = rp_x+4; cursor_y = md_y+28; gprint("Tags  #stdp", 0x8FB8CC);
    cursor_x = rp_x+4; cursor_y = md_y+40; gprint("T-Id  #phase_alpha", 0x8FB8CC);
    cursor_x = rp_x+4; cursor_y = md_y+52; gprint("Time  1:18:02", 0x8FB8CC);
    cursor_x = rp_x+4; cursor_y = md_y+64; gprint("Param_rate 300", 0x8FB8CC);

    int bw = (right_w - 30) / 3;
    int by = p2_y + p2_h - 44;
    
    int btn1_c = (g_snap_browser.active_btn_pulse == 1) ? 0xFFFFFF : 0xD19145;
    draw_rect(p2_x + 10, by, bw, 32, btn1_c);
    draw_rect(p2_x + 11, by+1, bw-2, 30, btn1_c);
    cursor_x = p2_x + 10 + (bw/2) - 52; cursor_y = by + 12; gprint("DIFF SELECTION", (g_snap_browser.active_btn_pulse == 1) ? 0xFFFFFF : 0xFFD89E);
    for(i=0; i<30; i++) put_pixel(p2_x + 10 + ((i*37)%bw), by + ((i*17)%32), btn1_c);

    int btn2_c = (g_snap_browser.active_btn_pulse == 2) ? 0xFFFFFF : 0xD19145;
    draw_rect(p2_x + 10 + bw + 5, by, bw, 32, btn2_c);
    draw_rect(p2_x + 11 + bw + 5, by+1, bw-2, 30, btn2_c);
    cursor_x = p2_x + 10 + bw + 5 + (bw/2) - 48; cursor_y = by + 12; gprint("TAG SELECTION", (g_snap_browser.active_btn_pulse == 2) ? 0xFFFFFF : 0xFFD89E);
    for(i=0; i<15; i++) put_pixel(p2_x + 10 + bw + 5 + ((i*29)%bw), by + ((i*7)%32), btn2_c);

    int btn3_c = (g_snap_browser.active_btn_pulse == 3) ? 0xFFFFFF : 0xD19145;
    draw_rect(p2_x + 10 + bw*2 + 10, by, bw, 32, btn3_c);
    draw_rect(p2_x + 11 + bw*2 + 10, by+1, bw-2, 30, btn3_c);
    cursor_x = p2_x + 10 + bw*2 + 10 + (bw/2) - 52; cursor_y = by + 12; gprint("EXPORT DATASET", (g_snap_browser.active_btn_pulse == 3) ? 0xFFFFFF : 0xFFD89E);
    for(i=0; i<15; i++) put_pixel(p2_x + 10 + bw*2 + 10 + ((i*31)%bw), by + ((i*11)%32), btn3_c);

    cursor_x = p2_x + 10; cursor_y = by - 12;
    gprint("OS MESSAGE: ", 0x4AA0C7);
    gprint(g_snap_browser.status_msg, 0xDEFFFF);

    if (g_snap_browser.active_btn_pulse > 0) g_snap_browser.active_btn_pulse = 0;
}

/* ------------------------------------------------------------ */
/*  Initialisation                                               */
/* ------------------------------------------------------------ */
static void draw_content_replay_control(int x, int y, int w, int h);
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
    wm_add_window(30, 34, 760, 450, "Spike Monitor App", draw_content_spike_monitor, 0, -1);
    wm_add_window(20, 26, 1000, 420, "Snapshot Browser App", draw_content_snapshot_browser, 0, -1);
    wm_add_window(20, 26, 760, 500, "Model Manager App", draw_content_model_manager, 0, -1);
    wm_add_window(150, 150, screen_w, 360, "Replay Control App", draw_content_replay_control, 0, -1);
    model_ui_sync_from_kernel();
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
        draw_hline(y, 0, screen_w, row_color);
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
    /* 6 icons, 80px each, centered in screen_wpx */
    int start_x = (screen_w - WM_ICON_SLOTS * 80) / 2;
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

void wm_open_model_manager(void) {
    int j;
    for (j = 0; j < wm_window_count; j++) {
        if (wm_str_eq(wm_windows[j].title, "Model Manager App")) {
            wm_windows[j].x = 20;
            wm_windows[j].y = 26;
            wm_windows[j].w = 760;
            wm_windows[j].h = 500;
            wm_windows[j].visible = 1;
            wm_windows[j].state = WM_STATE_NORMAL;
            bring_to_front(j);
            return;
        }
    }
}

void wm_open_spike_monitor(void) {
    int j;
    for (j = 0; j < wm_window_count; j++) {
        if (wm_str_eq(wm_windows[j].title, "Spike Monitor App")) {
            wm_windows[j].x = 30;
            wm_windows[j].y = 34;
            wm_windows[j].w = 760;
            wm_windows[j].h = 450;
            wm_windows[j].visible = 1;
            wm_windows[j].state = WM_STATE_NORMAL;
            bring_to_front(j);
            return;
        }
    }
}

void wm_open_snapshot_browser(void) {
    int j;
    for (j = 0; j < wm_window_count; j++) {
        if (wm_str_eq(wm_windows[j].title, "Snapshot Browser App")) {
            wm_windows[j].x = 20;
            wm_windows[j].y = 26;
            wm_windows[j].w = 1000;
            wm_windows[j].h = 420;
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
    int panel_x = (screen_w - WM_ICON_SLOTS * 80) / 2 - 10;
    int panel_w = WM_ICON_SLOTS * 80 + 20;

    /* Keep a subtle footer strip but float glass elements over it. */
    draw_filled_rect(0, WM_TASKBAR_Y, screen_w, WM_TASKBAR_H, 0x071421);
    draw_hline(WM_TASKBAR_Y, 0, screen_w, 0x123647);

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
void handle_replay_control_click_idx(int win_idx, int mx, int my);
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
                if (w->x + w->w > screen_w) w->x = screen_w - w->w;
                if (w->y + w->h > WM_TASKBAR_Y) w->y = WM_TASKBAR_Y - w->h;
            }
            if (released) {
                w->dragging = 0;
            }
            return; /* Don't process other clicks while dragging */
        }
    }

    if (!clicked) {
        if (held && wm_focused >= 0 && wm_focused < wm_window_count) {
            WmWindow *fw = &wm_windows[wm_focused];
            if (fw->visible && wm_str_eq(fw->title, "Model Manager App") &&
                pt_in_rect(mx, my, fw->x, fw->y, fw->w, fw->h)) {
                (void)handle_model_manager_drag(fw, mx, my);
            }
            if (fw->visible && wm_str_eq(fw->title, "Snapshot Browser App") &&
                pt_in_rect(mx, my, fw->x, fw->y, fw->w, fw->h)) {
                handle_snapshot_browser_drag(fw, mx, my);
            }
        }
        return;
    }

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
                w->w = screen_w; w->h = WM_TASKBAR_Y;
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

        if (wm_str_eq(w->title, "Model Manager App")) {
            (void)handle_model_manager_click(w, mx, my);
            return;
        }

        if (wm_str_eq(w->title, "Snapshot Browser App")) {
            handle_snapshot_browser_click(w, mx, my);
            return;
        }

        if (wm_str_eq(w->title, "Replay Control App")) {
            handle_replay_control_click_idx(idx, mx, my);
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
            int cx;
            int cy;
            int cw;
            int ch;

            if (wm_str_eq(w->title, "Model Manager App")) {
                cx = w->x + 2;
                cy = w->y + 22;
                cw = w->w - 4;
                ch = w->h - 24;
            } else {
                cx = w->x + WM_BORDER;
                cy = w->y + WM_TITLEBAR_H;
                cw = w->w - WM_BORDER * 2;
                ch = w->h - WM_TITLEBAR_H - WM_BORDER;
            }
            w->draw_fn(cx, cy, cw, ch);
        }
    }

    /* 3. Taskbar on top */
    draw_taskbar();

    /* 4. NeuroDock launcher overlay */
    launcher_render();

    /* 5. Mouse cursor on very top */
    wm_draw_cursor();

    wm_frames_total++;
    if (wm_last_fps_tick == 0) {
        wm_last_fps_tick = tick;
        wm_last_fps_frames = wm_frames_total;
    } else {
        uint32_t dt = tick - wm_last_fps_tick;
        if (dt >= 20u) {
            uint32_t df = wm_frames_total - wm_last_fps_frames;
            wm_fps_x10 = (df * 1000u) / dt;
            wm_last_fps_tick = tick;
            wm_last_fps_frames = wm_frames_total;
        }
    }
}

void wm_get_runtime_metrics(uint32_t *frames_total, uint32_t *fps_x10) {
    if (frames_total != 0) {
        *frames_total = wm_frames_total;
    }
    if (fps_x10 != 0) {
        *fps_x10 = wm_fps_x10;
    }
}

typedef struct {
    int playing;
    int current_frame;
    int last_cmd_executed;
} WmReplayState;
WmReplayState replay_state = {0, 0, -1};

extern int shell_is_recording_replay(void);
extern char replay_cmds[64][32];
extern int replay_count;

static void draw_hex_button(int x, int y, int w, int h, const char *txt1, const char *txt2, uint32_t border_col, uint32_t fill_col) {
    int bevel = 8;
    draw_filled_rect(x + bevel, y, w - bevel*2, h, fill_col);
    draw_filled_rect(x, y + bevel, w, h - bevel*2, fill_col);

    draw_line_segment(x + bevel, y, x + w - bevel, y, border_col);
    draw_line_segment(x + bevel, y + h, x + w - bevel, y + h, border_col);
    draw_line_segment(x, y + bevel, x, y + h - bevel, border_col);
    draw_line_segment(x + w, y + bevel, x + w, y + h - bevel, border_col);

    draw_line_segment(x, y + bevel, x + bevel, y, border_col);
    draw_line_segment(x + w - bevel, y, x + w, y + bevel, border_col);
    draw_line_segment(x, y + h - bevel, x + bevel, y + h, border_col);
    draw_line_segment(x + w - bevel, y + h, x + w, y + h - bevel, border_col);
    
    if (txt1) {
        int sl = 0; while (txt1[sl]) ++sl;
        cursor_x = x + (w - sl * 8) / 2;
        cursor_y = y + h/2 - 12;
        gprint((char *)txt1, 0x4EEBFE);
    }
    if (txt2) {
        int sl = 0; while (txt2[sl]) ++sl;
        cursor_x = x + (w - sl * 8) / 2;
        cursor_y = y + h/2 + 2;
        gprint((char *)txt2, 0xCCCCCC);
    }
}

static void draw_hex_button_orange(int x, int y, int w, int h, const char *txt1, const char *txt2, uint32_t border_col, uint32_t fill_col, int dot) {
    int bevel = 8;
    draw_filled_rect(x + bevel, y, w - bevel*2, h, fill_col);
    draw_filled_rect(x, y + bevel, w, h - bevel*2, fill_col);

    draw_line_segment(x + bevel, y, x + w - bevel, y, border_col);
    draw_line_segment(x + bevel, y + h, x + w - bevel, y + h, border_col);
    draw_line_segment(x, y + bevel, x, y + h - bevel, border_col);
    draw_line_segment(x + w, y + bevel, x + w, y + h - bevel, border_col);

    draw_line_segment(x, y + bevel, x + bevel, y, border_col);
    draw_line_segment(x + w - bevel, y, x + w, y + bevel, border_col);
    draw_line_segment(x, y + h - bevel, x + bevel, y + h, border_col);
    draw_line_segment(x + w - bevel, y + h, x + w, y + h - bevel, border_col);
    
    if (dot) {
        draw_filled_rect(x + w - 24, y + 6, 6, 6, shell_is_recording_replay() ? 0xFF0000 : 0xFF9F3B);
        cursor_x = x + w - 16; cursor_y = y + 5; gprint((char *)"REC", shell_is_recording_replay() ? 0xFF0000 : 0xFF9F3B);
    }

    if (txt1) {
        int sl = 0; while (txt1[sl]) ++sl;
        cursor_x = x + (w - sl * 8) / 2;
        cursor_y = y + h/2 - 12;
        gprint((char *)txt1, 0xFF9F3B);
    }
    if (txt2) {
        int sl = 0; while (txt2[sl]) ++sl;
        cursor_x = x + (w - sl * 8) / 2;
        cursor_y = y + h/2 + 2;
        gprint((char *)txt2, 0xFFFFFF);
    }
}

static void draw_content_replay_control(int x, int y, int w, int h) {
    int cx = x;
    int cy = y;
    int cw = w;
    int ch = h;
    
    draw_filled_rect(cx, cy, cw, ch, 0x05131C);
    
    int tb_y = cy + 10;
    draw_filled_rect(cx + 10, tb_y, cw - 200, 60, 0x0D2232);
    draw_rect(cx + 10, tb_y, cw - 200, 60, 0x2A617B);

    int btn_w = (cw - 220) / 6 - 5;
    int bx = cx + 18;
    int by = tb_y + 8;
    int border_c = 0x4CEBFD;
    int bg_c     = 0x0A2D40;
    
    draw_hex_button(bx, by, btn_w, 44, "|[<<]", "(Start)", border_c, bg_c); bx += btn_w + 6;
    draw_hex_button(bx, by, btn_w, 44, "[<<]", "(Rewind)", border_c, bg_c); bx += btn_w + 6;
    draw_hex_button(bx, by, btn_w, 44, "[ || ]", "(Pause)", border_c, bg_c); bx += btn_w + 6;
    draw_hex_button(bx, by, btn_w, 44, "|[>]", "(Play)", border_c, bg_c); bx += btn_w + 6;
    draw_hex_button(bx, by, btn_w, 44, "[>>]", "(Fast-Forward)", border_c, bg_c); bx += btn_w + 6;
    draw_hex_button(bx, by, btn_w, 44, "[>>]|", "(End)", border_c, bg_c); 
    
    int rx = cx + cw - 180;
    int is_rec = shell_is_recording_replay();
    draw_hex_button_orange(rx + 8, by, 144, 44, "RECORD", "User Summary", is_rec ? 0xFF0000 : 0xFF9F3B, 0x1F1410, 1);
    
    int t_y = cy + 80;
    draw_filled_rect(cx + 10, t_y, cw - 20, 80, 0x0D2232);
    draw_rect(cx + 10, t_y, cw - 20, 80, 0x2A617B);
    cursor_x = cx + 16; cursor_y = t_y + 6; gprint((char *)"Replay Scrubber Timeline", 0xFFFFFF);
    
    int line_y = t_y + 50;
    draw_line_segment(cx + 20, line_y, cx + cw - 30, line_y, 0x367F60);
    for(int i=cx+20; i < cx+cw-30; i+= 15) {
       draw_vline(i, line_y - 4, line_y + 4, 0x367F60);
    }
    
    if (replay_state.playing) {
        replay_state.current_frame += 2;
        if (replay_state.current_frame > (cw - 60)) {
            replay_state.playing = 0;
            replay_state.current_frame = 0;
            replay_state.last_cmd_executed = -1;
        }
    }
    
    int phx = cx + 20 + replay_state.current_frame;
    if (phx > cx + cw - 30) phx = cx + cw - 30;
    
    int pins = replay_count;
    if (pins > 0) {
        int spacing = (cw - 60) / pins;
        for (int p = 0; p < pins; p++) {
            int px = cx + 20 + (p + 1) * spacing;
            draw_filled_rect(px - 3, line_y - 3, 6, 6, 0x4EEBFE);
            draw_vline(px, t_y + 20, t_y + 50, 0x4EEBFE);
            
            if (replay_state.playing && phx >= px && p > replay_state.last_cmd_executed) {
                replay_state.last_cmd_executed = p;
                char cmd_copy[32];
                extern void copy_cmd_text(char *, const char *, int);
                int k = 0;
                while (replay_cmds[p][k] && k < 31) {
                    cmd_copy[k] = replay_cmds[p][k];
                    k++;
                }
                cmd_copy[k] = 0;
                process_command(cmd_copy);
            }
        }
    }
    draw_filled_rect(phx - 2, t_y + 30, 4, 30, 0x5DEAD4);
    
    cursor_x = cx + cw/2 - 64; cursor_y = t_y + 60;
    gprint((char *)"T:18:07:05.123", 0x5DEAD4);

    int h_y = cy + 170;
    draw_filled_rect(cx + 10, h_y, cw - 340, 110, 0x0D2232);
    draw_rect(cx + 10, h_y, cw - 340, 110, 0x2A617B);
    cursor_x = cx + 16; cursor_y = h_y + 6; gprint((char *)"ACTION HISTORY - Sequential Commands", 0xFFFFFF);
    
    for (int k = 0; k < replay_count && k < 4; k++) {
        cursor_x = cx + 16; cursor_y = h_y + 26 + k*16;
        gprint((char *)"> Ardent REPL ", 0xDDDDDD);
        char num[4];
        num[0] = '1' + k; num[1] = '.'; num[2] = ' '; num[3] = 0;
        gprint(num, 0xCCCCCC);
        gprint(replay_cmds[k], 0xFFDD99);
        gprint((char *)" (T+...ms)", 0x888888);
    }
    
    draw_filled_rect(cx + cw - 320, h_y, 310, 110, 0x0D2232);
    draw_rect(cx + cw - 320, h_y, 310, 110, 0x2A617B);
    
    draw_hex_button_orange(cx + cw - 310, h_y + 30, 140, 60, "CLEAR QUEUE", "", 0xFF9F3B, 0x1D0E09, 0);
    draw_hex_button_orange(cx + cw - 160, h_y + 30, 140, 60, "EXPORT SESSION", "Manifest", 0xFF9F3B, 0x1D0E09, 0);
}


int wm_is_replay_focused(void) {
    if (wm_focused >= 0 && wm_focused < wm_window_count) {
        if (wm_str_eq(wm_windows[wm_focused].title, "Replay Control App")) {
            return 1;
        }
    }
    return 0;
}

void wm_open_replay_control(void) {
    int j;
    for (j = 0; j < wm_window_count; j++) {
        if (wm_str_eq(wm_windows[j].title, "Replay Control App")) {
            wm_windows[j].x = 150;
            wm_windows[j].y = 150;
            wm_windows[j].w = screen_w;
            wm_windows[j].h = 360;
            wm_windows[j].visible = 1;
            wm_windows[j].state = WM_STATE_NORMAL;
            bring_to_front(j);
            return;
        }
    }
}extern int wm_is_replay_focused(void);
extern int wm_str_eq(const char *s1, const char *s2);

void handle_replay_control_click_idx(int win_idx, int mx, int my) {
    if (win_idx < 0 || win_idx >= 12) return;
    int wx = wm_windows[win_idx].x;
    int wy = wm_windows[win_idx].y;
    int rx = mx - wx;
    int ry = my - wy;
    
    int tb_y = 30 + 10;
    int by = tb_y + 8;
    int btn_w = (wm_windows[win_idx].w - 220) / 6 - 5;
    
    if (ry >= by && ry <= by + 44) {
        int bx = 18;
        if (rx >= bx && rx <= bx + btn_w) {
            replay_state.current_frame = 0;
            replay_state.last_cmd_executed = -1;
            return;
        }
        bx += btn_w + 6;
        if (rx >= bx && rx <= bx + btn_w) {
            replay_state.current_frame -= 60;       
            if (replay_state.current_frame < 0) {
               replay_state.current_frame = 0;
               replay_state.last_cmd_executed = -1;
            }
            return;
        }
        bx += btn_w + 6;
        if (rx >= bx && rx <= bx + btn_w) {
            replay_state.playing = 0;
            return;
        }
        bx += btn_w + 6;
        if (rx >= bx && rx <= bx + btn_w) {
            int cx2 = wm_windows[win_idx].w - WM_BORDER * 2;
            if (replay_state.current_frame >= (cx2 - 60)) {
                replay_state.current_frame = 0;
                replay_state.last_cmd_executed = -1;
            }
            replay_state.playing = 1;
            return;
        }
        bx += btn_w + 6;
        if (rx >= bx && rx <= bx + btn_w) {
            replay_state.current_frame += 60;       
            return;
        }
        bx += btn_w + 6;
        if (rx >= bx && rx <= bx + btn_w) {
            replay_state.current_frame = 99999;
            return;
        }
        
        int rbx = wm_windows[win_idx].w - 180 + 8;
        if (rx >= rbx && rx <= rbx + 144) {
            process_command("replay rec toggle");
            return;
        }
    }
    
    int h_y = 30 + 170;
    int cq_x = wm_windows[win_idx].w - 310;
    int ey_x = wm_windows[win_idx].w - 160;
    
    if (ry >= h_y + 30 && ry <= h_y + 90) {
        if (rx >= cq_x && rx <= cq_x + 140) {
            process_command("replay clear");
            return;
        }
        if (rx >= ey_x && rx <= ey_x + 140) {
            process_command("dataset export");
            return;
        }
    }
}





static void telemetry_itoa(int value, char *str) {
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    int is_negative = 0;
    if (value < 0) {
        is_negative = 1;
        value = -value;
    }
    char temp[32];
    int i = 0;
    while (value > 0) {
        temp[i++] = (value % 10) + '0';
        value /= 10;
    }
    int j = 0;
    if (is_negative) str[j++] = '-';
    while (i > 0) {
        str[j++] = temp[--i];
    }
    str[j] = '\0';
}

extern void itoa(int n, char *str);

void draw_content_telemetry(int cx, int cy, int cw, int ch) {
    draw_filled_rect(cx, cy, cw, ch, 0x05131C);
    int pill_w = 200, pill_h = 24;
    int pill_x = cx + (cw - pill_w)/2;
    int pill_y = cy + 10;
    draw_filled_rect(pill_x, pill_y, pill_w, pill_h, 0x0D2B1C);
    draw_rect(pill_x, pill_y, pill_w, pill_h, 0x24C655);
    draw_filled_rect(pill_x + 12, pill_y + 8, 8, 8, 0x24C655); 
    extern int cursor_x, cursor_y;
    cursor_x = pill_x + 30; cursor_y = pill_y + 8;
    gprint("ALL SYSTEMS NOMINAL", 0x24C655);

    int margin = 10;
    int panel_w = (cw / 2) - (margin * 1.5);
    int left_x = cx + margin;
    int right_x = cx + cw / 2 + margin / 2;

    int top_offset = 40;
    int avail_h = ch - top_offset - 40; // Total available vertical space subtracting top/bottom margins
    if (avail_h < 300) avail_h = 300;   // Layout minimum guard

    int row1_y = cy + top_offset;
    int row1_h = (avail_h * 40) / 100;

    int row2_y = row1_y + row1_h + margin;
    int row2_h = (avail_h * 32) / 100;

    int row3_y = row2_y + row2_h + margin;
    int row3_h = ch - (row3_y - cy) - margin;
    extern uint32_t tick;
    uint32_t secs = tick / 100;
    uint32_t mins = secs / 60;
    uint32_t hrs = mins / 60;

    extern int pmm_get_free_pages(void);
    extern int pmm_get_total_pages(void);
    int total_mb = pmm_get_total_pages() * 4 / 1024;
    if (total_mb == 0) total_mb = 32; // fallback
    int free_mb = pmm_get_free_pages() * 4 / 1024;
    int used_mb = total_mb - free_mb;
    if (used_mb < 0) used_mb = 0;

    NetRxStats nstats;
    net_get_rx_stats(&nstats);

    extern TCB os_tasks[MAX_TASKS];
    extern int os_task_count;
    /* extern WmNeuralPixel os_memory_map[2]; */

    // ----- ROW 1 LEFT: OS KERNEL & SCHEDULER -----
    draw_filled_rect(left_x, row1_y, panel_w, row1_h, 0x0D2232);
    draw_rect(left_x, row1_y, panel_w, row1_h, 0x4CEBFD);
    draw_filled_rect(left_x, row1_y, panel_w, 20, 0x123A4A);
    cursor_x = left_x + 8; cursor_y = row1_y + 6; gprint("OS KERNEL & SCHEDULER", 0xFFFFFF);
    
    cursor_x = left_x + 8; cursor_y = row1_y + 26; gprint("Processes", 0x888888);
    cursor_x = left_x + 120; cursor_y = row1_y + 26; gprint("Status", 0x888888);
    cursor_x = left_x + 180; cursor_y = row1_y + 26; gprint("PID", 0x888888);
    cursor_x = left_x + 228; cursor_y = row1_y + 26; gprint("Priority", 0x888888);
    draw_hline(row1_y + 38, left_x + 8, left_x + panel_w - 8, 0x2A617B);
    
    int num_to_draw = os_task_count > 5 ? 5 : os_task_count;
    for (int i=0; i<num_to_draw; i++) {
        int ry = row1_y + 44 + i * 16;
        
        char name[32]; wm_strcpy(name, "task_", 31);
        char tnum[4]; telemetry_itoa(i, tnum);
        int nlen=0; while(name[nlen]) nlen++; 
        for (int m = 0; tnum[m] && nlen < 31; m++) {
            name[nlen++] = tnum[m];
        }
        name[nlen] = 0;
        
        char pid[8]; telemetry_itoa(os_tasks[i].task_id, pid);
        char status[32];
        if (os_tasks[i].state == 1) wm_strcpy(status, "RUNNING", 31);
        else if (os_tasks[i].state == 0) wm_strcpy(status, "READY", 31);
        else if (os_tasks[i].state == 2) wm_strcpy(status, "BLOCKED", 31);
        else if (os_tasks[i].state == 3) wm_strcpy(status, "SLEEPING", 31);
        else wm_strcpy(status, "DYING", 31);
        
        char prio[8]; telemetry_itoa(os_tasks[i].priority, prio);

        cursor_x = left_x + 8; cursor_y = ry; gprint(name, 0xFFFFDD);
        cursor_x = left_x + 120; cursor_y = ry; gprint(status, os_tasks[i].state == 1 ? 0x24C655 : 0xAAAAAA);
        cursor_x = left_x + 180; cursor_y = ry; gprint(pid, 0xDDDDDD);
        cursor_x = left_x + 228; cursor_y = ry; gprint(prio, 0xFFDD88);
    }
    draw_filled_rect(left_x + panel_w - 6, row1_y + 44, 4, 30, 0x4CEBFD);

    // ----- ROW 1 RIGHT: TELEMETRY PANEL -----
    draw_filled_rect(right_x, row1_y, panel_w, row1_h, 0x0D2232);
    draw_rect(right_x, row1_y, panel_w, row1_h, 0x4CEBFD);
    draw_filled_rect(right_x, row1_y, panel_w, 20, 0x123A4A);
    cursor_x = right_x + 8; cursor_y = row1_y + 6; gprint("TELEMETRY PANEL", 0xFFFFFF);
    
    int sub_y = row1_y + 30, sub_h = row1_h - 40, sub_w = (panel_w - 40) / 3;
    
    int b1_x = right_x + 10;
    draw_filled_rect(b1_x + 5, sub_y, sub_w - 5, sub_h, 0x1A2A3A); draw_rect(b1_x + 5, sub_y, sub_w - 5, sub_h, 0x364E68);
    cursor_x = b1_x + ((sub_w-5)/2) - 20; cursor_y = sub_y + 8; gprint("Clock", 0xDDDDDD);
    
    // Clock icon
    draw_rect(b1_x + ((sub_w-5)/2) - 8, sub_y + 30, 16, 16, 0x4CEBFD);
    draw_line_segment(b1_x + ((sub_w-5)/2), sub_y + 38, b1_x + ((sub_w-5)/2), sub_y + 34, 0x4CEBFD);
    draw_line_segment(b1_x + ((sub_w-5)/2), sub_y + 38, b1_x + ((sub_w-5)/2) + 4, sub_y + 38, 0x4CEBFD);

    char time_str[16], hm_s[8]; telemetry_itoa(hrs % 24, time_str);
    int tlen=0; while(time_str[tlen]) tlen++;
    time_str[tlen++] = ':'; if ((mins%60) < 10) time_str[tlen++] = '0';
    telemetry_itoa(mins%60, hm_s); for(int m=0; hm_s[m]; m++) time_str[tlen++] = hm_s[m]; time_str[tlen] = 0;
    cursor_x = b1_x + ((sub_w-5)/2) - 20; cursor_y = sub_y + 55; gprint(time_str, 0xFFFFFF);

    int b2_x = b1_x + sub_w + 10;
    draw_filled_rect(b2_x + 5, sub_y, sub_w - 5, sub_h, 0x1A2A3A); draw_rect(b2_x + 5, sub_y, sub_w - 5, sub_h, 0x364E68);
    cursor_x = b2_x + ((sub_w-5)/2) - 24; cursor_y = sub_y + 8; gprint("Memory", 0xDDDDDD);

    char mem_str[32], mb_str[8]; telemetry_itoa(used_mb, mem_str);
    int mlen=0; while(mem_str[mlen]) mlen++;
    mem_str[mlen++] = 'M'; mem_str[mlen++] = 'B'; mem_str[mlen++] = ' '; mem_str[mlen++] = '/'; mem_str[mlen++] = ' ';
    telemetry_itoa(total_mb, mb_str); for(int m=0; mb_str[m]; m++) mem_str[mlen++] = mb_str[m];
    mem_str[mlen++] = 'M'; mem_str[mlen++] = 'B'; mem_str[mlen] = 0;
    cursor_x = b2_x + 12; cursor_y = sub_y + 24; gprint(mem_str, 0xAAAAAA);
    int chart_y_base = sub_y + sub_h - 10;
    static int mem_hist[64] = {0};
    static int cpu_hist[8] = {0};
    static uint32_t last_hist_tick = 0;
    
    // CPU load calculation based on running tasks ratio + base load
    int running_tasks = 0;
    for (int i=0; i<os_task_count; i++) if (os_tasks[i].state == 1) running_tasks++;
    int cpu_load = 5; // Base OS load
    if (os_task_count > 0) cpu_load += (running_tasks * 85) / os_task_count;
    if (cpu_load > 100) cpu_load = 100;

    // Update history every ~1 second (assuming 100 ticks = 1s)
    if (tick - last_hist_tick > 100 || last_hist_tick == 0) {
        for (int i=0; i<63; i++) mem_hist[i] = mem_hist[i+1];
        mem_hist[63] = used_mb;
        
        for (int i=0; i<7; i++) cpu_hist[i] = cpu_hist[i+1];
        cpu_hist[7] = cpu_load;
        
        last_hist_tick = tick;
    }

    // Draw Memory sparkline chart
    int max_mem_hist_w = (sub_w - 20) / 2;
    int mem_chart_h = (sub_h * 40) / 100; // Takes 40% of the box height
    for(int i=0; i<max_mem_hist_w && i<64; i+=1) {
        int v = (mem_hist[63 - max_mem_hist_w + i + 1] * mem_chart_h) / total_mb;
        if(v>mem_chart_h) { v=mem_chart_h; } if(v<0) { v=0; }
        int next_v = (mem_hist[63 - max_mem_hist_w + i + 2] * mem_chart_h) / total_mb;
        if(next_v>mem_chart_h) { next_v=mem_chart_h; } if(next_v<0) { next_v=0; }

        draw_line_segment(b2_x + 10 + (i*2), chart_y_base - v, b2_x + 12 + (i*2), chart_y_base - next_v, 0x4CEBFD);
    }

    int b3_x = b2_x + sub_w + 10;
    draw_filled_rect(b3_x + 5, sub_y, sub_w - 5, sub_h, 0x1A2A3A); draw_rect(b3_x + 5, sub_y, sub_w - 5, sub_h, 0x364E68);
    cursor_x = b3_x + ((sub_w-5)/2) - 12; cursor_y = sub_y + 8; gprint("CPU", 0xDDDDDD);
    char cpu_str[16]; telemetry_itoa(cpu_load, cpu_str);
    int clen=0; while(cpu_str[clen]) clen++; cpu_str[clen++]='%'; cpu_str[clen]=0;
    cursor_x = b3_x + ((sub_w-5)/2) - 10; cursor_y = sub_y + 24; gprint(cpu_str, 0xAAAAAA);
    int bar_w = ((sub_w-20)/8) - 2;

    // Draw CPU history bars
    int cpu_chart_h = (sub_h * 50) / 100; // Takes 50% of the box height
    for(int i=0; i<8; i++) {
        int h = (cpu_hist[i] * cpu_chart_h) / 100;
        if(h<2) h=2; else if (h>cpu_chart_h) h=cpu_chart_h;
        uint32_t bar_c = (h > (cpu_chart_h * 75)/100) ? 0xFF8800 : 0x24C655;
        draw_filled_rect(b3_x + 10 + i*(bar_w+2), sub_y + sub_h - h - 5, bar_w, h, bar_c);
    }


    // ----- ROW 2 LEFT: NETWORK QUALITY -----
    draw_filled_rect(left_x, row2_y, panel_w, row2_h, 0x0D2232);       
    draw_rect(left_x, row2_y, panel_w, row2_h, 0x4CEBFD);
    draw_filled_rect(left_x, row2_y, panel_w, 20, 0x123A4A);
    cursor_x = left_x + 8; cursor_y = row2_y + 6; gprint("NETWORK QUALITY", 0xFFFFFF);
    cursor_x = left_x + 10; cursor_y = row2_y + 26; gprint("Bytes Processed", 0xAAAAAA);

    char bytes_str[32]; telemetry_itoa(nstats.total_bytes, bytes_str);  
    cursor_x = left_x + 10; cursor_y = row2_y + 42; gprint(bytes_str, 0xFFFFFF);

    cursor_x = left_x + 200; cursor_y = row2_y + 26; gprint("Drop / IRQs", 0xAAAAAA);
    for(int i=0; i<20; i++) {
        int h = 4 + i; int errs = nstats.dropped_frames;
        uint32_t c = (i > 15 && errs > 5) ? 0xFF8800 : ((i > 10 && errs > 0) ? 0xFFFF00 : 0x24C655);
        draw_filled_rect(left_x + 200 + i*8, row2_y + 60 - h, 6, h, c);
        draw_line_segment(left_x + 200 + i*8, row2_y + 60 - h, left_x + 206 + i*8, row2_y + 60 - h - 1, 0xFFFFFF);
    }

    char drop_str[32]; telemetry_itoa(nstats.dropped_frames, drop_str); 
    cursor_x = left_x + 200; cursor_y = row2_y + 66; gprint(drop_str, 0xFF7777);

    // ----- ROW 2 RIGHT: SPIKE ACTIVITY - NEMS -----
    spike_monitor_update();
    draw_filled_rect(right_x, row2_y, panel_w, row2_h, 0x0D2232);       
    draw_rect(right_x, row2_y, panel_w, row2_h, 0x4CEBFD);
    draw_filled_rect(right_x, row2_y, panel_w, 20, 0x123A4A);
    cursor_x = right_x + 8; cursor_y = row2_y + 6; gprint("SPIKE ACTIVITY - NEMS", 0xFFFFFF);
    
    int spark_w = (panel_w * 40) / 100; if(spark_w < 120) spark_w = 120;
    int spark_x = right_x + panel_w - spark_w - 20;
    
    cursor_x = right_x + 10; cursor_y = row2_y + 30; gprint("Peak Firing Rate (ms)", 0xAAAAAA);
    cursor_x = spark_x; cursor_y = row2_y + 30; gprint("Raster Sparkline      Scrolls", 0xAAAAAA);

    int max_bar_h = (row2_h * 40) / 100;
    int bar_y_base = row2_y + 40 + max_bar_h;

    for(int i=0; i<15; i++) {
        int idx = (g_spike_mon.trend_col + i*8) % 120;
        int v = (g_spike_mon.trend_a[idx] * max_bar_h) / 100; // Increased multiplier for higher bars
        int h = 5 + v; if(h>max_bar_h) h=max_bar_h;
        uint32_t c = (h > (max_bar_h*80)/100) ? 0xFF8800 : ((h > (max_bar_h*50)/100) ? 0xFFFF00 : 0x24C655);
        draw_filled_rect(right_x + 10 + i*10, bar_y_base - h, 8, h, c);
        draw_line_segment(right_x + 10 + i*10, bar_y_base - h, right_x + 18 + i*10, bar_y_base - h - 2, 0xFFFFFF);
    }
    cursor_x = right_x + 10; cursor_y = bar_y_base + 10; gprint("Peak Firing Rate Points", 0x888888);
    for(int i=0; i<8; i++) {
        int ry = (i * 6) % 48;
        int spike = g_spike_mon.raster[ry][(g_spike_mon.col + 159) % 160];
        uint32_t clrc = spike ? 0xFF8800 : 0x24C655;
        int offset_y = spike ? 2 : 0;
        int size = spike ? 6 : 4;
        draw_filled_rect(right_x + 10 + i*16, bar_y_base + 26 - offset_y, size, size, clrc);
    }

    int prev_v = 0;
    int spark_y_base = row2_y + (row2_h * 75) / 100;
    for(int i=0; i<spark_w; i+=3) {
        // Read historical data ordered oldest to newest smoothly       
        int idx = (g_spike_mon.trend_col + (i * 120) / spark_w) % 120;  

        // Spike rate (avg_hz) naturally creates a noisy waveform based on traffic
        int v = g_spike_mon.trend_a[idx] / 4;

        // Inject realtime voltage from a neuron to give it immediate life instead of slow trend
        int n_volt = os_memory_map[0].neurons[i % 5].voltage;
        int real_spike = (n_volt > 200) ? n_volt/40 : 0;
        
        v += real_spike;
        int max_spark_v = (row2_h * 30) / 100;
        if(v > max_spark_v) v = max_spark_v;
        if(v < 0) v = 0;

        if (i > 0) {
            draw_line_segment(spark_x + i - 3, spark_y_base - prev_v, spark_x + i, spark_y_base - v, 0x24C655);
        }
        prev_v = v;
    }
    draw_filled_rect(spark_x + 10, spark_y_base + 5, 80, 4, 0x4CEBFD);

    // ----- ROW 3 LEFT: NETWORK STATUS -----
    draw_filled_rect(left_x, row3_y, panel_w, row3_h, 0x0D2232);
    draw_rect(left_x, row3_y, panel_w, row3_h, 0x4CEBFD);
    draw_filled_rect(left_x, row3_y, panel_w, 20, 0x123A4A);
    cursor_x = left_x + 8; cursor_y = row3_y + 6; gprint("NETWORK STATUS", 0xFFFFFF);
    cursor_x = left_x + 10; cursor_y = row3_y + 30; gprint("Total Frames RX", 0xAAAAAA);
    
    char n_frames[32]; telemetry_itoa(nstats.total_frames, n_frames);
    cursor_x = left_x + 20; cursor_y = row3_y + 46; gprint(n_frames, 0xFFFFFF);
    cursor_x = left_x + 10; cursor_y = row3_y + 70; gprint("Status: Active", 0x24C655);
    for(int i=0; i<20; i++) {
        int af = (nstats.total_frames / 100) % 20;
        if (i <= af || af == 0) draw_filled_rect(left_x + 180 + i*8, row3_y + 70, 6, 14, 0x24C655);
        else draw_filled_rect(left_x + 180 + i*8, row3_y + 70, 6, 14, 0x113311);
    }


}

void wm_open_telemetry_cockpit(void) {
    int j;
    for (j = 0; j < wm_window_count; j++) {
        if (wm_str_eq(wm_windows[j].title, "Telemetry Cockpit")) {
            wm_windows[j].visible = 1;
            bring_to_front(j);
            return;
        }
    }
    if (wm_window_count < 12) {
        wm_add_window(10, 30, 780, 520, "Telemetry Cockpit", draw_content_telemetry, 0, -1);
        wm_windows[wm_window_count - 1].visible = 1;
        bring_to_front(wm_window_count - 1);
    }
}


int handle_replay_control_click(WmWindow *w, int mx, int my) {
    int cx = w->x + WM_BORDER;
    int cy = w->y + WM_TITLEBAR_H;
    int cw = w->w - WM_BORDER * 2;
    int tx = mx - cx;
    int ty = my - cy;

    if (tx >= cw - 180 && tx <= cw - 20 && ty >= 10 && ty <= 70) {
        process_command("replay rec toggle");
        return 1;
    }
    if (ty >= 10 && ty <= 70 && tx >= 10 && tx <= cw - 200) {
        int btn_pitch = (cw - 220) / 6;
        int idx = (tx - 18) / (btn_pitch + 6);
        if (idx == 3) {
           process_command("replay run");
        }
        return 1;
    }
    if (ty >= 190 && ty <= 310) {
        if (tx >= cw - 310 && tx <= cw - 170) {
            process_command("replay clear");
            return 1;
        }
        if (tx >= cw - 160 && tx <= cw - 20) {
            process_command("dataset export session_manifest.dat");
            return 1;
        }
    }
    return 0;
}
