/* ================================================================
 * NeuroDock App Launcher System
 *
 * Implements the neuromorphic-themed application launcher with
 * animated transitions, app discovery by category, search, and
 * live status badges.
 * ================================================================ */
#include "launcher.h"
extern void wm_get_taskbar_icon_center(int slot, int *out_x, int *out_y);
extern void wm_launch_icon_slot(int slot);
extern void wm_open_model_manager(void);
extern void wm_open_spike_monitor(void);
extern void wm_open_snapshot_browser(void);
#define LAUNCHER_ANIMATION_SPEED 8      /* Frames to fully open/close */
#define LAUNCHER_RIPPLE_RADIUS 300      /* Max ripple spread from OS button */
#define APP_TILE_W 78
#define APP_TILE_H 62
#define APP_TILE_PAD 8
#define LAUNCHER_GRID_COLS 8
#define LAUNCHER_CONTENT_X 32
#define LAUNCHER_CONTENT_Y 60
#define LAUNCHER_CONTENT_W (800 - 64)
#define LAUNCHER_CONTENT_H 470
#define LAUNCHER_HEADER_H 36
#define LAUNCHER_SEARCH_H 32

/* Color Palette (neuromorphic) */
#define COL_BG_DARK 0x000018
#define COL_BG_TEAL 0x004060
#define COL_ACCENT_CYAN 0x00FFFF
#define COL_DATA_GREEN 0x00FF00
#define COL_CRITICAL_ORANGE 0xFF8800
#define COL_OVERLAY 0x001122
#define COL_TEXT_DIM 0x888888
#define COL_TEXT_BRIGHT 0xFFFFFF
#define COL_RIPPLE 0x00FFFF
#define COL_SPIKE_PURPLE 0xAA00FF
#define COL_SYNAPSE_CYAN 0x00DDDD

/* External graphics primitives */
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
extern uint32_t backbuffer[];

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

/* App registry and launcher state */
LauncherAppEntry launcher_apps[32];
int launcher_app_count = 0;
LauncherState launcher_state = {0};

/* Category names */
static const char *category_names[APP_CAT_COUNT] = {
    "Experimentation",
    "Monitoring",
    "Storage",
    "Networking",
    "Tools",
    "System"
};

/* ================================================================
 * Initialization
 * ================================================================ */
void launcher_init(void) {
    launcher_state.is_open = 0;
    launcher_state.animation_state = 0;
    launcher_state.animation_tick = 0;
    launcher_state.scroll_offset = 0;
    launcher_state.search_active = 0;
    launcher_state.search_buffer[0] = '\0';
    launcher_state.search_cursor = 0;
    launcher_state.hover_app_idx = -1;
    /* Fallback center; refreshed from WM taskbar icon each render/open. */
    launcher_state.os_button_x = 40;
    launcher_state.os_button_y = 560;

    /* Register dock-visible apps first so launcher mirrors taskbar app surface. */
    launcher_register_app("Launcher", APP_CAT_TOOLS, 'L', COL_ACCENT_CYAN, 0, "/bin/launcher");
    launcher_register_app("Neural Viz", APP_CAT_MONITORING, 'N', 0x66EEFF, 0, "/bin/neuralviz");
    launcher_register_app("Console", APP_CAT_TOOLS, 'C', 0x00AA00, 0, "/bin/console");
    launcher_register_app("Profiler", APP_CAT_TOOLS, 'F', 0xFF6600, 0, "/bin/profile");
    launcher_register_app("Settings", APP_CAT_SYSTEM, 'G', COL_TEXT_DIM, 0, "/bin/settings");

    /* Neuromorphic workflow apps */
    launcher_register_app("Model Manager", APP_CAT_EXPERIMENTATION, 'M', 0xFF0088, 0, "/bin/model");
    launcher_register_app("Spike Monitor", APP_CAT_MONITORING, 'S', COL_SPIKE_PURPLE, 0, "/bin/spike");
    launcher_register_app("Snapshot Browser", APP_CAT_STORAGE, 'D', COL_DATA_GREEN, 0, "/bin/snapshot");
    launcher_register_app("Replay Control", APP_CAT_EXPERIMENTATION, 'P', COL_ACCENT_CYAN, 0, "/bin/replay");
    launcher_register_app("Telemetry Panel", APP_CAT_MONITORING, 'T', COL_CRITICAL_ORANGE, 0, "/bin/telemetry");
}

