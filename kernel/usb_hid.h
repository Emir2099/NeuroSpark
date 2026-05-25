#ifndef USB_HID_H
#define USB_HID_H

#include "usb_core.h"

int usb_hid_bind_device(UsbDeviceInfo *dev);
void usb_hid_unbind_device(uint32_t device_id);
void usb_hid_poll(void);
uint32_t usb_hid_bound_count(void);
void usb_hid_process_report(uint32_t slot_id, uint8_t *data, int len);

#endif
