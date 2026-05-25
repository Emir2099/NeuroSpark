#ifndef USB_XHCI_H
#define USB_XHCI_H

#ifndef _UINT32_T_DEFINED
#define _UINT32_T_DEFINED
typedef unsigned int uint32_t;
#endif

typedef struct {
  uint32_t bus;
  uint32_t slot;
  uint32_t function;
  uint32_t mmio_base;
  uint32_t ports_total;
  uint32_t ports_connected;
  uint32_t enumerate_cycles;
  uint32_t hotplug_events;
  uint32_t reset_ok;
  uint32_t reset_fail;
  uint32_t stall_recoveries;
  uint32_t retry_events;
  uint32_t suspend_count;
  uint32_t resume_count;
  uint32_t quarantined_ports;
  uint32_t ready;
} UsbXhciReport;

void usb_xhci_init_from_pci(void);
void usb_xhci_poll(void);
int usb_xhci_is_ready(void);
int usb_xhci_reset_port(uint32_t port_index);
int usb_xhci_reset_all_ports(void);
void usb_xhci_note_hotplug_event(uint32_t count);
void usb_xhci_note_stall_recovery(void);
void usb_xhci_note_retry(void);
int usb_xhci_suspend(void);
int usb_xhci_resume(void);
void usb_xhci_set_quarantined_ports(uint32_t quarantined_ports);
void usb_xhci_note_enumeration(void);
void usb_xhci_get_report(UsbXhciReport *out_report);

#endif
