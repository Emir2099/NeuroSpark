#include "usb_xhci.h"
#include "pci.h"
#include "klog.h"
#include "paging.h"
#include "usb_hid.h"

extern void *pmm_alloc_page(void);
extern void kprint(const char *str, int row, int col, unsigned char color);

/* --- Serial debug helpers --- */
extern void outb(unsigned short port, unsigned char val);
static void ser(const char *s) { while (*s) outb(0x3F8, *s++); }
static void ser_hex32(uint32_t v) {
    char h[] = "0123456789ABCDEF";
    outb(0x3F8,'0'); outb(0x3F8,'x');
    for (int i = 28; i >= 0; i -= 4) outb(0x3F8, h[(v >> i) & 0xF]);
}
static void ser_dec(uint32_t v) {
    char buf[12]; int i = 0;
    if (v == 0) { outb(0x3F8, '0'); return; }
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i--) outb(0x3F8, buf[i]);
}

/* --- Register offsets --- */
#define XHCI_CAPLENGTH  0x00
#define XHCI_HCSPARAMS1 0x04
#define XHCI_HCCPARAMS1 0x10
#define XHCI_DBOFF      0x14
#define XHCI_RTSOFF     0x18
#define XHCI_USBCMD     0x00
#define XHCI_USBSTS     0x04
#define XHCI_CRCR       0x18
#define XHCI_DCBAAP     0x30
#define XHCI_CONFIG     0x38

#define USBCMD_RS   (1 << 0)
#define USBCMD_HCRST (1 << 1)

typedef struct {
    uint32_t param_low;
    uint32_t param_high;
    uint32_t status;
    uint32_t control;
} __attribute__((packed, aligned(16))) XhciTrb;

typedef struct {
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t size;
    uint32_t reserved;
} __attribute__((packed)) XhciErstEntry;

static UsbXhciReport g_xhci;
static uint32_t op_base;
static uint32_t rt_base;
static uint32_t db_base;
static uint32_t ctx_size = 32;

static uint64_t *dcbaa;
static XhciTrb *cmd_ring;
static uint32_t cmd_enqueue_idx = 0;
static uint32_t cmd_cycle = 1;

static XhciTrb *event_ring;
static uint32_t event_dequeue_idx = 0;
static uint32_t event_cycle = 1;
static XhciErstEntry *erst;

static inline uint32_t mmio_read32(uint32_t addr) { return *((volatile uint32_t *)addr); }
static inline void mmio_write32(uint32_t addr, uint32_t val) { *((volatile uint32_t *)addr) = val; }

/* Doorbell */
static void ring_doorbell(uint32_t slot_id, uint32_t target) {
    mmio_write32(db_base + (slot_id * 4), target);
}

/* ---------- Command Ring ---------- */
static void enqueue_cmd_trb(uint32_t low, uint32_t high, uint32_t status, uint32_t control) {
    control = (control & ~1u) | (cmd_cycle & 1);
    cmd_ring[cmd_enqueue_idx].param_low = low;
    cmd_ring[cmd_enqueue_idx].param_high = high;
    cmd_ring[cmd_enqueue_idx].status = status;
    cmd_ring[cmd_enqueue_idx].control = control;

    cmd_enqueue_idx++;
    if (cmd_enqueue_idx == 255) {
        /* Link TRB wrapping back to start */
        cmd_ring[255].param_low = (uint32_t)cmd_ring;
        cmd_ring[255].param_high = 0;
        cmd_ring[255].status = 0;
        cmd_ring[255].control = (6 << 10) | (1 << 1) | (cmd_cycle & 1); /* Toggle Cycle */
        cmd_cycle ^= 1;
        cmd_enqueue_idx = 0;
    }
    ring_doorbell(0, 0); /* Host Controller doorbell */
}

/* ---------- Per-slot state ---------- */
/* State machine:
 *  0 = unused
 *  1 = Enable Slot pending
 *  2 = Address Device pending
 *  3 = Configure Endpoint pending
 *  4 = Active (polling transfer ring)
 */
