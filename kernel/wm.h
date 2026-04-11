#ifndef WM_H
#define WM_H

/* Power and Time Correctness - RTC/Timezone Persistence
 *
 * TIMEZONE STORAGE:
 *   - Stored in CMOS registers with magic signature validation
 *   - Registers: 0x38 (sig_a='T'), 0x39 (sig_b='Z'), 0x3A-0x3B (offset)
 *   - Range: -720 to +840 minutes (UTC-12 to UTC+14 coverage)
 *   - Boot-time load: wm_init() calls wm_load_timezone_from_cmos()
 *   - Runtime save: wm_set_timezone_offset_minutes() via shell "tz" command
 *
 * RTC TIME SOURCE:
 *   - CMOS registers: 0x00 (sec), 0x02 (min), 0x04 (hour), 0x0A (status)
 *   - BCD format conversion via bcd_to_bin()
 *   - read_rtc_hm() fetches current HH:MM for WM clock display
 *
 * UTC vs LOCAL TIME:
 *   - QEMU RTC defaults to UTC (BIOS CLOCK=utc)
 *   - wm_tz_offset_minutes applied at display time (not stored on disk)
 *   - Session snapshots preserve offset via CMOS persistence across reboot
 *
 * POWER CONTROL:
 *   - Reboot via 8042 keyboard controller (0x64:0xFE) + PCI fallbacks
 *   - Shutdown via QEMU ACPI port (0x604, 0xB004) + Bochs/VBox variants
 */

#ifndef _UINT32_T_DEFINED
#define _UINT32_T_DEFINED
typedef unsigned int uint32_t;
#endif
#ifndef _UINT8_T_DEFINED
#define _UINT8_T_DEFINED
typedef unsigned char uint8_t;
#endif

#define WM_MAX_WINDOWS    8
#define WM_STATE_NORMAL   0
#define WM_STATE_MINIMIZED 1
#define WM_STATE_MAXIMIZED 2

#define WM_TITLEBAR_H    22
#define WM_BORDER         2
#define WM_TASKBAR_H     54
#define WM_TASKBAR_Y     (600 - WM_TASKBAR_H)

#define WM_BTN_W         16
#define WM_BTN_H         16
#define WM_BTN_PAD        3

#define WM_ICON_SLOTS     6

typedef void (*wm_draw_fn)(int x, int y, int w, int h);

typedef struct {
    int x, y, w, h;
    int ox, oy, ow, oh;
    char title[48];
    int state;
    int visible;
    int dragging;
    int drag_ox, drag_oy;
    wm_draw_fn draw_fn;
    int needs_keyboard;
    int icon_slot;
} WmWindow;

extern WmWindow wm_windows[WM_MAX_WINDOWS];
extern int wm_window_count;
extern int wm_focused;

void wm_init(void);
int  wm_add_window(int x, int y, int w, int h, const char *title,
                    wm_draw_fn fn, int needs_kb, int icon_slot);
void wm_render(void);
void wm_handle_mouse(int mx, int my, int buttons, int prev_buttons);
int  wm_focused_needs_keyboard(void);
void wm_get_runtime_metrics(uint32_t *frames_total, uint32_t *fps_x10);
int  wm_set_timezone_offset_minutes(int minutes);
int  wm_get_timezone_offset_minutes(void);
void wm_get_taskbar_icon_center(int slot, int *out_x, int *out_y);
void wm_launch_icon_slot(int slot);
void wm_open_model_manager(void);
void wm_open_spike_monitor(void);

#endif
