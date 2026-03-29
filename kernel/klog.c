#include "klog.h"

typedef unsigned int uint32_t;

extern void gprint(char *str, uint32_t color);

static void klog_emit(const char *prefix, const char *msg, uint32_t color) {
  gprint((char *)prefix, color);
  gprint((char *)msg, 0xDDDDDD);
  gprint("\n", 0x000000);
}

void klog_info(const char *msg) { klog_emit("[INFO] ", msg, 0x33FFAA); }
void klog_warn(const char *msg) { klog_emit("[WARN] ", msg, 0xFFCC33); }
void klog_error(const char *msg) { klog_emit("[ERR ] ", msg, 0xFF5555); }