typedef struct {
    uint32_t state;
    uint32_t port;
    uint32_t speed;
    uint8_t *input_ctx;
    uint8_t *output_ctx;
    XhciTrb *transfer_ring;
    uint8_t *transfer_buffer;
    uint32_t tr_cycle;
    uint32_t tr_enqueue_idx;
    XhciTrb *ep0_ring;     /* EP0 transfer ring for control pipe */
} XhciSlot;

static XhciSlot slots[32];

/* Pending Enable Slot tracking:
 * xHCI Enable Slot TRB does NOT accept a Slot ID — the controller assigns one.
 * We maintain a FIFO of port numbers waiting for their assigned slot.
 */
#define MAX_PENDING 8
static uint32_t pending_ports[MAX_PENDING];
static uint32_t pending_speeds[MAX_PENDING];
static int pending_head = 0;
static int pending_tail = 0;

static void push_pending(uint32_t port, uint32_t speed) {
    pending_ports[pending_tail] = port;
    pending_speeds[pending_tail] = speed;
    pending_tail = (pending_tail + 1) % MAX_PENDING;
}

static int pop_pending(uint32_t *port, uint32_t *speed) {
    if (pending_head == pending_tail) return 0;
    *port = pending_ports[pending_head];
    *speed = pending_speeds[pending_head];
    pending_head = (pending_head + 1) % MAX_PENDING;
    return 1;
}

/* ---------- Enable Slot ---------- */
static void init_slot_for_port(uint32_t port, uint32_t speed) {
    push_pending(port, speed);
    /* Enable Slot Command: TRB Type = 9, Slot ID MUST be 0 (reserved) */
    enqueue_cmd_trb(0, 0, 0, (9 << 10));
    ser("[xHCI] Enable Slot for port "); ser_dec(port);
    ser(" speed "); ser_dec(speed); ser("\n");
    klog_info("xHCI: Sent Enable Slot Command");
}

/* ---------- Address Device ---------- */
static void address_device_slot(uint32_t slot_id) {
    if (slot_id == 0 || slot_id >= 32) return;
    XhciSlot *s = &slots[slot_id];

    /* Allocate Input + Output Contexts (page-aligned = 4096-aligned, exceeds 64-byte requirement) */
    s->input_ctx = (uint8_t*)pmm_alloc_page();
    s->output_ctx = (uint8_t*)pmm_alloc_page();
    for (int i = 0; i < 4096; i++) { s->input_ctx[i] = 0; s->output_ctx[i] = 0; }

    /* Point DCBAA entry to output context */
    dcbaa[slot_id] = (uint64_t)(uint32_t)s->output_ctx;

    /* Input Control Context (offset 0):
     *   DWORD 0 = Drop Context Flags = 0
     *   DWORD 1 = Add Context Flags  = A0 (Slot) | A1 (EP0) = 0x03
     */
    uint32_t *ctrl_ctx = (uint32_t*)(s->input_ctx);
    ctrl_ctx[0] = 0;       /* Drop flags */
    ctrl_ctx[1] = 0x03;    /* Add Slot + EP0 */

    /* Slot Context (at offset ctx_size from start of Input Context)
     *   DWORD 0: [31:27] Context Entries = 1, [23:20] Speed, [19:0] Route String = 0
     *   DWORD 1: [23:16] Root Hub Port Number
     */
    uint32_t *slot_ctx = (uint32_t*)(s->input_ctx + ctx_size);
    slot_ctx[0] = (1u << 27) | ((s->speed & 0xF) << 20);
    slot_ctx[1] = ((s->port & 0xFF) << 16);

    /* EP0 Context (at offset ctx_size * 2)
     *   DWORD 0: [23:16] Max ESIT Payload Hi = 0
     *   DWORD 1: [2:1] CErr = 3, [5:3] EP Type = 4 (Control Bi), [31:16] Max Packet Size
     *   DWORD 2: TR Dequeue Pointer Low | DCS
     *   DWORD 3: TR Dequeue Pointer High = 0
     *   DWORD 4: [15:0] Average TRB Length
     *
     *   Max Packet Size: Full Speed (speed=1) = 8, Low Speed (speed=2) = 8,
     *                    High Speed (speed=3) = 64, Super Speed (speed=4) = 512
     */
    uint32_t max_pkt_ep0 = 8;
    if (s->speed == 3) max_pkt_ep0 = 64;
    else if (s->speed == 4) max_pkt_ep0 = 512;

    uint32_t *ep0_ctx = (uint32_t*)(s->input_ctx + ctx_size * 2);
    ep0_ctx[0] = 0;
    ep0_ctx[1] = (3u << 1) | (4u << 3) | (max_pkt_ep0 << 16);

    s->ep0_ring = (XhciTrb*)pmm_alloc_page();
    for (int i = 0; i < 256; i++) s->ep0_ring[i] = (XhciTrb){0,0,0,0};
    ep0_ctx[2] = (uint32_t)s->ep0_ring | 1; /* DCS = 1 */
    ep0_ctx[3] = 0;
    ep0_ctx[4] = 8; /* Average TRB Length */

    /* Address Device Command: TRB Type = 11, BSR = 0 */
    enqueue_cmd_trb((uint32_t)s->input_ctx, 0, 0, (11u << 10) | (slot_id << 24));
    ser("[xHCI] Address Device slot "); ser_dec(slot_id);
    ser(" port "); ser_dec(s->port);
    ser(" speed "); ser_dec(s->speed);
    ser(" maxpkt "); ser_dec(max_pkt_ep0);
    ser(" input_ctx "); ser_hex32((uint32_t)s->input_ctx);
    ser(" output_ctx "); ser_hex32((uint32_t)s->output_ctx);
    ser("\n");
    klog_info("xHCI: Sent Address Device Command");
}

