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
extern volatile int mouse_x;
extern volatile int mouse_y;
extern volatile int mouse_buttons;
extern uint32_t backbuffer[];

extern int pmm_get_free_pages(void);

/* ------------------------------------------------------------ */
/*  Window state                                                 */
/* ------------------------------------------------------------ */
WmWindow wm_windows[WM_MAX_WINDOWS];
int wm_window_count = 0;
int wm_focused = -1;

static int z_order[WM_MAX_WINDOWS];
static int z_count = 0;


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

static uint32_t blend_50(uint32_t a, uint32_t b) {
    uint32_t ar = (a >> 16) & 0xFF;
    uint32_t ag = (a >> 8) & 0xFF;
    uint32_t ab = a & 0xFF;
    uint32_t br = (b >> 16) & 0xFF;
    uint32_t bg = (b >> 8) & 0xFF;
    uint32_t bb = b & 0xFF;
    return (((ar + br) >> 1) << 16) | (((ag + bg) >> 1) << 8) | ((ab + bb) >> 1);
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

    /* Right-side glass status widget: time, battery and memory space. */
    {
        int wx = 646;
        int wy = WM_TASKBAR_Y - 2;
        int ww = 150;
        int wh = 50;
        uint32_t secs = tick / 100;
        uint32_t mins = secs / 60;
        uint32_t hrs  = mins / 60;
        int battery = 62 + (int)((tick / 300) % 32);
        int free_mb = pmm_get_free_pages() * 4 / 1024;

        char tbuf[6];
        char bbuf[5];
        tbuf[0] = (char)('0' + (hrs % 100) / 10);
        tbuf[1] = (char)('0' + hrs % 10);
        tbuf[2] = ':';
        tbuf[3] = (char)('0' + (mins % 60) / 10);
        tbuf[4] = (char)('0' + mins % 10);
        tbuf[5] = '\0';

        bbuf[0] = (char)('0' + (battery / 10));
        bbuf[1] = (char)('0' + (battery % 10));
        bbuf[2] = '%';
        bbuf[3] = '\0';

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

        cursor_x = wx + 12;
        cursor_y = wy + 28;
        gprint("Space:", 0x9FC2D8);
        gprint_dec(free_mb, 0xBDEBFF);
        gprint("MB", 0x9FC2D8);
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

    /* 1. Check taskbar clicks */
    if (my >= WM_TASKBAR_Y) {
        for (i = 0; i < WM_ICON_SLOTS; i++) {
            int cx = icon_x_pos(i);
            if (pt_in_rect(mx, my, cx - 20, WM_TASKBAR_Y + 4, 40, 46)) {
                if (i == 0) {
                    /* OS logo - decorative, do nothing */
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

    /* 4. Mouse cursor on very top */
    wm_draw_cursor();
}