/* ================================================================
 * App Registry Management
 * ================================================================ */
void launcher_register_app(const char *name, int category, char icon_char,
                           uint32_t icon_color, void (*launch_fn)(void),
                           const char *exec_path) {
    if (launcher_app_count >= 32) return;

    LauncherAppEntry *app = &launcher_apps[launcher_app_count];
    int i = 0;
    while (name[i] && i < 31) {
        app->name[i] = name[i];
        i++;
    }
    app->name[i] = '\0';

    app->category = category;
    app->icon_char = icon_char;
    app->icon_color = icon_color;
    app->pinned = 0;
    app->recently_used = 0;
    app->status = APP_STATUS_IDLE;
    app->launch_fn = launch_fn;

    i = 0;
    if (exec_path) {
        while (exec_path[i] && i < 63) {
            app->exec_path[i] = exec_path[i];
            i++;
        }
    }
    app->exec_path[i] = '\0';

    launcher_app_count++;
}

void launcher_set_app_status(int app_idx, int status) {
    if (app_idx >= 0 && app_idx < launcher_app_count) {
        launcher_apps[app_idx].status = status;
    }
}

void launcher_mark_recently_used(int app_idx) {
    if (app_idx >= 0 && app_idx < launcher_app_count) {
        launcher_apps[app_idx].recently_used = tick;
    }
}

/* ================================================================
 * Launcher Open/Close Control
 * ================================================================ */
void launcher_toggle(void) {
    if (launcher_state.animation_state == 0) {
        launcher_open();
    } else if (launcher_state.animation_state == 2) {
        launcher_close();
    }
}

void launcher_open(void) {
    if (launcher_state.animation_state > 0) return;  /* Already animating */
    wm_get_taskbar_icon_center(0, &launcher_state.os_button_x, &launcher_state.os_button_y);
    launcher_state.animation_state = 1;  /* Opening */
    launcher_state.animation_tick = tick;
    launcher_state.is_open = 1;
    launcher_state.hover_app_idx = -1;
}

void launcher_close(void) {
    if (launcher_state.animation_state != 2) return;  /* Not open */
    launcher_state.animation_state = 3;  /* Closing */
    launcher_state.animation_tick = tick;
}

/* ================================================================
 * Rendering
 * ================================================================ */

static void draw_ripple_circle(int center_x, int center_y, int radius, uint32_t color, int alpha_fade) {
    /* Draw an expanding circle ripple effect from OS button */
    (void)alpha_fade;
    int r_inner = radius - 2;
    int r_outer = radius;

    for (int y = center_y - r_outer; y <= center_y + r_outer; y++) {
        if (y < 0 || y >= SCREEN_HEIGHT) continue;
        for (int x = center_x - r_outer; x <= center_x + r_outer; x++) {
            if (x < 0 || x >= SCREEN_WIDTH) continue;

            int dx = x - center_x;
            int dy = y - center_y;
            int dist_sq = dx * dx + dy * dy;
            int r_sq = radius * radius;

            if (dist_sq >= r_inner * r_inner && dist_sq <= r_sq) {
                /* Blend ripple into backbuffer with fade */
                uint32_t existing = backbuffer[y * SCREEN_WIDTH + x];
                uint32_t blended = ((existing >> 1) & 0x7F7F7F) + ((color >> 1) & 0x7F7F7F);
                backbuffer[y * SCREEN_WIDTH + x] = blended;
            }
        }
    }
}

