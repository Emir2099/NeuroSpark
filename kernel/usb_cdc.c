#include "usb_cdc.h"

static uint32_t g_cdc_bound = 0;

int usb_cdc_bind_device(UsbDeviceInfo *dev) {
  if (dev == 0 || (dev->class_code != 0x02 && dev->class_code != 0x0A)) {
    return 0;
  }

  dev->driver = USB_DRV_CDC;
  dev->quarantined = 0;
  g_cdc_bound++;
  return 1;
}

void usb_cdc_unbind_device(uint32_t device_id) {
  (void)device_id;
  if (g_cdc_bound > 0) {
    g_cdc_bound--;
  }
}

uint32_t usb_cdc_bound_count(void) { return g_cdc_bound; }
