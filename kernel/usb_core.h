#ifndef USB_CORE_H
#define USB_CORE_H

#ifndef _UINT8_T_DEFINED
#define _UINT8_T_DEFINED
typedef unsigned char uint8_t;
#endif
#ifndef _UINT32_T_DEFINED
#define _UINT32_T_DEFINED
typedef unsigned int uint32_t;
#endif
#ifndef _UINT16_T_DEFINED
#define _UINT16_T_DEFINED
typedef unsigned short uint16_t;
#endif

#define USB_MAX_DEVICES 16

enum {
  USB_DRV_NONE = 0,
  USB_DRV_HID = 1,
  USB_DRV_MSC = 2,
  USB_DRV_CDC = 3
};

typedef struct {
  uint32_t id;
  uint32_t address;
  uint32_t port;
  uint8_t class_code;
  uint16_t vendor_id;
  uint16_t product_id;
  uint32_t driver;
  uint32_t quarantined;
} UsbDeviceInfo;

typedef struct {
  uint32_t enumerate_cycles;
  uint32_t attach_events;
  uint32_t detach_events;
  uint32_t reset_ok;
  uint32_t reset_fail;
  uint32_t stall_recoveries;
  uint32_t retry_events;
  uint32_t suspend_count;
  uint32_t resume_count;
  uint32_t quarantine_events;
  uint32_t hotplug_test_cycles;
  uint32_t hotplug_test_failures;
  uint32_t last_error_class;
  uint32_t last_error_code;
  uint32_t errors;
} UsbCoreStats;

void usb_core_init(void);
void usb_core_poll(void);
int usb_core_is_ready(void);
int usb_core_get_devices(UsbDeviceInfo *out_devices, int max_count);
void usb_core_get_stats(UsbCoreStats *out_stats);
int usb_core_reset_device(uint32_t device_id);
int usb_core_reset_all(void);
int usb_core_recover_stall(uint32_t device_id);
int usb_core_inject_fault(uint32_t device_id, uint32_t error_code);
int usb_core_suspend(void);
int usb_core_resume(void);
int usb_core_run_hotplug_regression(uint32_t cycles);
const char *usb_core_driver_name(uint32_t driver);

#endif