static void enqueue_transfer_trb(uint32_t slot_id, uint32_t low, uint32_t high, uint32_t status, uint32_t control) {
    XhciSlot *s = &slots[slot_id];
    control = (control & ~1u) | (s->tr_cycle & 1);
    s->transfer_ring[s->tr_enqueue_idx].param_low = low;
    s->transfer_ring[s->tr_enqueue_idx].param_high = high;
    s->transfer_ring[s->tr_enqueue_idx].status = status;
    s->transfer_ring[s->tr_enqueue_idx].control = control;

    s->tr_enqueue_idx++;
    if (s->tr_enqueue_idx == 255) {
        s->transfer_ring[255].param_low = (uint32_t)s->transfer_ring;
        s->transfer_ring[255].param_high = 0;
        s->transfer_ring[255].status = 0;
        s->transfer_ring[255].control = (6u << 10) | (1u << 1) | (s->tr_cycle & 1);
        s->tr_cycle ^= 1;
        s->tr_enqueue_idx = 0;
    }
    ring_doorbell(slot_id, 3); // EP1 IN DCI 3
}

/* ---------- Configure Endpoint ---------- */
static void configure_addressed_slot(uint32_t slot_id) {
    if (slot_id == 0 || slot_id >= 32) return;
    XhciSlot *s = &slots[slot_id];

    /* Allocate Transfer Ring + data buffer for EP1 IN */
    s->transfer_ring = (XhciTrb*)pmm_alloc_page();
    s->transfer_buffer = (uint8_t*)pmm_alloc_page();
    s->tr_cycle = 1;
    s->tr_enqueue_idx = 0;
    for (int i = 0; i < 256; i++) s->transfer_ring[i] = (XhciTrb){0,0,0,0};
    for (int i = 0; i < 4096; i++) s->transfer_buffer[i] = 0;

    /* Rebuild Input Context for Configure Endpoint */
    for (int i = 0; i < 4096; i++) s->input_ctx[i] = 0;

    /* Control Context:
     *   Drop = 0
     *   Add  = A0 (Slot Context) | A3 (DCI 3 = EP1 IN) = (1<<0)|(1<<3) = 0x09
     */
    uint32_t *ctrl_ctx = (uint32_t*)(s->input_ctx);
    ctrl_ctx[0] = 0;       /* Drop flags */
    ctrl_ctx[1] = 0x09;    /* Add Slot + EP1 IN (DCI 3) */

    /* Slot Context: update Context Entries to 3 (covers DCI 1,2,3) */
    uint32_t *slot_ctx = (uint32_t*)(s->input_ctx + ctx_size);
    slot_ctx[0] = (3u << 27) | ((s->speed & 0xF) << 20);
    slot_ctx[1] = ((s->port & 0xFF) << 16);

    /* EP1 IN Context (DCI 3 → index 3 in Device Context, index 4 in Input Context
     * because Input Context prepends the Control Context).
     * Offset = ctx_size * (DCI + 1) = ctx_size * 4
     *
     *   DWORD 0: [23:16] Interval = polling interval
     *            For Full-Speed Interrupt: Interval = bInterval (ms) encoded as 2^(Interval-1)
     *            QEMU USB HID uses bInterval=10 → for FS, use raw value 10
     *            But xHCI encodes FS interrupt interval differently: Interval field = bInterval + 3
     *            Actually for FS/LS: Interval = value from 0-255, representing (Interval+1) * 125 µs frames
     *            Simplest safe value: 6 (≈ 8ms polling, 2^(6-1) = 32 microframes = 4ms for HS,
     *            or just the raw bInterval for FS which the HC clamps).
     *   DWORD 1: [2:1] CErr = 3, [5:3] EP Type = 7 (Interrupt IN), [31:16] Max Packet = 8
     *   DWORD 2: TR Dequeue Pointer Low | DCS
     *   DWORD 3: TR Dequeue Pointer High = 0
     *   DWORD 4: [15:0] Average TRB Length = 8, [31:16] Max ESIT Payload = 8
     */
    uint32_t *ep1_ctx = (uint32_t*)(s->input_ctx + ctx_size * 4);
    ep1_ctx[0] = (6u << 16);  /* Interval = 6 */
    ep1_ctx[1] = (3u << 1) | (7u << 3) | (8u << 16);  /* CErr=3, EPType=7(IntIN), MaxPkt=8 */
    ep1_ctx[2] = ((uint32_t)s->transfer_ring) | 1;      /* TR Dequeue Ptr | DCS=1 */
    ep1_ctx[3] = 0;
    ep1_ctx[4] = 8 | (8u << 16);  /* Avg TRB Len = 8, Max ESIT Payload = 8 */

    /* Configure Endpoint Command: TRB Type = 12 */
    enqueue_cmd_trb((uint32_t)s->input_ctx, 0, 0, (12u << 10) | (slot_id << 24));
    ser("[xHCI] Configure Endpoint slot "); ser_dec(slot_id);
    ser(" add_flags "); ser_hex32(ctrl_ctx[1]);
    ser(" ep1_dw0 "); ser_hex32(ep1_ctx[0]);
    ser(" ep1_dw1 "); ser_hex32(ep1_ctx[1]);
    ser(" ep1_dw2 "); ser_hex32(ep1_ctx[2]);
    ser(" ep1_dw4 "); ser_hex32(ep1_ctx[4]);
    ser("\n");
    klog_info("xHCI: Sent Configure Endpoint Command");
}