static void draw_app_tile(int x_pos, int y_pos, LauncherAppEntry *app, int hovered) {
    uint32_t bg_color = hovered ? 0x003366 : 0x001144;
    uint32_t border_color = hovered ? COL_SYNAPSE_CYAN : COL_TEXT_DIM;

    /* Tile background */
    clear_region(x_pos, y_pos, x_pos + APP_TILE_W, y_pos + APP_TILE_H, bg_color);

    /* Icon */
    int icon_x = x_pos + (APP_TILE_W / 2) - 3;
    int icon_y = y_pos + 6;
    draw_char(app->icon_char, icon_x, icon_y, app->icon_color);

    /* App name below icon (truncate to avoid tile overflow) */
    {
        char label[10];
        int i = 0;
        while (app->name[i] && i < 9) {
            label[i] = app->name[i];
            i++;
        }
        label[i] = '\0';
        cursor_x = x_pos + 4;
        cursor_y = y_pos + 28;
        gprint(label, COL_TEXT_BRIGHT);
    }

    /* Status badge (small dot) */
    int status_x = x_pos + APP_TILE_W - 6;
    int status_y = y_pos + 6;
    uint32_t status_color = APP_STATUS_IDLE;
    switch (app->status) {
        case APP_STATUS_OFFLINE:
            status_color = COL_TEXT_DIM;
            break;
        case APP_STATUS_IDLE:
            status_color = COL_SYNAPSE_CYAN;
            break;
        case APP_STATUS_RUNNING:
            status_color = COL_DATA_GREEN;
            break;
        case APP_STATUS_ALERT:
            status_color = COL_CRITICAL_ORANGE;
            break;
    }
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            put_pixel(status_x + dx, status_y + dy, status_color);
        }
    }

    /* Border */
    draw_hline(y_pos, x_pos, x_pos + APP_TILE_W, border_color);
    draw_hline(y_pos + APP_TILE_H - 1, x_pos, x_pos + APP_TILE_W, border_color);
    draw_vline(x_pos, y_pos, y_pos + APP_TILE_H, border_color);
    draw_vline(x_pos + APP_TILE_W - 1, y_pos, y_pos + APP_TILE_H, border_color);
}

