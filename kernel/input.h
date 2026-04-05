#ifndef INPUT_H
#define INPUT_H

/* ============================================================
 * Input Stack Completion (Canonical Input Routing)
 * ============================================================
 *
 * INPUT PIPELINE (Final):
 *
 *   Hardware IRQ1 (keyboard)
 *     └─> interrupt.asm IRQ1 stub
 *     └─> keyboard_handler() [input.c]
 *         • Scans PS/2 keyboard port (0x60)
 *         • Updates kb_buf[] (circular keyboard buffer)
 *         • Routes to wm_focused_needs_keyboard() check
 *         • Sends to WM console + kernel input buffer
 *
 *   Hardware IRQ12 (PS/2 mouse)
 *     └─> interrupt.asm IRQ12 stub
 *     └─> mouse_handler() [input.c]
 *         • Scans PS/2 mouse port (0x60), accumulates 3-byte packets
 *         • Updates mouse_x, mouse_y, mouse_buttons
 *         • Calls wm_handle_mouse() [wm.c] for WM routing
 *
 *   WM Mouse Handler (wm_handle_mouse, wm.c)
 *     └─> click routing by location:
 *         • Taskbar: icon clicks for app launch/hide/power
 *         • Title bar: drag window with bounds clamping
 *         • Title buttons: minimize/maximize/close
 *         • Content area: focus window
 *         • Power menu: reboot/shutdown actions
 *         • Deterministic z-order: topmost window gets click
 *
 * CONSOLIDATION:
 *   - Canonical runtime input path is input.c -> wm.c
 *   - Dashboard.c legacy wm_handle_clicks() is non-runtime/unused
 *   - No duplicate active runtime input paths
 * ============================================================ */

void keyboard_handler(void);
void mouse_handler(void);
void init_input_stack(void);

extern volatile int mouse_x;   /* Current mouse X coordinate (0-799) */
extern volatile int mouse_y;   /* Current mouse Y coordinate (0-599) */
extern volatile int mouse_buttons;  /* Bitmask: bit0=left, bit1=right, bit2=middle */

#endif