/* ---------- Start Polling ---------- */
static void start_polling_slot(uint32_t slot_id) {
    XhciSlot *s = &slots[slot_id];

    /* Enqueue a Normal TRB on the transfer ring for EP1 IN */
    enqueue_transfer_trb(slot_id, (uint32_t)s->transfer_buffer, 0, 8, (1u << 10) | (1u << 5) | (1u << 2));

    ser("[xHCI] Polling started for slot "); ser_dec(slot_id); ser("\n");
    klog_info("xHCI: Ringing EP doorbell to start polling");
}

/* ---------- Event Ring Processing ---------- */
static void process_event_ring(void) {
    int processed = 0;
    while ((event_ring[event_dequeue_idx].control & 1) == event_cycle) {
        XhciTrb *ev = &event_ring[event_dequeue_idx];
        uint32_t trb_type = (ev->control >> 10) & 0x3F;
        uint32_t completion_code = (ev->status >> 24) & 0xFF;

        if (trb_type == 33) {
            /* Command Completion Event */
            uint32_t slot_id = (ev->control >> 24) & 0xFF;

            ser("[xHCI] CmdCompletion CC="); ser_dec(completion_code);
            ser(" slot="); ser_dec(slot_id);
            ser("\n");

            if (completion_code == 1) {
                /* Success */
                /* Check if this is an Enable Slot completion (slot just assigned) */
                uint32_t pending_port = 0, pending_speed = 0;
                if (slot_id > 0 && slot_id < 32 && slots[slot_id].state == 0) {
                    /* This slot was just assigned by Enable Slot command */
                    if (pop_pending(&pending_port, &pending_speed)) {
                        slots[slot_id].state = 2;  /* Skip state 1, go to "Address Device pending" */
                        slots[slot_id].port = pending_port;
                        slots[slot_id].speed = pending_speed;
                        address_device_slot(slot_id);
                    }
                } else if (slot_id > 0 && slot_id < 32 && slots[slot_id].state == 2) {
                    /* Address Device completed → send Configure Endpoint */
                    slots[slot_id].state = 3;
                    configure_addressed_slot(slot_id);
                } else if (slot_id > 0 && slot_id < 32 && slots[slot_id].state == 3) {
                    /* Configure Endpoint completed → start polling */
                    slots[slot_id].state = 4;
                    start_polling_slot(slot_id);
                }
            } else {
                /* Command failed */
                ser("[xHCI] COMMAND FAILED CC="); ser_dec(completion_code);
                ser(" slot="); ser_dec(slot_id);
                ser(" param_lo="); ser_hex32(ev->param_low);
                ser(" status="); ser_hex32(ev->status);
                ser(" control="); ser_hex32(ev->control);
                ser("\n");
                klog_error("xHCI: Command Failed");
                /* Reset state so we don't get stuck */
                if (slot_id > 0 && slot_id < 32) {
                    slots[slot_id].state = 0;
                }
            }
        } else if (trb_type == 32) {
            /* Transfer Event */
            uint32_t slot_id = (ev->control >> 24) & 0xFF;
            if (completion_code == 1 || completion_code == 13) {
                /* Short Packet (13) is OK for HID - means fewer bytes transferred */
                if (slot_id > 0 && slot_id < 32 && slots[slot_id].state == 4) {
                    uint32_t residual = ev->status & 0xFFFFFF;
                    uint32_t actual_len = (residual <= 8) ? (8 - residual) : 8;
                    usb_hid_process_report(slot_id, slots[slot_id].transfer_buffer, actual_len);

                    /* Re-enqueue the Normal TRB for next poll */
                    enqueue_transfer_trb(slot_id, (uint32_t)slots[slot_id].transfer_buffer, 0, 8, (1u << 10) | (1u << 5) | (1u << 2));
                }
            } else {
                ser("[xHCI] Transfer Event FAILED CC="); ser_dec(completion_code);
                ser(" slot="); ser_dec(slot_id); ser("\n");
            }
        } else if (trb_type == 34) {
            /* Port Status Change Event */
            uint32_t port_id = (ev->param_low >> 24) & 0xFF;
            ser("[xHCI] Port Status Change port="); ser_dec(port_id); ser("\n");
        }

        /* Advance dequeue pointer */
        event_dequeue_idx++;
        if (event_dequeue_idx == 256) {
            event_dequeue_idx = 0;
            event_cycle ^= 1;
        }

        /* Write ERDP to acknowledge */
        uint32_t ir_base = rt_base + 0x20;
        mmio_write32(ir_base + 0x18, (uint32_t)&event_ring[event_dequeue_idx] | (1u << 3));  /* EHB */

        processed++;
        if (processed > 64) break; /* Limit per-poll to avoid lockup */
    }
}

