#ifndef USB_CDC_H
#define USB_CDC_H

#include "usb_core.h"

int usb_cdc_bind_device(UsbDeviceInfo *dev);
void usb_cdc_unbind_device(uint32_t device_id);
uint32_t usb_cdc_bound_count(void);

#endif