void launcher_render(void) {
        wm_get_taskbar_icon_center(0, &launcher_state.os_button_x, &launcher_state.os_button_y);

    if (launcher_state.is_open == 0 && launcher_state.animation_state == 0) {
        return;  /* Launcher is closed */
    }

    /* Calculate animation progress */
    uint32_t elapsed = tick - launcher_state.animation_tick;
    int progress = (elapsed * 256) / LAUNCHER_ANIMATION_SPEED;
    if (progress > 256) progress = 256;

    if (launcher_state.animation_state == 1 && progress >= 256) {
        launcher_state.animation_state = 2;  /* Now fully open */
    }
    if (launcher_state.animation_state == 3 && progress >= 256) {
        launcher_state.animation_state = 0;  /* Now fully closed */
        launcher_state.is_open = 0;
        return;
    }

    /* Draw background overlay with fade */
    int alpha = (progress * 200) / 256;
    if (alpha > 200) alpha = 200;
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            uint32_t existing = backbuffer[y * SCREEN_WIDTH + x];
            uint32_t darkened = ((existing >> 1) & 0x7F7F7F);
            backbuffer[y * SCREEN_WIDTH + x] = darkened;
        }
    }

    /* Draw ripple effect from OS button */
    int ripple_radius = (LAUNCHER_RIPPLE_RADIUS * progress) / 256;
    draw_ripple_circle(launcher_state.os_button_x, launcher_state.os_button_y,
                       ripple_radius, COL_RIPPLE, progress);

    /* Draw launcher panel background */
    clear_region(LAUNCHER_CONTENT_X - 8, LAUNCHER_CONTENT_Y - 8,
                 LAUNCHER_CONTENT_X + LAUNCHER_CONTENT_W + 8,
                 LAUNCHER_CONTENT_Y + LAUNCHER_CONTENT_H + 8,
                 COL_BG_DARK);

    /* Header */
    clear_region(LAUNCHER_CONTENT_X, LAUNCHER_CONTENT_Y,
                 LAUNCHER_CONTENT_X + LAUNCHER_CONTENT_W,
                 LAUNCHER_CONTENT_Y + LAUNCHER_HEADER_H,
                 0x00182A);
    cursor_x = LAUNCHER_CONTENT_X + 10;
    cursor_y = LAUNCHER_CONTENT_Y + 10;
    gprint("NeuroDock", COL_ACCENT_CYAN);
    cursor_x = LAUNCHER_CONTENT_X + LAUNCHER_CONTENT_W - 210;
    cursor_y = LAUNCHER_CONTENT_Y + 10;
    gprint("Search: /", COL_TEXT_DIM);

    /* Draw panel border */
    draw_hline(LAUNCHER_CONTENT_Y - 8, LAUNCHER_CONTENT_X - 8,
               LAUNCHER_CONTENT_X + LAUNCHER_CONTENT_W + 8, COL_SYNAPSE_CYAN);
    draw_hline(LAUNCHER_CONTENT_Y + LAUNCHER_CONTENT_H + 8,
               LAUNCHER_CONTENT_X - 8,
               LAUNCHER_CONTENT_X + LAUNCHER_CONTENT_W + 8, COL_SYNAPSE_CYAN);
    draw_vline(LAUNCHER_CONTENT_X - 8, LAUNCHER_CONTENT_Y - 8,
               LAUNCHER_CONTENT_Y + LAUNCHER_CONTENT_H + 8, COL_SYNAPSE_CYAN);
    draw_vline(LAUNCHER_CONTENT_X + LAUNCHER_CONTENT_W + 8,
               LAUNCHER_CONTENT_Y - 8,
               LAUNCHER_CONTENT_Y + LAUNCHER_CONTENT_H + 8, COL_SYNAPSE_CYAN);

    /* Render app tiles by category */
    int tile_y = LAUNCHER_CONTENT_Y + LAUNCHER_HEADER_H + 12;
    int panel_bottom = LAUNCHER_CONTENT_Y + LAUNCHER_CONTENT_H - 10;

    for (int cat = 0; cat < APP_CAT_COUNT; cat++) {
        int tile_x = LAUNCHER_CONTENT_X + 8;
        int col_count = 0;
        int apps_in_cat = 0;

        /* Skip empty categories */
        for (int app_idx = 0; app_idx < launcher_app_count; app_idx++) {
            if (launcher_apps[app_idx].category == cat) {
                apps_in_cat = 1;
                break;
            }
        }
        if (!apps_in_cat) {
            continue;
        }

        if (tile_y + 20 >= panel_bottom) {
            break;
        }

        /* Category header */
        cursor_x = tile_x;
        cursor_y = tile_y;
        gprint((char *)category_names[cat], COL_ACCENT_CYAN);
        tile_y += 24;

        if (tile_y + APP_TILE_H >= panel_bottom) {
            break;
        }

        /* Render apps in this category */
        for (int app_idx = 0; app_idx < launcher_app_count; app_idx++) {
            LauncherAppEntry *app = &launcher_apps[app_idx];
            if (app->category != cat) continue;

            if (tile_y + APP_TILE_H >= panel_bottom) {
                break;
            }

            int hovered = (app_idx == launcher_state.hover_app_idx) ? 1 : 0;
            draw_app_tile(tile_x, tile_y, app, hovered);

            tile_x += APP_TILE_W + APP_TILE_PAD;
            col_count++;

            if (col_count >= LAUNCHER_GRID_COLS) {
                col_count = 0;
                tile_x = LAUNCHER_CONTENT_X;
                tile_y += APP_TILE_H + APP_TILE_PAD;
            }
        }

        if (col_count != 0) {
            tile_y += APP_TILE_H + APP_TILE_PAD;
        }
        tile_y += 8;  /* Space between categories */
    }

    /* Draw search bar if needed (not yet implemented) */
    if (launcher_state.search_active) {
        clear_region(LAUNCHER_CONTENT_X, LAUNCHER_CONTENT_Y - 40,
                     LAUNCHER_CONTENT_X + LAUNCHER_CONTENT_W, LAUNCHER_CONTENT_Y - 8,
                     0x002244);
        cursor_x = LAUNCHER_CONTENT_X + 8;
        cursor_y = LAUNCHER_CONTENT_Y - 36;
        gprint("Search: ", COL_TEXT_DIM);
        gprint(launcher_state.search_buffer, COL_TEXT_BRIGHT);
    }
}

/* ================================================================
 * Event Handling
 * ================================================================ */

