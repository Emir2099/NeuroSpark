#ifndef LAUNCHER_H
#define LAUNCHER_H

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

/* ================================================================
 * NeuroDock Launcher - App Registry and System
 *
 * Implements a neuromorphic-themed application launcher that:
 *   • Shows apps organized by cognitive/workflow categories
 *   • Animates ripple/pulse from OS button
 *   • Supports search, pinned favorites, and recent apps
 *   • Maintains live status badges for active experiments
 * ================================================================ */

/* App Categories */
#define APP_CAT_EXPERIMENTATION 0
#define APP_CAT_MONITORING      1
#define APP_CAT_STORAGE         2
#define APP_CAT_NETWORKING      3
#define APP_CAT_TOOLS           4
#define APP_CAT_SYSTEM          5

#define APP_CAT_COUNT           6

/* App Status Flags */
#define APP_STATUS_OFFLINE      0
#define APP_STATUS_IDLE         1
#define APP_STATUS_RUNNING      2
#define APP_STATUS_ALERT        3

/* App Entry in Registry */
typedef struct {
    char name[32];           /* "Model Manager", "Spike Monitor", etc. */
    int category;            /* APP_CAT_* */
    char icon_char;          /* Single character icon (M, S, D, N, T, G) */
    uint32_t icon_color;     /* Icon display color */
    int pinned;              /* 1 if in favorites */
    int recently_used;       /* Timestamp of last launch */
    int status;              /* APP_STATUS_* */
    void (*launch_fn)(void); /* Function to invoke on launch (NULL for userspace) */
    char exec_path[64];      /* Path for userspace apps (e.g. /bin/model) */
} LauncherAppEntry;

/* Launcher Global State */
typedef struct {
    int is_open;                    /* 0 = closed, 1 or more = animation frame */
    int animation_state;            /* 0 = idle, 1 = opening, 2 = open, 3 = closing */
    uint32_t animation_tick;        /* Tick when animation started */
    int scroll_offset;              /* Vertical offset for category scrolling */
    int search_active;              /* 1 if search mode */
    char search_buffer[32];         /* Search query */
    int search_cursor;              /* Position in search buffer */
    int hover_app_idx;              /* Currently hovered app index */
    int os_button_x, os_button_y;   /* OS button position for ripple origin */
} LauncherState;

extern LauncherAppEntry launcher_apps[32];
extern int launcher_app_count;
extern LauncherState launcher_state;

/* Core launcher API */
void launcher_init(void);
void launcher_toggle(void);
void launcher_open(void);
void launcher_close(void);
void launcher_render(void);
void launcher_handle_mouse(int mx, int my, int buttons, int prev_buttons);
void launcher_handle_key(char key);
void launcher_register_app(const char *name, int category, char icon_char, 
                           uint32_t icon_color, void (*launch_fn)(void), 
                           const char *exec_path);
void launcher_launch_app(int app_idx);
void launcher_set_app_status(int app_idx, int status);
void launcher_mark_recently_used(int app_idx);

#endif
