#ifndef USB_MSC_H
#define USB_MSC_H

#include "usb_core.h"

int usb_msc_bind_device(UsbDeviceInfo *dev);
void usb_msc_unbind_device(uint32_t device_id);
int usb_msc_storage_hook_ready(void);
uint32_t usb_msc_bound_count(void);

#endif
