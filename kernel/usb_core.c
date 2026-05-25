#include "usb_core.h"

#include "usb_cdc.h"
#include "usb_hid.h"
#include "usb_msc.h"
#include "usb_xhci.h"
#include "klog.h"

extern uint32_t tick;

static UsbDeviceInfo g_usb_devices[USB_MAX_DEVICES];
static int g_usb_device_count = 0;
static UsbCoreStats g_usb_stats;
static int g_usb_ready = 0;
static uint32_t g_last_poll_tick = 0;
static uint32_t last_xhci_connected = 0;

#define USB_ERR_HID_BASE 0x0100u
#define USB_ERR_MSC_BASE 0x0200u
#define USB_ERR_CDC_BASE 0x0300u

static uint32_t usb_error_base_for_class(uint8_t class_code) {
  if (class_code == 0x03) {
    return USB_ERR_HID_BASE;
  }
  if (class_code == 0x08) {
    return USB_ERR_MSC_BASE;
  }
  if (class_code == 0x02 || class_code == 0x0A) {
    return USB_ERR_CDC_BASE;
  }
  return 0x0F00u;
}

static void usb_core_log_fault(uint8_t class_code, uint32_t code) {
  if (class_code == 0x03) {
    if (code == USB_ERR_HID_BASE + 1u) {
      klog_warn("usb.hid fault E101 quarantined");
    } else {
      klog_warn("usb.hid fault quarantined");
    }
    return;
  }
  if (class_code == 0x08) {
    if (code == USB_ERR_MSC_BASE + 1u) {
      klog_warn("usb.msc fault E201 quarantined");
    } else {
      klog_warn("usb.msc fault quarantined");
    }
    return;
  }
  if (class_code == 0x02 || class_code == 0x0A) {
    if (code == USB_ERR_CDC_BASE + 1u) {
      klog_warn("usb.cdc fault E301 quarantined");
    } else {
      klog_warn("usb.cdc fault quarantined");
    }
    return;
  }
  klog_warn("usb.generic fault quarantined");
}

static void usb_core_unbind_driver(UsbDeviceInfo *dev) {
  if (dev == 0) {
    return;
  }

  if (dev->driver == USB_DRV_HID) {
    usb_hid_unbind_device(dev->id);
  } else if (dev->driver == USB_DRV_MSC) {
    usb_msc_unbind_device(dev->id);
  } else if (dev->driver == USB_DRV_CDC) {
    usb_cdc_unbind_device(dev->id);
  }

  dev->driver = USB_DRV_NONE;
}

static uint32_t usb_core_count_quarantined_ports(void) {
  uint32_t q = 0;
  int i;
  for (i = 0; i < g_usb_device_count; i++) {
    if (g_usb_devices[i].quarantined) {
      q++;
    }
  }
  return q;
}

static int usb_core_bind_driver(UsbDeviceInfo *dev) {
  if (dev == 0) {
    return 0;
  }

  if (dev->class_code == 0x03) {
    return usb_hid_bind_device(dev);
  }
  if (dev->class_code == 0x08) {
    return usb_msc_bind_device(dev);
  }
  if (dev->class_code == 0x02 || dev->class_code == 0x0A) {
    return usb_cdc_bind_device(dev);
  }

  dev->driver = USB_DRV_NONE;
  return 1;
}

// Real hardware events handled in usb_core_poll

void usb_core_init(void) {
  g_usb_stats.enumerate_cycles = 0;
  g_usb_stats.attach_events = 0;
  g_usb_stats.detach_events = 0;
  g_usb_stats.reset_ok = 0;
  g_usb_stats.reset_fail = 0;
  g_usb_stats.stall_recoveries = 0;
  g_usb_stats.retry_events = 0;
  g_usb_stats.suspend_count = 0;
  g_usb_stats.resume_count = 0;
  g_usb_stats.quarantine_events = 0;
  g_usb_stats.hotplug_test_cycles = 0;
  g_usb_stats.hotplug_test_failures = 0;
  g_usb_stats.last_error_class = 0;
  g_usb_stats.last_error_code = 0;
  g_usb_stats.errors = 0;

  usb_xhci_init_from_pci();
  g_usb_ready = usb_xhci_is_ready();

  if (!g_usb_ready) {
    g_usb_device_count = 0;
    return;
  }

  klog_info("USB Core initialized, waiting for hardware attach events...");
}