/* ---------- Init ---------- */
void usb_xhci_init_from_pci(void) {
    g_xhci.bus = 0; g_xhci.slot = 0; g_xhci.function = 0;
    g_xhci.mmio_base = 0; g_xhci.ports_total = 0; g_xhci.ports_connected = 0;
    g_xhci.enumerate_cycles = 0; g_xhci.hotplug_events = 0;
    g_xhci.ready = 0;

    int i;
    for (i = 0; i < pci_found_count; i++) {
        PciDevice *d = &pci_found[i];
        if (d->class_code == 0x0C && d->subclass == 0x03) {
            g_xhci.bus = d->bus; g_xhci.slot = d->slot; g_xhci.function = d->function;
            uint32_t bar0 = pci_read_bar(i, 0);
            if (bar0 == 0 || bar0 == 0xFFFFFFFF) bar0 = pci_read_bar(i, 1);
            g_xhci.mmio_base = bar0 & ~0xF;

            if (g_xhci.mmio_base != 0 && g_xhci.mmio_base != 0xFFFFFFF0) {
                extern uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
                extern void pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
                uint32_t cmd = pci_read_config(d->bus, d->slot, d->function, 0x04);
                cmd |= (1 << 1) | (1 << 2); /* Memory Space + Bus Master */
                pci_write_config(d->bus, d->slot, d->function, 0x04, cmd);
            }
            break;
        }
    }

    if (g_xhci.mmio_base == 0 || g_xhci.mmio_base == 0xFFFFFFF0) return;

    map_mmio_region(g_xhci.mmio_base & 0xFFFFF000U, 0x4000);

    uint32_t cap_base = g_xhci.mmio_base;
    uint8_t caplength = mmio_read32(cap_base) & 0xFF;
    op_base = cap_base + caplength;

    uint32_t hcsparams1 = mmio_read32(cap_base + XHCI_HCSPARAMS1);
    g_xhci.ports_total = (hcsparams1 >> 24) & 0xFF;
    if (g_xhci.ports_total == 0 || g_xhci.ports_total == 0xFF) return;
    uint32_t max_slots = hcsparams1 & 0xFF;

    uint32_t hccparams1 = mmio_read32(cap_base + XHCI_HCCPARAMS1);
    ctx_size = (hccparams1 & (1 << 2)) ? 64 : 32;

    db_base = cap_base + mmio_read32(cap_base + XHCI_DBOFF);
    rt_base = cap_base + mmio_read32(cap_base + XHCI_RTSOFF);

    ser("[xHCI] MMIO "); ser_hex32(g_xhci.mmio_base);
    ser(" ports "); ser_dec(g_xhci.ports_total);
    ser(" maxslots "); ser_dec(max_slots);
    ser(" ctxsize "); ser_dec(ctx_size);
    ser(" op_base "); ser_hex32(op_base);
    ser(" rt_base "); ser_hex32(rt_base);
    ser(" db_base "); ser_hex32(db_base);
    ser("\n");

    /* Reset controller */
    mmio_write32(op_base + XHCI_USBCMD, USBCMD_HCRST);
    while (mmio_read32(op_base + XHCI_USBCMD) & USBCMD_HCRST) {}
    while (mmio_read32(op_base + XHCI_USBSTS) & (1 << 11)) {} /* CNR */

    /* Configure max device slots */
    mmio_write32(op_base + XHCI_CONFIG, max_slots);

    /* Allocate DCBAA */
    dcbaa = (uint64_t *)pmm_alloc_page();
    for (int j = 0; j < 256; j++) dcbaa[j] = 0;

    /* Allocate Command Ring */
    cmd_ring = (XhciTrb *)pmm_alloc_page();
    for (int j = 0; j < 256; j++) cmd_ring[j] = (XhciTrb){0,0,0,0};

    /* Allocate Event Ring + ERST */
    event_ring = (XhciTrb *)pmm_alloc_page();
    for (int j = 0; j < 256; j++) event_ring[j] = (XhciTrb){0,0,0,0};
    erst = (XhciErstEntry *)pmm_alloc_page();
    erst[0].base_addr_low = (uint32_t)event_ring;
    erst[0].base_addr_high = 0;
    erst[0].size = 256;
    erst[0].reserved = 0;

    /* Program DCBAAP */
    mmio_write32(op_base + XHCI_DCBAAP, (uint32_t)dcbaa);
    mmio_write32(op_base + XHCI_DCBAAP + 4, 0);

    /* Program CRCR */
    mmio_write32(op_base + XHCI_CRCR, (uint32_t)cmd_ring | 1);
    mmio_write32(op_base + XHCI_CRCR + 4, 0);

    /* Program Interrupter 0 */
    uint32_t ir_base = rt_base + 0x20;
    mmio_write32(ir_base + 0x08, 1);                      /* ERSTSZ = 1 segment */
    mmio_write32(ir_base + 0x18, (uint32_t)event_ring);   /* ERDP low */
    mmio_write32(ir_base + 0x1C, 0);                      /* ERDP high */
    mmio_write32(ir_base + 0x10, (uint32_t)erst);          /* ERSTBA low */
    mmio_write32(ir_base + 0x14, 0);                       /* ERSTBA high */

    for (int j = 0; j < 32; j++) {
        slots[j].state = 0;
        slots[j].port = 0;
        slots[j].speed = 0;
        slots[j].input_ctx = 0;
        slots[j].output_ctx = 0;
        slots[j].transfer_ring = 0;
        slots[j].transfer_buffer = 0;
        slots[j].tr_cycle = 1;
        slots[j].ep0_ring = 0;
    }
    pending_head = 0;
    pending_tail = 0;

    /* Start the controller */
    mmio_write32(op_base + XHCI_USBCMD, USBCMD_RS);
    g_xhci.ready = 1;
    klog_info("xHCI Host Controller initialized and running.");
}

