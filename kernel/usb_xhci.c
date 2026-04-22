#include "usb_xhci.h"

#include "pci.h"

static UsbXhciReport g_xhci;

void usb_xhci_init_from_pci(void) {
  int i;

  g_xhci.bus = 0;
  g_xhci.slot = 0;
  g_xhci.function = 0;
  g_xhci.mmio_base = 0;
  g_xhci.ports_total = 0;
  g_xhci.ports_connected = 0;
  g_xhci.enumerate_cycles = 0;
  g_xhci.hotplug_events = 0;
  g_xhci.reset_ok = 0;
  g_xhci.reset_fail = 0;
  g_xhci.stall_recoveries = 0;
  g_xhci.retry_events = 0;
  g_xhci.suspend_count = 0;
  g_xhci.resume_count = 0;
  g_xhci.quarantined_ports = 0;
  g_xhci.ready = 0;

  for (i = 0; i < pci_found_count; i++) {
    PciDevice *d = &pci_found[i];
    if (d->class_code == 0x0C && d->subclass == 0x03) {
      g_xhci.bus = d->bus;
      g_xhci.slot = d->slot;
      g_xhci.function = d->function;
      g_xhci.mmio_base = pci_read_bar(i, 0);
      if (g_xhci.mmio_base == 0) {
        g_xhci.mmio_base = pci_read_bar(i, 1);
      }
      g_xhci.ports_total = 8;
      g_xhci.ports_connected = 0;
      g_xhci.ready = 1;
      return;
    }
  }
}

void usb_xhci_poll(void) {
  if (!g_xhci.ready) {
    return;
  }
}

int usb_xhci_is_ready(void) { return g_xhci.ready ? 1 : 0; }

int usb_xhci_reset_port(uint32_t port_index) {
  if (!g_xhci.ready || port_index >= g_xhci.ports_total) {
    g_xhci.reset_fail++;
    return 0;
  }
  g_xhci.reset_ok++;
  return 1;
}

int usb_xhci_reset_all_ports(void) {
  if (!g_xhci.ready) {
    g_xhci.reset_fail++;
    return 0;
  }
  g_xhci.reset_ok += g_xhci.ports_total;
  return 1;
}

void usb_xhci_note_hotplug_event(uint32_t count) {
  if (!g_xhci.ready) {
    return;
  }
  g_xhci.hotplug_events += count;
}

void usb_xhci_note_stall_recovery(void) {
  if (!g_xhci.ready) {
    return;
  }
  g_xhci.stall_recoveries++;
}

void usb_xhci_note_retry(void) {
  if (!g_xhci.ready) {
    return;
  }
  g_xhci.retry_events++;
}

int usb_xhci_suspend(void) {
  if (!g_xhci.ready) {
    return 0;
  }
  g_xhci.suspend_count++;
  return 1;
}

int usb_xhci_resume(void) {
  if (!g_xhci.ready) {
    return 0;
  }
  g_xhci.resume_count++;
  return 1;
}

void usb_xhci_set_quarantined_ports(uint32_t quarantined_ports) {
  if (!g_xhci.ready) {
    return;
  }
  g_xhci.quarantined_ports = quarantined_ports;
}

void usb_xhci_note_enumeration(void) {
  if (!g_xhci.ready) {
    return;
  }
  g_xhci.enumerate_cycles++;
}

void usb_xhci_get_report(UsbXhciReport *out_report) {
  if (out_report == 0) {
    return;
  }
  *out_report = g_xhci;
}
