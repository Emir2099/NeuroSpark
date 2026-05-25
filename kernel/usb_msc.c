#include "usb_msc.h"

#include "vfs.h"

static uint32_t g_msc_bound = 0;
static int g_msc_storage_hook = 0;

int usb_msc_bind_device(UsbDeviceInfo *dev) {
  if (dev == 0 || dev->class_code != 0x08) {
    return 0;
  }

  dev->driver = USB_DRV_MSC;
  dev->quarantined = 0;
  g_msc_bound++;
  /* Storage/VFS hook point for BOT mount flow. */
  g_msc_storage_hook = 1;
  return 1;
}

void usb_msc_unbind_device(uint32_t device_id) {
  (void)device_id;
  if (g_msc_bound > 0) {
    g_msc_bound--;
  }
  if (g_msc_bound == 0) {
    g_msc_storage_hook = 0;
  }
}

int usb_msc_storage_hook_ready(void) {
  return g_msc_storage_hook ? 1 : 0;
}

uint32_t usb_msc_bound_count(void) { return g_msc_bound; }