/* ---------- Poll ---------- */
void usb_xhci_poll(void) {
    if (!g_xhci.ready) return;

    process_event_ring();

    /* Scan ports for connection status changes */
    for (uint32_t i = 1; i <= g_xhci.ports_total; i++) {
        uint32_t portsc_addr = op_base + 0x400 + (0x10 * (i - 1));
        uint32_t portsc = mmio_read32(portsc_addr);

        /* Check CSC (Connect Status Change, bit 17) and CCS (Current Connect Status, bit 0) */
        if ((portsc & (1 << 17)) && (portsc & 1)) {
            g_xhci.hotplug_events++;
            g_xhci.ports_connected++;

            /* Acknowledge CSC by writing 1 to bit 17 (W1C), preserve RW bits */
            /* PORTSC W1C bits: 17(CSC), 18(PEC), 19(WRC), 20(OCC), 21(PRC), 22(PLC), 23(CEC) */
            /* Must not accidentally clear other W1C bits: mask them out, set only CSC */
            uint32_t portsc_ack = (portsc & 0x0E00C3E0u) | (1u << 17);
            mmio_write32(portsc_addr, portsc_ack);

            /* Issue Port Reset (bit 4) */
            portsc = mmio_read32(portsc_addr);
            mmio_write32(portsc_addr, (portsc & 0x0E00C3E0u) | (1u << 4));

            /* Wait for Port Reset Change (bit 21) to indicate reset complete */
            for (int w = 0; w < 100000; w++) {
                portsc = mmio_read32(portsc_addr);
                if (portsc & (1u << 21)) break; /* PRC set = reset done */
            }
            /* Clear PRC */
            mmio_write32(portsc_addr, (portsc & 0x0E00C3E0u) | (1u << 21));

            /* Read port speed from PORTSC bits [13:10] */
            portsc = mmio_read32(portsc_addr);
            uint32_t speed = (portsc >> 10) & 0xF;
            ser("[xHCI] Port "); ser_dec(i);
            ser(" connected, PORTSC="); ser_hex32(portsc);
            ser(" speed="); ser_dec(speed);
            ser("\n");

            init_slot_for_port(i, speed);
        }
    }
}

int usb_xhci_is_ready(void) { return g_xhci.ready ? 1 : 0; }
int usb_xhci_reset_port(uint32_t port_index) { (void)port_index; return 1; }
int usb_xhci_reset_all_ports(void) { return 1; }
void usb_xhci_note_hotplug_event(uint32_t count) { g_xhci.hotplug_events += count; }
void usb_xhci_note_stall_recovery(void) { g_xhci.stall_recoveries++; }
void usb_xhci_note_retry(void) { g_xhci.retry_events++; }
int usb_xhci_suspend(void) { g_xhci.suspend_count++; return 1; }
int usb_xhci_resume(void) { g_xhci.resume_count++; return 1; }
void usb_xhci_set_quarantined_ports(uint32_t q) { g_xhci.quarantined_ports = q; }
void usb_xhci_note_enumeration(void) { g_xhci.enumerate_cycles++; }
void usb_xhci_get_report(UsbXhciReport *out_report) {
    if (out_report) *out_report = g_xhci;
}
