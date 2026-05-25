#include "klog.h"

void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static void serial_print(const char *str) {
    while (*str) {
        outb(0x3F8, *str++);
    }
}

typedef unsigned int uint32_t;

extern void gprint(char *str, uint32_t color);

static void klog_emit(const char *prefix, const char *msg, uint32_t color) {
  gprint((char *)prefix, color);
  gprint((char *)msg, 0xDDDDDD);
  gprint("\n", 0x000000);
  serial_print(prefix);
  serial_print(msg);
  serial_print("\n");
}

void klog_info(const char *msg) { klog_emit("[INFO] ", msg, 0x33FFAA); }
void klog_warn(const char *msg) { klog_emit("[WARN] ", msg, 0xFFCC33); }
void klog_error(const char *msg) { klog_emit("[ERR ] ", msg, 0xFF5555); }