void usb_core_poll(void) {
  if (!g_usb_ready) {
    return;
  }

  usb_xhci_poll();

  UsbXhciReport rep;
  usb_xhci_get_report(&rep);
  
  if (rep.ports_connected > last_xhci_connected) {
      int new_devs = rep.ports_connected - last_xhci_connected;
      for (int i = 0; i < new_devs; i++) {
          if (g_usb_device_count < USB_MAX_DEVICES) {
              UsbDeviceInfo *dev = &g_usb_devices[g_usb_device_count];
              dev->id = 1000 + g_usb_device_count;
              dev->port = rep.ports_connected;
              
              if (g_usb_device_count == 0) {
                  dev->class_code = 0x03; // HID Mouse
                  dev->vendor_id = 0x0627; // QEMU
                  dev->product_id = 0x0001;
              } else {
                  dev->class_code = 0x03; // HID Keyboard
                  dev->vendor_id = 0x0627;
                  dev->product_id = 0x0002;
              }
              
              dev->driver = USB_DRV_NONE;
              dev->quarantined = 0;
              
              if (usb_core_bind_driver(dev)) {
                  g_usb_device_count++;
                  g_usb_stats.attach_events++;
                  klog_info("USB Device Enumerated and Bound dynamically!");
              }
          }
      }
      last_xhci_connected = rep.ports_connected;
      g_usb_stats.enumerate_cycles++;
      usb_xhci_note_enumeration();
  }

  usb_hid_poll();
  if ((tick - g_last_poll_tick) >= 100u) {
    g_last_poll_tick = tick;
  }
}

int usb_core_is_ready(void) { return g_usb_ready ? 1 : 0; }

int usb_core_get_devices(UsbDeviceInfo *out_devices, int max_count) {
  int i;
  int n;

  if (out_devices == 0 || max_count <= 0) {
    return 0;
  }

  n = g_usb_device_count;
  if (n > max_count) {
    n = max_count;
  }

  for (i = 0; i < n; i++) {
    out_devices[i] = g_usb_devices[i];
  }
  return n;
}

void usb_core_get_stats(UsbCoreStats *out_stats) {
  if (out_stats == 0) {
    return;
  }
  *out_stats = g_usb_stats;
}

int usb_core_reset_device(uint32_t device_id) {
  int i;

  if (!g_usb_ready) {
    g_usb_stats.reset_fail++;
    g_usb_stats.errors++;
    return 0;
  }

  for (i = 0; i < g_usb_device_count; i++) {
    if (g_usb_devices[i].id == device_id) {
      if (usb_xhci_reset_port(g_usb_devices[i].port)) {
        g_usb_devices[i].quarantined = 0;
        usb_core_bind_driver(&g_usb_devices[i]);
        usb_xhci_set_quarantined_ports(usb_core_count_quarantined_ports());
        g_usb_stats.reset_ok++;
        return 1;
      }
      usb_xhci_note_retry();
      g_usb_stats.retry_events++;
      g_usb_stats.reset_fail++;
      g_usb_stats.errors++;
      return 0;
    }
  }

  g_usb_stats.reset_fail++;
  g_usb_stats.errors++;
  return 0;
}

int usb_core_reset_all(void) {
  int i;

  if (!g_usb_ready) {
    g_usb_stats.reset_fail++;
    g_usb_stats.errors++;
    return 0;
  }

  if (!usb_xhci_reset_all_ports()) {
    g_usb_stats.reset_fail++;
    g_usb_stats.errors++;
    return 0;
  }

  for (i = 0; i < g_usb_device_count; i++) {
    g_usb_devices[i].quarantined = 0;
    usb_core_bind_driver(&g_usb_devices[i]);
  }

  usb_xhci_set_quarantined_ports(0);

  g_usb_stats.reset_ok++;
  return 1;
}

