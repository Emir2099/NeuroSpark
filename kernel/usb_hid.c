#include "usb_hid.h"

#include "input.h"

extern volatile int mouse_x;
extern volatile int mouse_y;

static uint32_t g_hid_bound = 0;
static uint32_t g_hid_last_device = 0;

int usb_hid_bind_device(UsbDeviceInfo *dev) {
  if (dev == 0 || dev->class_code != 0x03) {
    return 0;
  }

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
  if (g_hid_bound == 0) {
    return;
  }

  /* Integration hook: keep pointer within screen constraints if needed. */
  if (mouse_x < 0) {
    mouse_x = 0;
  }
  if (mouse_y < 0) {
    mouse_y = 0;
  }
}

uint32_t usb_hid_bound_count(void) { return g_hid_bound; }