void launcher_handle_mouse(int mx, int my, int buttons, int prev_buttons) {
    if (launcher_state.animation_state == 0) return;  /* Launcher not visible */

    /* Keep clicks local to panel bounds */
    if (mx < LAUNCHER_CONTENT_X - 8 || mx >= LAUNCHER_CONTENT_X + LAUNCHER_CONTENT_W + 8 ||
        my < LAUNCHER_CONTENT_Y - 8 || my >= LAUNCHER_CONTENT_Y + LAUNCHER_CONTENT_H + 8) {
        launcher_state.hover_app_idx = -1;
        if (buttons && !prev_buttons) {
            launcher_close();
        }
        return;
    }

    /* Check if mouse is over any app tile using same layout as render() */
    {
        int tile_y = LAUNCHER_CONTENT_Y + LAUNCHER_HEADER_H + 12;
        int panel_bottom = LAUNCHER_CONTENT_Y + LAUNCHER_CONTENT_H - 10;

        launcher_state.hover_app_idx = -1;

        for (int cat = 0; cat < APP_CAT_COUNT; cat++) {
            int tile_x = LAUNCHER_CONTENT_X + 8;
            int col_count = 0;
            int apps_in_cat = 0;

            for (int i = 0; i < launcher_app_count; i++) {
                if (launcher_apps[i].category == cat) {
                    apps_in_cat = 1;
                    break;
                }
            }
            if (!apps_in_cat) {
                continue;
            }

            if (tile_y + 20 >= panel_bottom) {
                break;
            }

            tile_y += 24;  /* Category header */

            if (tile_y + APP_TILE_H >= panel_bottom) {
                break;
            }

            for (int app_idx = 0; app_idx < launcher_app_count; app_idx++) {
                LauncherAppEntry *app = &launcher_apps[app_idx];
                if (app->category != cat) continue;

                if (tile_y + APP_TILE_H >= panel_bottom) {
                    break;
                }

                if (mx >= tile_x && mx < tile_x + APP_TILE_W &&
                    my >= tile_y && my < tile_y + APP_TILE_H) {
                    launcher_state.hover_app_idx = app_idx;
                    if (buttons && !prev_buttons) {
                        launcher_launch_app(app_idx);
                        launcher_close();
                    }
                    return;
                }

                tile_x += APP_TILE_W + APP_TILE_PAD;
                col_count++;
                if (col_count >= LAUNCHER_GRID_COLS) {
                    col_count = 0;
                    tile_x = LAUNCHER_CONTENT_X + 8;
                    tile_y += APP_TILE_H + APP_TILE_PAD;
                }
            }

            if (col_count != 0) {
                tile_y += APP_TILE_H + APP_TILE_PAD;
            }
            tile_y += 8;
        }
    }
}

void launcher_handle_key(char key) {
    if (key == 27) {  /* ESC */
        launcher_close();
    } else if (key == '/' && !launcher_state.search_active) {
        launcher_state.search_active = 1;
        launcher_state.search_buffer[0] = '\0';
        launcher_state.search_cursor = 0;
    }
}

void launcher_launch_app(int app_idx) {
    if (app_idx < 0 || app_idx >= launcher_app_count) return;

    LauncherAppEntry *app = &launcher_apps[app_idx];
    launcher_mark_recently_used(app_idx);

    /* Dock-surface apps map to WM icon slots and should open actual windows. */
    if (app->name[0] == 'L') { /* Launcher */
        wm_launch_icon_slot(1);
        launcher_set_app_status(app_idx, APP_STATUS_RUNNING);
        return;
    }
    if (app->name[0] == 'N') { /* Neural Viz */
        wm_launch_icon_slot(2);
        launcher_set_app_status(app_idx, APP_STATUS_RUNNING);
        return;
    }
    if (app->name[0] == 'C') { /* Console */
        wm_launch_icon_slot(3);
        launcher_set_app_status(app_idx, APP_STATUS_RUNNING);
        return;
    }
    if (app->name[0] == 'P') { /* Profiler */
        wm_launch_icon_slot(4);
        launcher_set_app_status(app_idx, APP_STATUS_RUNNING);
        return;
    }
    if (app->name[0] == 'S' && app->name[1] == 'e') { /* Settings */
        wm_launch_icon_slot(5);
        launcher_set_app_status(app_idx, APP_STATUS_RUNNING);
        return;
    }
    if (app->name[0] == 'M' && app->name[1] == 'o') { /* Model Manager */
        wm_open_model_manager();
        launcher_set_app_status(app_idx, APP_STATUS_RUNNING);
        return;
    }
    if (app->name[0] == 'S' && app->name[1] == 'p') { /* Spike Monitor */
        wm_open_spike_monitor();
        launcher_set_app_status(app_idx, APP_STATUS_RUNNING);
        return;
    }
    if (app->name[0] == 'S' && app->name[1] == 'n') { /* Snapshot Browser */
        wm_open_snapshot_browser();
        launcher_set_app_status(app_idx, APP_STATUS_RUNNING);
        return;
    }
    if (app->name[0] == 'R' && app->name[1] == 'e') { /* Replay Control */
        wm_open_replay_control();
        launcher_set_app_status(app_idx, APP_STATUS_RUNNING);
        return;
    }

    if (app->launch_fn) {
        app->launch_fn();
    } else {
        /* TODO: spawn userspace process from app->exec_path */
        /* For now, just mark as running */
        launcher_set_app_status(app_idx, APP_STATUS_RUNNING);
    }
}
