#include "usb_hid.h"
#include "input.h"

extern volatile int mouse_x;
extern volatile int mouse_y;
extern volatile int mouse_buttons;
extern int screen_width;
extern int screen_height;
extern volatile char input_buffer[];
extern volatile int buffer_idx;
extern volatile int shell_dirty;

extern void wm_handle_mouse(int mx, int my, int buttons, int prev_buttons);
extern void process_command(char *cmd);

static uint32_t g_hid_bound = 0;
static uint32_t g_hid_last_device = 0;
static uint8_t last_key = 0;

int usb_hid_bind_device(UsbDeviceInfo *dev) {
  if (dev == 0 || dev->class_code != 0x03) return 0;
  dev->driver = USB_DRV_HID;
  dev->quarantined = 0;
  g_hid_bound++;
  g_hid_last_device = dev->id;
  return 1;
}

void usb_hid_unbind_device(uint32_t device_id) {
  if (g_hid_bound > 0 && g_hid_last_device == device_id) {
    g_hid_bound--;
  }
}

void usb_hid_poll(void) {
  if (g_hid_bound == 0) return;
  if (mouse_x < 0) mouse_x = 0;
  if (mouse_y < 0) mouse_y = 0;
}

uint32_t usb_hid_bound_count(void) { return g_hid_bound; }

static char hid_usage_to_ascii(uint8_t usage, uint8_t shift) {
    if (usage >= 0x04 && usage <= 0x1D) { // a-z
        if (shift) return 'A' + (usage - 0x04);
        else return 'a' + (usage - 0x04);
    }
    if (usage >= 0x1E && usage <= 0x27) { // 1-9, 0
        if (usage == 0x27) return shift ? ')' : '0';
        char c = '1' + (usage - 0x1E);
        if (shift) {
            char symbols[] = "!@#$%^&*(";
            return symbols[usage - 0x1E];
        }
        return c;
    }
    if (usage == 0x28) return '\n'; // Enter
    if (usage == 0x2A) return '\b'; // Backspace
    if (usage == 0x2C) return ' '; // Space
    if (usage == 0x2D) return shift ? '_' : '-'; // Minus
    if (usage == 0x2E) return shift ? '+' : '='; // Equal
    if (usage == 0x33) return shift ? ';' : ':'; // Semicolon
    if (usage == 0x34) return shift ? '"' : '\''; // Quote
    if (usage == 0x36) return shift ? '<' : ','; // Comma
    if (usage == 0x37) return shift ? '>' : '.'; // Period
    if (usage == 0x38) return shift ? '?' : '/'; // Slash
    return 0;
}

void usb_hid_process_report(uint32_t slot_id, uint8_t *data, int len) {
    if (len >= 8) {
        // Keyboard report (typically 8 bytes)
        uint8_t modifiers = data[0];
        uint8_t key1 = data[2];
        
        if (key1 == 0 && modifiers == 0) {
            last_key = 0;
            return; // Key release
        }
        if (key1 == last_key && key1 != 0) return; // Ignore repeat for now
        last_key = key1;
        
        if (key1 == 0) return; // Modifiers only changed, no key pressed
        
        uint8_t shift = (modifiers & 0x02) || (modifiers & 0x20); // LShift or RShift
        
        if (key1 == 0x28) { // Enter
            input_buffer[buffer_idx] = '\0';
            char cmd_copy[128]; // SHELL_CMD_MAX
            for (int i = 0; i <= buffer_idx && i < 128; i++) cmd_copy[i] = input_buffer[i];
            cmd_copy[127] = '\0';
            process_command(cmd_copy);
            buffer_idx = 0;
            shell_dirty = 1;
            return;
        }
        if (key1 == 0x2A && buffer_idx > 0) { // Backspace
            buffer_idx--;
            shell_dirty = 1;
            return;
        }
        
        char c = hid_usage_to_ascii(key1, shift);
        if (c && buffer_idx < 127) {
            input_buffer[buffer_idx++] = c;
            shell_dirty = 1;
        }
    } else if (len >= 3) {
        // Mouse report (typically 3 or 4 bytes)
        int buttons = data[0] & 0x07;
        int dx = (signed char)data[1];
        int dy = (signed char)data[2];
        
        mouse_x += dx;
        mouse_y += dy; 
        
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_x >= screen_width) mouse_x = screen_width - 1;
        if (mouse_y >= screen_height) mouse_y = screen_height - 1;
        
        wm_handle_mouse(mouse_x, mouse_y, buttons, mouse_buttons);
        mouse_buttons = buttons;
    }
}