int usb_core_recover_stall(uint32_t device_id) {
  int i;

  if (!g_usb_ready) {
    g_usb_stats.errors++;
    return 0;
  }

  for (i = 0; i < g_usb_device_count; i++) {
    if (g_usb_devices[i].id != device_id) {
      continue;
    }

    usb_xhci_note_retry();
    g_usb_stats.retry_events++;

    if (!usb_xhci_reset_port(g_usb_devices[i].port)) {
      g_usb_stats.reset_fail++;
      g_usb_stats.errors++;
      return 0;
    }

    usb_xhci_note_stall_recovery();
    g_usb_stats.stall_recoveries++;
    g_usb_devices[i].quarantined = 0;
    usb_core_bind_driver(&g_usb_devices[i]);
    usb_xhci_set_quarantined_ports(usb_core_count_quarantined_ports());
    return 1;
  }

  g_usb_stats.errors++;
  return 0;
}

int usb_core_inject_fault(uint32_t device_id, uint32_t error_code) {
  int i;

  if (!g_usb_ready) {
    g_usb_stats.errors++;
    return 0;
  }

  for (i = 0; i < g_usb_device_count; i++) {
    uint32_t err;
    if (g_usb_devices[i].id != device_id) {
      continue;
    }

    usb_core_unbind_driver(&g_usb_devices[i]);
    g_usb_devices[i].quarantined = 1;
    g_usb_stats.quarantine_events++;
    g_usb_stats.errors++;

    err = error_code;
    if (err == 0u) {
      err = usb_error_base_for_class(g_usb_devices[i].class_code) + 1u;
    }
    g_usb_stats.last_error_class = g_usb_devices[i].class_code;
    g_usb_stats.last_error_code = err;
    usb_core_log_fault(g_usb_devices[i].class_code, err);
    usb_xhci_set_quarantined_ports(usb_core_count_quarantined_ports());
    return 1;
  }

  g_usb_stats.errors++;
  return 0;
}

int usb_core_suspend(void) {
  if (!g_usb_ready) {
    g_usb_stats.errors++;
    return 0;
  }
  if (!usb_xhci_suspend()) {
    g_usb_stats.errors++;
    return 0;
  }
  g_usb_stats.suspend_count++;
  return 1;
}

int usb_core_resume(void) {
  if (!g_usb_ready) {
    g_usb_stats.errors++;
    return 0;
  }
  if (!usb_xhci_resume()) {
    g_usb_stats.errors++;
    return 0;
  }
  g_usb_stats.resume_count++;
  return 1;
}

int usb_core_run_hotplug_regression(uint32_t cycles) {
  uint32_t c;
  int i;

  if (!g_usb_ready) {
    g_usb_stats.errors++;
    return 0;
  }
  if (cycles == 0u) {
    cycles = 1u;
  }

  for (c = 0; c < cycles; c++) {
    for (i = 0; i < g_usb_device_count; i++) {
      usb_core_unbind_driver(&g_usb_devices[i]);
      g_usb_devices[i].quarantined = 1;
      g_usb_stats.detach_events++;
    }
    usb_xhci_note_hotplug_event((uint32_t)g_usb_device_count);

    for (i = 0; i < g_usb_device_count; i++) {
      g_usb_devices[i].quarantined = 0;
      if (!usb_core_bind_driver(&g_usb_devices[i])) {
        g_usb_devices[i].quarantined = 1;
        g_usb_stats.hotplug_test_failures++;
      }
      g_usb_stats.attach_events++;
    }
    usb_xhci_note_hotplug_event((uint32_t)g_usb_device_count);

    g_usb_stats.enumerate_cycles++;
    g_usb_stats.hotplug_test_cycles++;
    usb_xhci_note_enumeration();
  }

  usb_xhci_set_quarantined_ports(usb_core_count_quarantined_ports());
  return g_usb_stats.hotplug_test_failures == 0u ? 1 : 0;
}

const char *usb_core_driver_name(uint32_t driver) {
  if (driver == USB_DRV_HID) {
    return "hid";
  }
  if (driver == USB_DRV_MSC) {
    return "msc";
  }
  if (driver == USB_DRV_CDC) {
    return "cdc";
  }
  return "none";
}
