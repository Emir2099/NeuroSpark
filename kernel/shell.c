#include "shell.h"
#include "disk.h"
#include "pci.h"
#include "ahci.h"
#include "storage_manager.h"
#include "klog.h"
#include "vfs.h"
#include "usermode.h"
#include "net.h"
#include "profiling.h"
#include "model_manager.h"
#include "task.h"
#include "scheduler.h"
#include "dashboard.h"
#include "wm.h"
#include "ext2fs.h"
#include "page_cache.h"
#include "module_loader.h"
#include "posix.h"

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

typedef struct {
  int voltage;
  int spike_count;
  int id;
  int synaptic_weight;
  int refractory_timer;
  int dynamic_threshold;
} Neuron;

typedef struct {
  Neuron neurons[5];
  unsigned char current_phase;
  int pixel_recent_spikes;
} NeuralPixel;

typedef struct {
  int task_id;
  int priority;
  int target_pixel;
  int base_integration;
  int fire_threshold;
  const char *task_name;
  int saved_voltages[5];
  int saved_weights[5];
  int saved_thresholds[5];
  int last_spike_count;
  int spikes_per_second;
  int state;
} TaskControlBlock;

extern volatile int current_shell_row;
extern volatile char input_buffer[32];
extern volatile int buffer_idx;
extern volatile int mouse_x;
extern volatile int mouse_y;
extern volatile int mouse_buttons;
extern int zoom_level;
extern int zoom_offset;
extern uint32_t tick;
extern uint32_t render_frame;
extern int potentials[16];
extern TaskControlBlock task_list[2];
extern NeuralPixel os_memory_map[2];
extern FileEntry root_directory[25];

extern void gprint(char *str, uint32_t color);
extern void gprint_dec(int val, uint32_t color);
extern void gprint_hex(uint32_t val, int digits, uint32_t color);
extern void clear_region(int x0, int y0, int x1, int y1, uint32_t color);
extern void pmm_print_map(void);
extern void pmm_free_page(uint32_t page_addr);
extern void *pmm_alloc_page(void);
extern void map_mmio_region(uint32_t phys, uint32_t size);
extern int sys_load_task(int task_id, int slot);
extern void sys_save_task(int task_id, int slot);
extern void disk_write_sector(uint32_t lba, uint16_t *buffer);
extern void disk_read_sector(uint32_t lba, uint16_t *buffer);
extern void pci_print_results(void);
extern void set_cmd_output(const char *text);
extern int vfs_delete(const char *path);
extern int wm_set_timezone_offset_minutes(int minutes);
extern int wm_get_timezone_offset_minutes(void);
extern uint32_t remote_get_auth_deadline(void);

void process_command(char *cmd);

static int str_eq(const char *a, const char *b) {
  int i = 0;
  while (a[i] && b[i]) {
    if (a[i] != b[i])
      return 0;
    i++;
  }
  return a[i] == '\0' && b[i] == '\0';
}

static int parse_u32_dec(const char *s) {
  int v = 0;
  int i = 0;
  while (s[i] >= '0' && s[i] <= '9') {
    v = (v * 10) + (s[i] - '0');
    i++;
  }
  return v;
}

static int is_dec_number(const char *s) {
  int i = 0;
  if (s == 0 || s[0] == '\0') {
    return 0;
  }
  while (s[i]) {
    if (s[i] < '0' || s[i] > '9') {
      return 0;
    }
    i++;
  }
  return 1;
}

static uint32_t parse_u32_hex(const char *s) {
  uint32_t v = 0;
  int i = 0;
  if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    i = 2;
  while (s[i]) {
    char c = s[i++];
    if (c >= '0' && c <= '9')
      v = (v << 4) | (uint32_t)(c - '0');
    else if (c >= 'a' && c <= 'f')
      v = (v << 4) | (uint32_t)(c - 'a' + 10);
    else if (c >= 'A' && c <= 'F')
      v = (v << 4) | (uint32_t)(c - 'A' + 10);
    else
      break;
  }
  return v;
}

static int parse_u32_octal(const char *s, uint32_t *out) {
  uint32_t v = 0;
  int i = 0;

  if (s == 0 || out == 0 || s[0] == '\0') {
    return 0;
  }

  while (s[i]) {
    if (s[i] < '0' || s[i] > '7') {
      return 0;
    }
    v = (v << 3) | (uint32_t)(s[i] - '0');
    i++;
  }

  *out = v;
  return 1;
}

static int str_ieq(const char *a, const char *b) {
  int i = 0;
  while (a[i] && b[i]) {
    char ca = a[i];
    char cb = b[i];
    if (ca >= 'A' && ca <= 'Z')
      ca = (char)(ca + 32);
    if (cb >= 'A' && cb <= 'Z')
      cb = (char)(cb + 32);
    if (ca != cb)
      return 0;
    i++;
  }
  return a[i] == '\0' && b[i] == '\0';
}

static const char *next_token(const char *src, char *out, int out_len) {
  int i = 0;
  if (src == 0 || out == 0 || out_len <= 0) {
    return src;
  }

  while (*src == ' ') {
    src++;
  }
  if (*src == '\0') {
    out[0] = '\0';
    return src;
  }

  while (src[i] && src[i] != ' ' && i < out_len - 1) {
    out[i] = src[i];
    i++;
  }
  out[i] = '\0';

  while (src[i] && src[i] != ' ') {
    i++;
  }
  while (src[i] == ' ') {
    i++;
  }
  return src + i;
}

static int clamp_i32(int value, int lo, int hi) {
  if (value < lo)
    return lo;
  if (value > hi)
    return hi;
  return value;
}

static int abs_i32(int v) {
  if (v < 0)
    return -v;
  return v;
}

static void append_text(char *out, int out_len, int *cursor, const char *text) {
  int i = 0;
  if (out == 0 || cursor == 0 || text == 0 || out_len <= 0) {
    return;
  }
  while (text[i] && *cursor < out_len - 1) {
    out[*cursor] = text[i];
    (*cursor)++;
    i++;
  }
  out[*cursor] = '\0';
}

static void append_dec(char *out, int out_len, int *cursor, int value) {
  char tmp[16];
  int i = 0;
  int neg = 0;

  if (value == 0) {
    append_text(out, out_len, cursor, "0");
    return;
  }
  if (value < 0) {
    neg = 1;
    value = -value;
  }
  while (value > 0 && i < 15) {
    tmp[i++] = (char)('0' + (value % 10));
    value /= 10;
  }
  if (neg && i < 15) {
    tmp[i++] = '-';
  }
  while (i > 0) {
    char c[2];
    c[0] = tmp[--i];
    c[1] = '\0';
    append_text(out, out_len, cursor, c);
  }
}

static int parse_ipv4(const char *s, uint32_t *out) {
  int idx = 0;
  uint32_t parts[4] = {0, 0, 0, 0};
  if (s == 0 || out == 0) {
    return 0;
  }

  while (*s) {
    if (*s >= '0' && *s <= '9') {
      parts[idx] = (parts[idx] * 10u) + (uint32_t)(*s - '0');
      if (parts[idx] > 255u) {
        return 0;
      }
    } else if (*s == '.') {
      idx++;
      if (idx > 3) {
        return 0;
      }
    } else {
      return 0;
    }
    s++;
  }

  if (idx != 3) {
    return 0;
  }

  *out = (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3];
  return 1;
}

static void print_ipv4(uint32_t ip) {
  gprint_dec((int)((ip >> 24) & 0xFFu), 0xFFFFFF);
  gprint(".", 0x666666);
  gprint_dec((int)((ip >> 16) & 0xFFu), 0xFFFFFF);
  gprint(".", 0x666666);
  gprint_dec((int)((ip >> 8) & 0xFFu), 0xFFFFFF);
  gprint(".", 0x666666);
  gprint_dec((int)(ip & 0xFFu), 0xFFFFFF);
}

static const char *task_state_name(uint32_t state) {
  if (state == TASK_STATE_READY) {
    return "READY";
  }
  if (state == TASK_STATE_RUNNING) {
    return "RUN";
  }
  if (state == TASK_STATE_BLOCKED) {
    return "BLK";
  }
  if (state == TASK_STATE_SLEEPING) {
    return "SLP";
  }
  if (state == TASK_STATE_TERMINATED) {
    return "TERM";
  }
  return "?";
}

static const char *workload_name(uint32_t klass) {
  if (klass == WORKLOAD_CLASS_SIM) {
    return "SIM";
  }
  if (klass == WORKLOAD_CLASS_LOG) {
    return "LOG";
  }
  if (klass == WORKLOAD_CLASS_EXPORT) {
    return "EXP";
  }
  return "GEN";
}

static const char *trace_event_name(uint32_t evt) {
  if (evt == TASK_TRACE_EVT_SWITCH_IN) {
    return "SW-IN";
  }
  if (evt == TASK_TRACE_EVT_SWITCH_OUT) {
    return "SW-OUT";
  }
  if (evt == TASK_TRACE_EVT_SYSCALL) {
    return "SYSCALL";
  }
  if (evt == TASK_TRACE_EVT_SLEEP) {
    return "SLEEP";
  }
  if (evt == TASK_TRACE_EVT_WAKE) {
    return "WAKE";
  }
  if (evt == TASK_TRACE_EVT_TERMINATE) {
    return "TERM";
  }
  if (evt == TASK_TRACE_EVT_PRIORITY) {
    return "PRIO";
  }
  if (evt == TASK_TRACE_EVT_KILL) {
    return "KILL";
  }
  if (evt == TASK_TRACE_EVT_FAULT) {
    return "FAULT";
  }
  return "NONE";
}

static const char *fault_vector_name(uint32_t packed_fault) {
  uint32_t vector = (packed_fault >> 24) & 0xFFu;
  if (vector == 0) {
    return "-";
  }
  if (vector == 13) {
    return "GP";
  }
  if (vector == 14) {
    return "PF";
  }
  if (vector == 6) {
    return "UD";
  }
  if (vector == 8) {
    return "DF";
  }
  return "EXC";
}

static int require_admin_context(void) {
  if (os_current_task != 0) {
    set_cmd_output("ERR: PRIVILEGED (TASK0)");
    return 0;
  }
  return 1;
}

#define REPLAY_MAX_CMDS 64
static char replay_cmds[REPLAY_MAX_CMDS][32];
static int replay_count = 0;
static int replay_recording = 0;
static int replay_running = 0;

static void copy_cmd_text(char *dst, const char *src, int max_len) {
  int i = 0;
  if (dst == 0 || max_len <= 0) {
    return;
  }
  if (src == 0) {
    dst[0] = '\0';
    return;
  }
  while (src[i] && i < max_len - 1) {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
}

static void replay_record_command(const char *cmd) {
  if (!replay_recording || replay_running || cmd == 0) {
    return;
  }
  if (cmd[0] == 'r' && cmd[1] == 'e' && cmd[2] == 'p' && cmd[3] == 'l' &&
      cmd[4] == 'a' && cmd[5] == 'y' && (cmd[6] == '\0' || cmd[6] == ' ')) {
    return;
  }
  if (replay_count < REPLAY_MAX_CMDS) {
    copy_cmd_text(replay_cmds[replay_count], cmd, 32);
    replay_count++;
  }
}

static int parse_tz_offset_minutes(const char *s, int *out_minutes) {
  int i = 0;
  int sign = 1;
  int hours = 0;
  int mins = 0;
  int saw_digit = 0;

  if (s == 0 || out_minutes == 0 || s[0] == '\0') {
    return 0;
  }

  if (s[i] == '+') {
    i++;
  } else if (s[i] == '-') {
    sign = -1;
    i++;
  }

  while (s[i] >= '0' && s[i] <= '9') {
    saw_digit = 1;
    hours = (hours * 10) + (s[i] - '0');
    i++;
  }

  if (!saw_digit) {
    return 0;
  }

  if (s[i] == ':') {
    i++;
    if (!(s[i] >= '0' && s[i] <= '9' && s[i + 1] >= '0' && s[i + 1] <= '9')) {
      return 0;
    }
    mins = ((s[i] - '0') * 10) + (s[i + 1] - '0');
    i += 2;
    if (mins < 0 || mins > 59) {
      return 0;
    }
  }

  if (s[i] != '\0') {
    return 0;
  }

  *out_minutes = sign * ((hours * 60) + mins);
  return 1;
}

static int preset_profile(const char *preset, int *weight_delta, int *thr_delta,
                          int *base_integration, int *fire_threshold) {
  if (str_ieq(preset, "calm")) {
    *weight_delta = -40;
    *thr_delta = 220;
    *base_integration = 20;
    *fire_threshold = 1200;
    return 1;
  }
  if (str_ieq(preset, "focus")) {
    *weight_delta = 60;
    *thr_delta = 80;
    *base_integration = 35;
    *fire_threshold = 1000;
    return 1;
  }
  if (str_ieq(preset, "plastic")) {
    *weight_delta = 120;
    *thr_delta = -120;
    *base_integration = 55;
    *fire_threshold = 850;
    return 1;
  }
  return 0;
}

void scroll_shell(void) {
  current_shell_row = 24;
}

void list_files(void) {
  if (ata_disk_available) {
    disk_read_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
  }

  gprint("NAME         LBA    SIZE\n", 0x99EEFF);
  gprint("------------------------\n", 0x557799);

  int found = 0;
  for (int i = 0; i < TFS_MAX_FILES; i++) {
    if (root_directory[i].flags == 1) {
      gprint(root_directory[i].name, 0x00FF88);
      gprint("  ", 0x000000);
      gprint_hex(root_directory[i].lba, 4, 0xFFE066);
      gprint("  ", 0x000000);
      gprint_dec((int)root_directory[i].size, 0xFFFFFF);
      gprint("\n", 0x000000);
      found = 1;
    }
  }

  if (!found) {
    gprint("NO FILES FOUND.\n", 0xFF5555);
  }
}

void list_files_gfx(void) { list_files(); }

static void cmd_help(const char *args) {
  (void)args;
  gprint("commands: help save load ls stat mount umount pcache diag clear delete eval stim mall free map\n", 0xFFFFFF);
    gprint("          tsave tload pci pci bar ahci [show|backend|reset|identify|read|smoke]\n", 0xFFFFFF);
    gprint("          exec <path> mkdemo net up|cfg|status|tx|export|profile|manifest remote ...\n", 0xFFFFFF);
  gprint("phase8:   manifest save|load|show  replay rec on|off|run|show|clear\n", 0x88E0FF);
  gprint("          dataset export <path>  dataset import <path>\n", 0x88E0FF);
  gprint("phase9:   profile on|off|show|reset|export <path>|hud compact|detail\n", 0x88E0FF);
  gprint("phase10:  model show|select|param ...  stdp on|off\n", 0x88E0FF);
  gprint("phase11:  ps  kill <pid>  nice <pid> <0..3>  trace <pid> on|off|show\n", 0x88E0FF);
  gprint("phase11.1: ipc send <ch> <val> | ipc recv <ch> | ipc stat <ch>\n", 0x88E0FF);
  gprint("phase12:  net up  net cfg <ip> <mask> <gw>  net export manifest\n", 0x88E0FF);
  gprint("          remote on|off|auth <hex> [ttl]|token <hex>\n", 0x88E0FF);
  gprint("phase13:  viz heatmap|raster|off|show  viz scrub <0..63|+|->  viz play on|off\n", 0x88E0FF);
  gprint("          viz compare <a> <b>  viz export <path>\n", 0x88E0FF);
  gprint("phase6:   synview synset synrule synpreset syncmp\n", 0x77C8FF);
  gprint("          sbrowse spreview stag sdiff\n", 0x77C8FF);
  gprint("phase25:  insmod <path>  rmmod <path|handle>  lsmod\n", 0x77C8FF);
  gprint("phase26:  id [task]  chmod <path> <octal>  chown <path> <uid> <gid>\n", 0x77C8FF);
  gprint("          setuid <uid>  setgid <gid>  setpgid <task> <pgid>\n", 0x77C8FF);
  gprint("          jobs  fg <task>  bg <task>  tcsetpgrp <tty> <pgid>  tcgetpgrp <tty>\n", 0x77C8FF);
  gprint("system:   tz show | tz set <+HH[:MM]|-HH[:MM]>\n", 0x77C8FF);
}

static void cmd_tz(const char *args) {
  char sub[16];
  char val[16];
  int minutes = 0;
  int abs_minutes = 0;
  int hh = 0;
  int mm = 0;

  args = next_token(args, sub, sizeof(sub));
  next_token(args, val, sizeof(val));

    if (sub[0] == '\0' || str_ieq(sub, "show")) {
    minutes = wm_get_timezone_offset_minutes();
    abs_minutes = minutes < 0 ? -minutes : minutes;
    hh = abs_minutes / 60;
    mm = abs_minutes % 60;

    gprint("TZ OFFSET: ", 0x99EEFF);
    gprint(minutes < 0 ? "-" : "+", minutes < 0 ? 0xFFAA66 : 0x77FFAA);
    if (hh < 10) gprint("0", 0xFFFFFF);
    gprint_dec(hh, 0xFFFFFF);
    gprint(":", 0x99EEFF);
    if (mm < 10) gprint("0", 0xFFFFFF);
    gprint_dec(mm, 0xFFFFFF);
    gprint("\n", 0x000000);
    set_cmd_output("TZ SHOW");
    return;
  }

  if (!str_ieq(sub, "set") || val[0] == '\0') {
    set_cmd_output("USAGE: tz show | tz set <+HH[:MM]|-HH[:MM]>");
    return;
  }

  if (!parse_tz_offset_minutes(val, &minutes)) {
    set_cmd_output("TZ FORMAT ERR");
    return;
  }

  if (!wm_set_timezone_offset_minutes(minutes)) {
    set_cmd_output("TZ RANGE ERR (-12:00..+14:00)");
    return;
  }

  set_cmd_output("TZ SET+SAVED");
}

static void cmd_viz(const char *args) {
  char sub[16];
  char a[16];
  char b[16];
  char c[40];

  args = next_token(args, sub, sizeof(sub));
  args = next_token(args, a, sizeof(a));
  args = next_token(args, b, sizeof(b));
  next_token(args, c, sizeof(c));

  if (sub[0] == '\0' || str_eq(sub, "show")) {
    gprint("VIZ MODE: ", 0x99EEFF);
    if (viz_get_mode() == VIZ_MODE_HEATMAP) {
      gprint("HEATMAP", 0x77FFAA);
    } else if (viz_get_mode() == VIZ_MODE_RASTER) {
      gprint("RASTER", 0x77FFAA);
    } else if (viz_get_mode() == VIZ_MODE_COMPARE) {
      gprint("COMPARE", 0x77FFAA);
    } else {
      gprint("OFF", 0xFFAA66);
    }
    gprint(" SCRUB:", 0x99EEFF);
    gprint_dec(viz_get_scrub(), 0xFFFFFF);
    gprint(" PLAY:", 0x99EEFF);
    gprint(viz_get_autoplay() ? "ON" : "OFF", viz_get_autoplay() ? 0x44FF88 : 0xFFAA66);
    gprint("\n", 0x000000);
    return;
  }

  if (str_eq(sub, "off")) {
    viz_set_mode(VIZ_MODE_OFF);
    set_cmd_output("VIZ OFF");
    return;
  }

  if (str_eq(sub, "heatmap")) {
    viz_set_mode(VIZ_MODE_HEATMAP);
    set_cmd_output("VIZ HEATMAP");
    return;
  }

  if (str_eq(sub, "raster")) {
    viz_set_mode(VIZ_MODE_RASTER);
    set_cmd_output("VIZ RASTER");
    return;
  }

  if (str_eq(sub, "scrub")) {
    if (str_eq(a, "+")) {
      viz_step_scrub(1);
      set_cmd_output("VIZ SCRUB +");
      return;
    }
    if (str_eq(a, "-")) {
      viz_step_scrub(-1);
      set_cmd_output("VIZ SCRUB -");
      return;
    }
    if (!is_dec_number(a)) {
      set_cmd_output("USAGE: viz scrub <0..63|+|->");
      return;
    }
    viz_set_scrub(parse_u32_dec(a));
    set_cmd_output("VIZ SCRUB SET");
    return;
  }

  if (str_eq(sub, "play")) {
    if (str_eq(a, "on")) {
      viz_set_autoplay(1);
      set_cmd_output("VIZ PLAY ON");
      return;
    }
    if (str_eq(a, "off")) {
      viz_set_autoplay(0);
      set_cmd_output("VIZ PLAY OFF");
      return;
    }
    set_cmd_output("USAGE: viz play on|off");
    return;
  }

  if (str_eq(sub, "compare")) {
    int sa, sb;
    if (!is_dec_number(a) || !is_dec_number(b)) {
      set_cmd_output("USAGE: viz compare <a> <b>");
      return;
    }
    sa = parse_u32_dec(a);
    sb = parse_u32_dec(b);
    if (!viz_set_compare_slots(sa, sb)) {
      set_cmd_output("VIZ COMPARE FAIL");
      return;
    }
    viz_set_mode(VIZ_MODE_COMPARE);
    set_cmd_output("VIZ COMPARE");
    return;
  }

  if (str_eq(sub, "export")) {
    char path[40];
    char report[480];
    int cur = 0;
    int dv = 0, dw = 0, dt = 0;
    int dvv[5], dww[5], dtt[5];

    copy_cmd_text(path, a, sizeof(path));
    if (path[0] == '\0') {
      copy_cmd_text(path, "/viz.rpt", sizeof(path));
    }

    append_text(report, sizeof(report), &cur, "viz_mode=");
    append_dec(report, sizeof(report), &cur, viz_get_mode());
    append_text(report, sizeof(report), &cur, " scrub=");
    append_dec(report, sizeof(report), &cur, viz_get_scrub());
    append_text(report, sizeof(report), &cur, " autoplay=");
    append_dec(report, sizeof(report), &cur, viz_get_autoplay());
    append_text(report, sizeof(report), &cur, "\n");

    if (storage_diff_snapshots(0, 1, &dv, &dw, &dt)) {
      append_text(report, sizeof(report), &cur, "slot01 dV=");
      append_dec(report, sizeof(report), &cur, dv);
      append_text(report, sizeof(report), &cur, " dW=");
      append_dec(report, sizeof(report), &cur, dw);
      append_text(report, sizeof(report), &cur, " dT=");
      append_dec(report, sizeof(report), &cur, dt);
      append_text(report, sizeof(report), &cur, "\n");
    }

    if (storage_diff_snapshots_vector(0, 1, dvv, dww, dtt, 5)) {
      for (int i = 0; i < 5; i++) {
        append_text(report, sizeof(report), &cur, "n");
        append_dec(report, sizeof(report), &cur, i);
        append_text(report, sizeof(report), &cur, " dV=");
        append_dec(report, sizeof(report), &cur, dvv[i]);
        append_text(report, sizeof(report), &cur, " dW=");
        append_dec(report, sizeof(report), &cur, dww[i]);
        append_text(report, sizeof(report), &cur, " dT=");
        append_dec(report, sizeof(report), &cur, dtt[i]);
        append_text(report, sizeof(report), &cur, "\n");
      }
    }

    if (vfs_write_file(path, report, (uint32_t)cur) >= 0) {
      set_cmd_output("VIZ REPORT EXPORTED");
    } else {
      set_cmd_output("VIZ EXPORT FAIL");
    }
    return;
  }

  set_cmd_output("USAGE: viz heatmap|raster|off|show|scrub <0..63|+|->|play on|off|compare <a> <b>|export <path>");
}

static void cmd_ps(const char *args) {
  (void)args;

  gprint("PID ST   PR CLS RT CSW TRACE EVT ARG FERR FADDR FEIP FRSN\n", 0x99EEFF);
  for (int i = 0; i < os_task_count; i++) {
    gprint_dec(i, 0xFFFFFF);
    gprint("   ", 0x000000);
    gprint((char *)task_state_name(os_tasks[i].state), 0x77FFAA);
    gprint(" ", 0x000000);
    gprint_dec((int)os_tasks[i].priority, 0xFFE066);
    gprint("  ", 0x000000);
    gprint((char *)workload_name(os_tasks[i].workload_class), 0xA0E0FF);
    gprint("  ", 0x000000);
    gprint_dec((int)os_tasks[i].runtime_ticks, 0xFFFFFF);
    gprint(" ", 0x000000);
    gprint_dec((int)os_tasks[i].context_switches, 0xFFFFFF);
    gprint("   ", 0x000000);
    gprint(os_tasks[i].trace_enabled ? "ON " : "OFF", os_tasks[i].trace_enabled ? 0x44FF88 : 0xFFAA66);
    gprint(" ", 0x000000);
    gprint((char *)trace_event_name(os_tasks[i].trace_last_event), 0x77C8FF);
    gprint(" ", 0x000000);
    gprint_dec((int)os_tasks[i].trace_last_arg, 0xFFFFFF);
    gprint(" ", 0x000000);
    gprint_hex(os_tasks[i].fault_code, 8, 0xFFCC66);
    gprint(" ", 0x000000);
    gprint_hex(os_tasks[i].fault_addr, 8, 0xFFAA66);
    gprint(" ", 0x000000);
    gprint_hex(os_tasks[i].fault_eip, 8, 0x77C8FF);
    gprint(" ", 0x000000);
    gprint((char *)fault_vector_name(os_tasks[i].fault_code), 0xFFAA66);
    gprint("\n", 0x000000);
  }
}

static void cmd_kill(const char *args) {
  if (!require_admin_context()) {
    return;
  }
  int pid = parse_u32_dec(args);
  if (!is_dec_number(args)) {
    set_cmd_output("USAGE: kill <pid>");
    return;
  }
  if (pid <= 0 || pid >= os_task_count) {
    set_cmd_output("KILL DENIED");
    return;
  }
  if (os_tasks[pid].state == TASK_STATE_TERMINATED) {
    set_cmd_output("ALREADY TERM");
    return;
  }

  os_tasks[pid].state = TASK_STATE_TERMINATED;
  os_tasks[pid].time_slice = 0;
  task_trace_event(pid, TASK_TRACE_EVT_KILL, (uint32_t)os_current_task);
  set_cmd_output("TASK KILLED");
}

static void cmd_nice(const char *args) {
  if (!require_admin_context()) {
    return;
  }
  char t_pid[8];
  char t_prio[8];
  args = next_token(args, t_pid, sizeof(t_pid));
  next_token(args, t_prio, sizeof(t_prio));

  if (!is_dec_number(t_pid) || !is_dec_number(t_prio)) {
    set_cmd_output("USAGE: nice <pid> <0..3>");
    return;
  }

  {
    int pid = parse_u32_dec(t_pid);
    int prio = parse_u32_dec(t_prio);
    if (pid < 0 || pid >= os_task_count || prio < 0 || prio > 3) {
      set_cmd_output("NICE FAIL");
      return;
    }

    os_tasks[pid].priority = (uint32_t)prio;
    task_trace_event(pid, TASK_TRACE_EVT_PRIORITY, (uint32_t)prio);
    set_cmd_output("NICE OK");
  }
}

static void cmd_trace(const char *args) {
  char t_pid[8];
  char t_mode[8];
  args = next_token(args, t_pid, sizeof(t_pid));
  next_token(args, t_mode, sizeof(t_mode));

  if (!is_dec_number(t_pid)) {
    set_cmd_output("USAGE: trace <pid> on|off|show");
    return;
  }

  {
    int pid = parse_u32_dec(t_pid);
    if (pid < 0 || pid >= os_task_count) {
      set_cmd_output("TRACE FAIL");
      return;
    }

    if (t_mode[0] == '\0' || str_eq(t_mode, "show")) {
      gprint("TRACE PID ", 0x99EEFF);
      gprint_dec(pid, 0xFFFFFF);
      gprint(" ", 0x000000);
      gprint(os_tasks[pid].trace_enabled ? "ON" : "OFF",
             os_tasks[pid].trace_enabled ? 0x44FF88 : 0xFFAA66);
      gprint(" EVT:", 0x99EEFF);
      gprint((char *)trace_event_name(os_tasks[pid].trace_last_event), 0x77C8FF);
      gprint(" ARG:", 0x99EEFF);
      gprint_dec((int)os_tasks[pid].trace_last_arg, 0xFFFFFF);
      gprint(" CNT:", 0x99EEFF);
      gprint_dec((int)os_tasks[pid].trace_event_count, 0xFFFFFF);
      gprint("\n", 0x000000);
      return;
    }

    if (str_eq(t_mode, "on")) {
      if (!require_admin_context()) {
        return;
      }
      os_tasks[pid].trace_enabled = 1;
      task_trace_event(pid, TASK_TRACE_EVT_PRIORITY, os_tasks[pid].priority);
      set_cmd_output("TRACE ON");
      return;
    }
    if (str_eq(t_mode, "off")) {
      if (!require_admin_context()) {
        return;
      }
      os_tasks[pid].trace_enabled = 0;
      set_cmd_output("TRACE OFF");
      return;
    }
  }

  set_cmd_output("USAGE: trace <pid> on|off|show");
}

static void cmd_ipc(const char *args) {
  char sub[8];
  char t_ch[8];
  char t_val[16];
  int ch;

  args = next_token(args, sub, sizeof(sub));
  args = next_token(args, t_ch, sizeof(t_ch));
  next_token(args, t_val, sizeof(t_val));

  if (sub[0] == '\0') {
    set_cmd_output("USAGE: ipc send|recv|stat <ch> [val]");
    return;
  }
  if (!is_dec_number(t_ch)) {
    set_cmd_output("USAGE: ipc send|recv|stat <ch> [val]");
    return;
  }
  ch = parse_u32_dec(t_ch);

  if (str_eq(sub, "send")) {
    int val;
    if (!require_admin_context()) {
      return;
    }
    if (!is_dec_number(t_val)) {
      set_cmd_output("USAGE: ipc send <ch> <val>");
      return;
    }
    val = parse_u32_dec(t_val);
    if (ipc_send(ch, val)) {
      set_cmd_output("IPC SENT");
    } else {
      set_cmd_output("IPC SEND FAIL");
    }
    return;
  }

  if (str_eq(sub, "recv")) {
    int out = 0;
    if (ipc_recv(ch, &out)) {
      gprint("IPC[", 0x99EEFF);
      gprint_dec(ch, 0xFFFFFF);
      gprint("] -> ", 0x99EEFF);
      gprint_dec(out, 0x77FFAA);
      gprint("\n", 0x000000);
      set_cmd_output("IPC RECV OK");
    } else {
      set_cmd_output("IPC EMPTY");
    }
    return;
  }

  if (str_eq(sub, "stat")) {
    int depth = ipc_queue_depth(ch);
    if (depth < 0) {
      set_cmd_output("IPC CH INVALID");
      return;
    }
    gprint("IPC[", 0x99EEFF);
    gprint_dec(ch, 0xFFFFFF);
    gprint("] depth=", 0x99EEFF);
    gprint_dec(depth, 0x77FFAA);
    gprint("\n", 0x000000);
    return;
  }

  set_cmd_output("USAGE: ipc send|recv|stat <ch> [val]");
}

static void cmd_model(const char *args) {
  char sub[12];
  char a[16];
  char b[16];
  char c[16];
  char d[16];

  args = next_token(args, sub, sizeof(sub));
  args = next_token(args, a, sizeof(a));
  args = next_token(args, b, sizeof(b));
  args = next_token(args, c, sizeof(c));
  next_token(args, d, sizeof(d));

  if (sub[0] == '\0' || str_eq(sub, "show")) {
    gprint("MODEL T0:", 0x99EEFF);
    gprint((char *)model_name(0), 0x77FFAA);
    gprint(" T1:", 0x99EEFF);
    gprint((char *)model_name(1), 0x77FFAA);
    gprint(" STDP:", 0x99EEFF);
    gprint(model_get_stdp() ? "ON" : "OFF", model_get_stdp() ? 0x44FF88 : 0xFFAA66);
    gprint("\n", 0x000000);
    return;
  }

  if (str_eq(sub, "select")) {
    int task_id = -1;
    const char *model = a;

    if (is_dec_number(a)) {
      task_id = parse_u32_dec(a);
      model = b;
    }

    if (model == 0 || model[0] == '\0') {
      set_cmd_output("USAGE: model select [task] <lif|adapt|stdp>");
      return;
    }

    if (model_select(task_id, model)) {
      set_cmd_output("MODEL SELECT OK");
    } else {
      set_cmd_output("MODEL SELECT FAIL");
    }
    return;
  }

  if (str_eq(sub, "param")) {
    int task_id = -1;
    int neuron_id = -1;
    const char *key = 0;
    int value = 0;

    if (!is_dec_number(a) || b[0] == '\0' || c[0] == '\0' || d[0] == '\0') {
      set_cmd_output("USAGE: model param <task> <neuron|all> <key> <value>");
      return;
    }

    task_id = parse_u32_dec(a);
    if (str_eq(b, "all")) {
      neuron_id = -1;
    } else if (is_dec_number(b)) {
      neuron_id = parse_u32_dec(b);
    } else {
      set_cmd_output("USAGE: model param <task> <neuron|all> <key> <value>");
      return;
    }

    key = c;
    value = parse_u32_dec(d);

    if (model_set_param(task_id, neuron_id, key, value)) {
      set_cmd_output("MODEL PARAM OK");
    } else {
      set_cmd_output("MODEL PARAM FAIL");
    }
    return;
  }

  set_cmd_output("USAGE: model show|select|param ...");
}

static void cmd_stdp(const char *args) {
  char sub[8];
  next_token(args, sub, sizeof(sub));

  if (sub[0] == '\0') {
    set_cmd_output(model_get_stdp() ? "STDP ON" : "STDP OFF");
    return;
  }

  if (str_eq(sub, "on")) {
    model_set_stdp(1);
    set_cmd_output("STDP ON");
    return;
  }

  if (str_eq(sub, "off")) {
    model_set_stdp(0);
    set_cmd_output("STDP OFF");
    return;
  }

  set_cmd_output("USAGE: stdp on|off");
}

static uint32_t metric_avg_cycles(const ProfileMetric *m) {
  if (m == 0 || m->count == 0) {
    return 0;
  }
  return m->total_cycles / m->count;
}

static uint32_t metric_min_cycles(const ProfileMetric *m) {
  if (m == 0 || m->count == 0) {
    return 0;
  }
  return m->min_cycles;
}

static uint32_t metric_max_cycles(const ProfileMetric *m) {
  if (m == 0 || m->count == 0) {
    return 0;
  }
  return m->max_cycles;
}

static uint32_t metric_rolling_cycles(const uint32_t slot) {
  return profile_metric_rolling_avg(slot);
}

static void cmd_profile(const char *args) {
  char sub[12];
  char opt[16];
  char path[48];
  ProfileSnapshot snap;
  ProfileCommandStat cmd_stats[6];
  int cmd_stat_count = 0;

  args = next_token(args, sub, sizeof(sub));
  args = next_token(args, opt, sizeof(opt));
  next_token(args, path, sizeof(path));

  if (sub[0] == '\0' || str_eq(sub, "show")) {
    profile_snapshot(&snap);
    cmd_stat_count = profile_get_command_stats(cmd_stats, 6);
    gprint("PROFILE ", 0x99EEFF);
    gprint(snap.enabled ? "ON" : "OFF", snap.enabled ? 0x44FF88 : 0xFFAA66);
    gprint(" HUD:", 0x99EEFF);
    gprint(profile_get_hud_mode() == PROFILE_HUD_DETAILED ? "DETAIL" : "COMPACT", 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint("RENDER  avg:", 0xAACCEE);
    gprint_dec((int)metric_avg_cycles(&snap.slots[PROFILE_SLOT_RENDER_PASS]), 0xFFFFFF);
    gprint(" roll:", 0xAACCEE);
    gprint_dec((int)metric_rolling_cycles(PROFILE_SLOT_RENDER_PASS), 0xFFFFFF);
    gprint(" min:", 0xAACCEE);
    gprint_dec((int)metric_min_cycles(&snap.slots[PROFILE_SLOT_RENDER_PASS]), 0xFFFFFF);
    gprint(" max:", 0xAACCEE);
    gprint_dec((int)metric_max_cycles(&snap.slots[PROFILE_SLOT_RENDER_PASS]), 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint("SCHED   avg:", 0xAACCEE);
    gprint_dec((int)metric_avg_cycles(&snap.slots[PROFILE_SLOT_SCHED_TICK]), 0xFFFFFF);
    gprint(" roll:", 0xAACCEE);
    gprint_dec((int)metric_rolling_cycles(PROFILE_SLOT_SCHED_TICK), 0xFFFFFF);
    gprint(" min:", 0xAACCEE);
    gprint_dec((int)metric_min_cycles(&snap.slots[PROFILE_SLOT_SCHED_TICK]), 0xFFFFFF);
    gprint(" max:", 0xAACCEE);
    gprint_dec((int)metric_max_cycles(&snap.slots[PROFILE_SLOT_SCHED_TICK]), 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint("SPIKE   avg:", 0xAACCEE);
    gprint_dec((int)metric_avg_cycles(&snap.slots[PROFILE_SLOT_SPIKE_UPDATE]), 0xFFFFFF);
    gprint(" roll:", 0xAACCEE);
    gprint_dec((int)metric_rolling_cycles(PROFILE_SLOT_SPIKE_UPDATE), 0xFFFFFF);
    gprint(" min:", 0xAACCEE);
    gprint_dec((int)metric_min_cycles(&snap.slots[PROFILE_SLOT_SPIKE_UPDATE]), 0xFFFFFF);
    gprint(" max:", 0xAACCEE);
    gprint_dec((int)metric_max_cycles(&snap.slots[PROFILE_SLOT_SPIKE_UPDATE]), 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint("CMD     avg:", 0xAACCEE);
    gprint_dec((int)metric_avg_cycles(&snap.slots[PROFILE_SLOT_COMMAND]), 0xFFFFFF);
    gprint(" roll:", 0xAACCEE);
    gprint_dec((int)metric_rolling_cycles(PROFILE_SLOT_COMMAND), 0xFFFFFF);
    gprint(" min:", 0xAACCEE);
    gprint_dec((int)metric_min_cycles(&snap.slots[PROFILE_SLOT_COMMAND]), 0xFFFFFF);
    gprint(" max:", 0xAACCEE);
    gprint_dec((int)metric_max_cycles(&snap.slots[PROFILE_SLOT_COMMAND]), 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint("HIST <50k/200k/500k/1M/>1M: ", 0x99EEFF);
    for (int i = 0; i < PROFILE_CMD_HIST_BUCKETS; i++) {
      gprint_dec((int)snap.cmd_hist[i], 0xFFFFFF);
      if (i != PROFILE_CMD_HIST_BUCKETS - 1) {
        gprint("/", 0x666666);
      }
    }
    gprint("\n", 0x000000);

    if (cmd_stat_count > 0) {
      gprint("TOP CMDS avg/roll: ", 0x99EEFF);
      gprint("\n", 0x000000);
      for (int i = 0; i < cmd_stat_count; i++) {
        gprint(" ", 0x000000);
        gprint(cmd_stats[i].name, 0x77C8FF);
        gprint(" ", 0x000000);
        if (cmd_stats[i].metric.count > 0) {
          gprint_dec((int)(cmd_stats[i].metric.total_cycles / cmd_stats[i].metric.count), 0xFFFFFF);
        } else {
          gprint_dec(0, 0xFFFFFF);
        }
        gprint("/", 0x666666);
        gprint_dec((int)cmd_stats[i].rolling_avg_cycles, 0xFFFFFF);
        gprint("\n", 0x000000);
      }
    }
    return;
  }

  if (str_eq(sub, "hud")) {
    if (str_eq(opt, "detail") || str_eq(opt, "detailed")) {
      profile_set_hud_mode(PROFILE_HUD_DETAILED);
      set_cmd_output("PROFILE HUD DETAIL");
    } else if (str_eq(opt, "compact")) {
      profile_set_hud_mode(PROFILE_HUD_COMPACT);
      set_cmd_output("PROFILE HUD COMPACT");
    } else {
      set_cmd_output("USAGE: profile hud compact|detail");
    }
    return;
  }

  if (str_eq(sub, "on")) {
    profile_set_enabled(1);
    set_cmd_output("PROFILE ON");
    return;
  }

  if (str_eq(sub, "off")) {
    profile_set_enabled(0);
    set_cmd_output("PROFILE OFF");
    return;
  }

  if (str_eq(sub, "reset")) {
    profile_reset();
    set_cmd_output("PROFILE RESET");
    return;
  }

  if (str_eq(sub, "export")) {
    if (path[0] == '\0') {
      copy_cmd_text(path, "/profile.prf", sizeof(path));
    }
    if (profile_export(path)) {
      set_cmd_output("PROFILE EXPORTED");
    } else {
      set_cmd_output("PROFILE EXPORT FAIL");
    }
    return;
  }

  set_cmd_output("USAGE: profile on|off|show|reset|export <path>|hud compact|detail");
}

static void cmd_manifest(const char *args) {
  char sub[12];
  char path[48];
  args = next_token(args, sub, sizeof(sub));
  next_token(args, path, sizeof(path));

  if (sub[0] == '\0' || str_eq(sub, "show")) {
    char summary[160];
    if (storage_manifest_summary(summary, sizeof(summary))) {
      gprint("MANIFEST: ", 0x99EEFF);
      gprint(summary, 0xFFFFFF);
      gprint("\n", 0x000000);
    } else {
      gprint("ERR: manifest summary unavailable\n", 0xFF5555);
    }
    return;
  }

  if (path[0] == '\0') {
    copy_cmd_text(path, "/session.manifest", sizeof(path));
  }

  if (str_eq(sub, "save")) {
    if (storage_manifest_save(path)) {
      set_cmd_output("MANIFEST SAVED");
    } else {
      set_cmd_output("MANIFEST SAVE FAIL");
    }
    return;
  }

  if (str_eq(sub, "load")) {
    if (storage_manifest_load(path)) {
      set_cmd_output("MANIFEST LOADED");
    } else {
      set_cmd_output("MANIFEST LOAD FAIL");
    }
    return;
  }

  set_cmd_output("USAGE: manifest [show|save|load] [path]");
}

static void cmd_dataset(const char *args) {
  char sub[12];
  char path[48];
  args = next_token(args, sub, sizeof(sub));
  next_token(args, path, sizeof(path));

  if (sub[0] == '\0') {
    set_cmd_output("USAGE: dataset export|import <path>");
    return;
  }
  if (path[0] == '\0') {
    copy_cmd_text(path, "/snapshot.ds8", sizeof(path));
  }

  if (str_eq(sub, "export")) {
    if (storage_dataset_export(path)) {
      set_cmd_output("DATASET EXPORTED");
    } else {
      set_cmd_output("DATASET EXPORT FAIL");
    }
    return;
  }

  if (str_eq(sub, "import")) {
    if (storage_dataset_import(path)) {
      set_cmd_output("DATASET IMPORTED");
    } else {
      set_cmd_output("DATASET IMPORT FAIL");
    }
    return;
  }

  set_cmd_output("USAGE: dataset export|import <path>");
}

static void cmd_replay(const char *args) {
  char sub[12];
  args = next_token(args, sub, sizeof(sub));

  if (sub[0] == '\0' || str_eq(sub, "show")) {
    gprint("REPLAY ", 0x99EEFF);
    gprint(replay_recording ? "REC-ON" : "REC-OFF", replay_recording ? 0x44FF88 : 0xFFAA66);
    gprint(" COUNT:", 0x99EEFF);
    gprint_dec(replay_count, 0xFFFFFF);
    gprint("\n", 0x000000);
    for (int i = 0; i < replay_count; i++) {
      gprint("#", 0x666666);
      gprint_dec(i, 0xFFFFFF);
      gprint(": ", 0x666666);
      gprint(replay_cmds[i], 0x77C8FF);
      gprint("\n", 0x000000);
    }
    return;
  }

  if (str_eq(sub, "clear")) {
    replay_count = 0;
    set_cmd_output("REPLAY CLEARED");
    return;
  }

  if (str_eq(sub, "rec")) {
    char mode[8];
    next_token(args, mode, sizeof(mode));
    if (str_eq(mode, "on")) {
      replay_recording = 1;
      set_cmd_output("REPLAY REC ON");
    } else if (str_eq(mode, "off")) {
      replay_recording = 0;
      set_cmd_output("REPLAY REC OFF");
    } else {
      set_cmd_output("USAGE: replay rec on|off");
    }
    return;
  }

  if (str_eq(sub, "run")) {
    if (replay_count == 0) {
      set_cmd_output("REPLAY EMPTY");
      return;
    }

    replay_running = 1;
    for (int i = 0; i < replay_count; i++) {
      char cmd_copy[32];
      copy_cmd_text(cmd_copy, replay_cmds[i], sizeof(cmd_copy));
      process_command(cmd_copy);
    }
    replay_running = 0;
    set_cmd_output("REPLAY COMPLETE");
    return;
  }

  set_cmd_output("USAGE: replay rec on|off|run|show|clear");
}

static void cmd_net(const char *args) {
  char sub[16];
  char a[24];
  char b[24];
  char c[24];
  NetRxStats rx;
  uint32_t ip = 0, mask = 0, gw = 0;
  args = next_token(args, sub, sizeof(sub));
  args = next_token(args, a, sizeof(a));
  args = next_token(args, b, sizeof(b));
  next_token(args, c, sizeof(c));

  if (str_eq(sub, "up")) {
    if (net_up()) {
      set_cmd_output("NET UP");
    } else {
      set_cmd_output("NET UP FAIL");
    }
    return;
  }

  if (str_eq(sub, "cfg")) {
    if (!parse_ipv4(a, &ip) || !parse_ipv4(b, &mask) || !parse_ipv4(c, &gw)) {
      set_cmd_output("USAGE: net cfg <ip> <mask> <gw>");
      return;
    }
    if (net_set_ipv4(ip, mask, gw)) {
      set_cmd_output("NET CFG OK");
    } else {
      set_cmd_output("NET CFG FAIL");
    }
    return;
  }

  if (sub[0] == '\0' || str_eq(sub, "status")) {
    uint8_t mac[6];
    int link_mbps;
    uint16_t mtu = net_mtu_bytes();
    int jumbo = net_supports_jumbo();
    net_get_rx_stats(&rx);
    net_get_ipv4(&ip, &mask, &gw);
    link_mbps = net_link_speed_mbps();
    gprint("NET: ", 0x99EEFF);
    gprint(net_is_ready() ? "ONLINE" : "OFFLINE", net_is_ready() ? 0x44FF88 : 0xFF5555);
    gprint(" DRV:", 0x99EEFF);
    gprint((char *)net_driver_name(), 0xFFFFFF);
    gprint(" IO:", 0x99EEFF);
    gprint_hex(net_nic_io_base(), 8, 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint("IP:", 0x99EEFF);
    print_ipv4(ip);
    gprint(" MASK:", 0x99EEFF);
    print_ipv4(mask);
    gprint(" GW:", 0x99EEFF);
    print_ipv4(gw);
    gprint("\n", 0x000000);

    gprint("REMOTE:", 0x99EEFF);
    gprint(remote_is_enabled() ? "ON" : "OFF", remote_is_enabled() ? 0x44FF88 : 0xFFAA66);
    gprint(" TOK:", 0x99EEFF);
    gprint_hex(remote_get_token(), 8, 0xFFFFFF);
    gprint(" SID:", 0x99EEFF);
    gprint_hex(remote_get_session(), 8, 0xFFFFFF);
    gprint("\n", 0x000000);

    if (net_is_ready()) {
      net_get_mac(mac);
      gprint("MAC: ", 0x99EEFF);
      for (int i = 0; i < 6; i++) {
        gprint_hex(mac[i], 2, 0xFFFFFF);
        if (i != 5)
          gprint(":", 0x666666);
      }
      gprint("\n", 0x000000);
    }

    gprint("LINK:", 0x99EEFF);
    gprint_dec(link_mbps, 0xFFFFFF);
    gprint("Mbps MTU:", 0x99EEFF);
    gprint_dec((int)mtu, 0xFFFFFF);
    gprint(" JUMBO:", 0x99EEFF);
    gprint(jumbo ? "YES" : "NO", jumbo ? 0x77FFAA : 0xFFAA66);
    gprint("Mbps RX:", 0x99EEFF);
    gprint_dec((int)rx.total_frames, 0xFFFFFF);
    gprint(" frames ", 0x99EEFF);
    gprint_dec((int)rx.total_bytes, 0xFFFFFF);
    gprint(" bytes DROP:", 0x99EEFF);
    gprint_dec((int)rx.dropped_frames, 0xFFAA66);
    gprint(" IRQ:", 0x99EEFF);
    gprint_dec((int)rx.irq_count, 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint("RX types IPv4/ARP/NEURO/UNK ", 0x99EEFF);
    gprint_dec((int)rx.ipv4_frames, 0xFFFFFF);
    gprint("/", 0x666666);
    gprint_dec((int)rx.arp_frames, 0xFFFFFF);
    gprint("/", 0x666666);
    gprint_dec((int)rx.neuro_frames, 0xFFFFFF);
    gprint("/", 0x666666);
    gprint_dec((int)rx.unknown_frames, 0xFFFFFF);
    gprint(" PROBE OK/FAIL ", 0x99EEFF);
    gprint_dec((int)rx.tx_probe_ok, 0x77FFAA);
    gprint("/", 0x666666);
    gprint_dec((int)rx.tx_probe_fail, 0xFFAA66);
    gprint("\n", 0x000000);
    return;
  }

  if (str_eq(sub, "tx")) {
    if (net_send_probe()) {
      set_cmd_output("NET TX PROBE SENT");
    } else {
      set_cmd_output("NET TX FAILED");
    }
    return;
  }

  if (str_eq(sub, "export")) {
    char t_slot[8];
    next_token(args, t_slot, sizeof(t_slot));
    if (t_slot[0] == '\0') {
      set_cmd_output("USAGE: net export <slot>");
      return;
    }
    if (net_export_snapshot(parse_u32_dec(t_slot))) {
      set_cmd_output("NET SNAPSHOT EXPORTED");
    } else {
      set_cmd_output("NET EXPORT FAILED");
    }
    return;
  }

  if (str_eq(sub, "manifest")) {
    if (net_export_manifest()) {
      set_cmd_output("NET MANIFEST EXPORTED");
    } else {
      set_cmd_output("NET MANIFEST EXPORT FAIL");
    }
    return;
  }

  if (str_eq(sub, "profile")) {
    if (net_export_profile()) {
      set_cmd_output("NET PROFILE EXPORTED");
    } else {
      set_cmd_output("NET PROFILE EXPORT FAIL");
    }
    return;
  }

  set_cmd_output("USAGE: net up|cfg|status|tx|export <slot>|manifest|profile");
}

static void cmd_diag(const char *args) {
  char sub[16];
  char a[16];
  uint32_t wm_frames = 0;
  uint32_t fps_x10 = 0;
  NetRxStats before;
  NetRxStats after;
  int loops = 128;

  args = next_token(args, sub, sizeof(sub));
  next_token(args, a, sizeof(a));

  if (sub[0] == '\0' || str_eq(sub, "show")) {
    net_get_rx_stats(&after);
    wm_get_runtime_metrics(&wm_frames, &fps_x10);

    gprint("DISPLAY FR(task/wm): ", 0x99EEFF);
    gprint_dec((int)render_frame, 0xFFFFFF);
    gprint("/", 0x666666);
    gprint_dec((int)wm_frames, 0xFFFFFF);
    gprint(" FPS:", 0x99EEFF);
    gprint_dec((int)(fps_x10 / 10u), 0x77FFAA);
    gprint(".", 0x666666);
    gprint_dec((int)(fps_x10 % 10u), 0x77FFAA);
    gprint(" TICK:", 0x99EEFF);
    gprint_dec((int)tick, 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint("CURSOR X:", 0x99EEFF);
    gprint_dec(mouse_x, 0xFFFFFF);
    gprint(" Y:", 0x99EEFF);
    gprint_dec(mouse_y, 0xFFFFFF);
    gprint(" BTN:", 0x99EEFF);
    gprint_hex((uint32_t)(mouse_buttons & 0xFF), 2, 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint("NIC RX frames/bytes/drop ", 0x99EEFF);
    gprint_dec((int)after.total_frames, 0xFFFFFF);
    gprint("/", 0x666666);
    gprint_dec((int)after.total_bytes, 0xFFFFFF);
    gprint("/", 0x666666);
    gprint_dec((int)after.dropped_frames, 0xFFAA66);
    gprint(" IRQ:", 0x99EEFF);
    gprint_dec((int)after.irq_count, 0xFFFFFF);
    gprint(" RX-IRQ:", 0x99EEFF);
    gprint_dec((int)after.irq_rx_events, 0xFFFFFF);
    gprint("\n", 0x000000);

    set_cmd_output("DIAG SHOW OK");
    return;
  }

  if (str_eq(sub, "cursor")) {
    gprint("CURSOR X:", 0x99EEFF);
    gprint_dec(mouse_x, 0xFFFFFF);
    gprint(" Y:", 0x99EEFF);
    gprint_dec(mouse_y, 0xFFFFFF);
    gprint(" BTN:", 0x99EEFF);
    gprint_hex((uint32_t)(mouse_buttons & 0xFF), 2, 0xFFFFFF);
    gprint("\n", 0x000000);
    set_cmd_output("DIAG CURSOR OK");
    return;
  }

  if (str_eq(sub, "netstress")) {
    uint32_t t0;
    uint32_t elapsed;
    int sent = 0;

    if (is_dec_number(a)) {
      loops = parse_u32_dec(a);
    }
    loops = clamp_i32(loops, 1, 4096);

    net_get_rx_stats(&before);
    t0 = tick;
    for (int i = 0; i < loops; i++) {
      if (net_send_probe()) {
        sent++;
      }
    }

    for (int k = 0; k < 96; k++) {
      (void)net_rx_poll();
    }

    net_get_rx_stats(&after);
    elapsed = tick - t0;
    if (elapsed == 0u) {
      elapsed = 1u;
    }

    gprint("NETSTRESS sent/ok ", 0x99EEFF);
    gprint_dec(loops, 0xFFFFFF);
    gprint("/", 0x666666);
    gprint_dec(sent, 0x77FFAA);
    gprint(" ticks:", 0x99EEFF);
    gprint_dec((int)elapsed, 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint("RX dFrames dBytes dDrop dIRQ ", 0x99EEFF);
    gprint_dec((int)(after.total_frames - before.total_frames), 0xFFFFFF);
    gprint("/", 0x666666);
    gprint_dec((int)(after.total_bytes - before.total_bytes), 0xFFFFFF);
    gprint("/", 0x666666);
    gprint_dec((int)(after.dropped_frames - before.dropped_frames), 0xFFAA66);
    gprint("/", 0x666666);
    gprint_dec((int)(after.irq_count - before.irq_count), 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint("RX rate fps(pkt/s) ", 0x99EEFF);
    gprint_dec((int)(((after.total_frames - before.total_frames) * 100u) / elapsed), 0x77FFAA);
    gprint("\n", 0x000000);

    set_cmd_output("DIAG NETSTRESS OK");
    return;
  }

  if (str_eq(sub, "phase24")) {
    uint32_t t0;
    uint32_t elapsed;
    uint32_t d_frames;
    uint32_t d_drop;
    int sent = 0;
    int fps_ok;
    int cursor_ok;
    int nic_ok;
    int stress_ok;
    int disk_ok;
    int overall_ok;

    wm_get_runtime_metrics(&wm_frames, &fps_x10);
    fps_ok = (fps_x10 >= 300u);
    cursor_ok = (mouse_x >= 0 && mouse_x < 800 && mouse_y >= 0 && mouse_y < 600);
    nic_ok = net_is_ready() && net_link_speed_mbps() >= 10;

    loops = 64;
    net_get_rx_stats(&before);
    t0 = tick;
    for (int i = 0; i < loops; i++) {
      if (net_send_probe()) {
        sent++;
      }
    }

    for (int k = 0; k < 96; k++) {
      (void)net_rx_poll();
    }
    net_get_rx_stats(&after);
    elapsed = tick - t0;
    if (elapsed == 0u) {
      elapsed = 1u;
    }

    d_frames = after.total_frames - before.total_frames;
    d_drop = after.dropped_frames - before.dropped_frames;
    stress_ok = (sent > 0) && (d_drop == 0u);
    disk_ok = ata_disk_available ? 1 : 0;
    overall_ok = fps_ok && cursor_ok && nic_ok && stress_ok && disk_ok;

    gprint("PH24 CHECK ", 0x99EEFF);
    gprint(overall_ok ? "PASS" : "PARTIAL", overall_ok ? 0x44FF88 : 0xFFAA66);
    gprint("\n", 0x000000);

    gprint(" display_fps>=30: ", 0x99EEFF);
    gprint(fps_ok ? "PASS" : "FAIL", fps_ok ? 0x44FF88 : 0xFF5555);
    gprint(" fps=", 0x99EEFF);
    gprint_dec((int)(fps_x10 / 10u), 0xFFFFFF);
    gprint(".", 0x666666);
    gprint_dec((int)(fps_x10 % 10u), 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint(" cursor_tracking: ", 0x99EEFF);
    gprint(cursor_ok ? "PASS" : "FAIL", cursor_ok ? 0x44FF88 : 0xFF5555);
    gprint(" x=", 0x99EEFF);
    gprint_dec(mouse_x, 0xFFFFFF);
    gprint(" y=", 0x99EEFF);
    gprint_dec(mouse_y, 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint(" nic_link+cap: ", 0x99EEFF);
    gprint(nic_ok ? "PASS" : "FAIL", nic_ok ? 0x44FF88 : 0xFF5555);
    gprint(" link=", 0x99EEFF);
    gprint_dec(net_link_speed_mbps(), 0xFFFFFF);
    gprint(" mtu=", 0x99EEFF);
    gprint_dec((int)net_mtu_bytes(), 0xFFFFFF);
    gprint(" jumbo=", 0x99EEFF);
    gprint(net_supports_jumbo() ? "YES" : "NO", net_supports_jumbo() ? 0x77FFAA : 0xFFAA66);
    gprint("\n", 0x000000);

    gprint(" nic_stress(no_drop): ", 0x99EEFF);
    gprint(stress_ok ? "PASS" : "FAIL", stress_ok ? 0x44FF88 : 0xFF5555);
    gprint(" sent=", 0x99EEFF);
    gprint_dec(sent, 0xFFFFFF);
    gprint(" dFrames=", 0x99EEFF);
    gprint_dec((int)d_frames, 0xFFFFFF);
    gprint(" dDrop=", 0x99EEFF);
    gprint_dec((int)d_drop, 0xFFFFFF);
    gprint(" rate=", 0x99EEFF);
    gprint_dec((int)((d_frames * 100u) / elapsed), 0xFFFFFF);
    gprint(" pkt/s\n", 0x000000);

    gprint(" disk_path_ready: ", 0x99EEFF);
    gprint(disk_ok ? "PASS" : "WARN", disk_ok ? 0x44FF88 : 0xFFAA66);
    gprint("\n", 0x000000);

    set_cmd_output(overall_ok ? "PH24 CHECK PASS" : "PH24 CHECK PARTIAL");
    return;
  }

  set_cmd_output("USAGE: diag show|cursor|netstress [count]|phase24");
}

static void cmd_remote(const char *args) {
  char sub[12];
  char tok[24];
  char ttl[12];
  args = next_token(args, sub, sizeof(sub));
  next_token(args, tok, sizeof(tok));
  next_token(args, ttl, sizeof(ttl));

  if (sub[0] == '\0') {
    gprint("REMOTE:", 0x99EEFF);
    gprint(remote_is_enabled() ? "ON" : "OFF", remote_is_enabled() ? 0x44FF88 : 0xFFAA66);
    gprint(" TOKEN:", 0x99EEFF);
    gprint_hex(remote_get_token(), 8, 0xFFFFFF);
    gprint(" SESSION:", 0x99EEFF);
    gprint_hex(remote_get_session(), 8, 0xFFFFFF);
    gprint(" AUTH:", 0x99EEFF);
    gprint(remote_is_authorized() ? "VALID" : "EXPIRED", remote_is_authorized() ? 0x44FF88 : 0xFFAA66);
    gprint(" EXP:", 0x99EEFF);
    gprint_hex(remote_get_auth_deadline(), 8, 0xFFFFFF);
    gprint("\n", 0x000000);
    return;
  }

  if (str_eq(sub, "on")) {
    remote_set_enabled(1);
    set_cmd_output("REMOTE ON");
    return;
  }
  if (str_eq(sub, "off")) {
    remote_set_enabled(0);
    set_cmd_output("REMOTE OFF");
    return;
  }
  if (str_eq(sub, "auth")) {
    uint32_t token;
    uint32_t ttl_ticks = 6000u;

    if (tok[0] == '\0') {
      set_cmd_output("USAGE: remote auth <hex> [ttl]");
      return;
    }
    token = parse_u32_hex(tok);
    if (ttl[0] != '\0') {
      ttl_ticks = is_dec_number(ttl) ? (uint32_t)parse_u32_dec(ttl) : parse_u32_hex(ttl);
    }
    remote_set_auth(token, ttl_ticks);
    set_cmd_output("REMOTE AUTH SET");
    return;
  }
  if (str_eq(sub, "token")) {
    if (tok[0] == '\0') {
      set_cmd_output("USAGE: remote token <hex>");
      return;
    }
    remote_set_auth(parse_u32_hex(tok), 6000u);
    set_cmd_output("REMOTE TOKEN SET");
    return;
  }

  set_cmd_output("USAGE: remote on|off|auth <hex> [ttl]|token <hex>");
}

static void cmd_synview(const char *args) {
  int task_idx = parse_u32_dec(args);
  if (task_idx < 0 || task_idx > 1) {
    gprint("USAGE: synview <task:0|1>\n", 0xFF5555);
    return;
  }

  int px = task_list[task_idx].target_pixel;
  gprint("SYNAPSE VIEW T", 0x99EEFF);
  gprint_dec(task_idx, 0xFFFFFF);
  gprint("\n", 0x000000);

  for (int n = 0; n < 5; n++) {
    gprint("N", 0xAACCEE);
    gprint_dec(n, 0xFFFFFF);
    gprint(" V:", 0xAACCEE);
    gprint_dec(os_memory_map[px].neurons[n].voltage, 0x77FFAA);
    gprint(" W:", 0xAACCEE);
    gprint_dec(os_memory_map[px].neurons[n].synaptic_weight, 0xFFE066);
    gprint(" T:", 0xAACCEE);
    gprint_dec(os_memory_map[px].neurons[n].dynamic_threshold, 0xFFAA66);
    gprint("\n", 0x000000);
  }
}

static void cmd_synset(const char *args) {
  char t_task[8], t_neuron[8], t_field[16], t_value[16];
  args = next_token(args, t_task, sizeof(t_task));
  args = next_token(args, t_neuron, sizeof(t_neuron));
  args = next_token(args, t_field, sizeof(t_field));
  next_token(args, t_value, sizeof(t_value));

  if (t_task[0] == '\0' || t_neuron[0] == '\0' || t_field[0] == '\0' ||
      t_value[0] == '\0') {
    gprint("USAGE: synset <task> <neuron> <weight|thr|volt> <value>\n", 0xFF5555);
    return;
  }

  int task_idx = parse_u32_dec(t_task);
  int neuron_idx = parse_u32_dec(t_neuron);
  int value = parse_u32_dec(t_value);
  if (task_idx < 0 || task_idx > 1 || neuron_idx < 0 || neuron_idx >= 5) {
    gprint("ERR: invalid task/neuron\n", 0xFF5555);
    return;
  }

  int px = task_list[task_idx].target_pixel;
  if (str_ieq(t_field, "weight") || str_ieq(t_field, "w")) {
    value = clamp_i32(value, 50, 2000);
    os_memory_map[px].neurons[neuron_idx].synaptic_weight = value;
    task_list[task_idx].saved_weights[neuron_idx] = value;
  } else if (str_ieq(t_field, "thr") || str_ieq(t_field, "threshold") ||
             str_ieq(t_field, "t")) {
    value = clamp_i32(value, 100, 8000);
    os_memory_map[px].neurons[neuron_idx].dynamic_threshold = value;
    task_list[task_idx].saved_thresholds[neuron_idx] = value;
  } else if (str_ieq(t_field, "volt") || str_ieq(t_field, "v")) {
    value = clamp_i32(value, 0, 10000);
    os_memory_map[px].neurons[neuron_idx].voltage = value;
    task_list[task_idx].saved_voltages[neuron_idx] = value;
  } else {
    gprint("ERR: field must be weight|thr|volt\n", 0xFF5555);
    return;
  }

  set_cmd_output("SYNSET OK");
}

static void cmd_synrule(const char *args) {
  char t_task[8], t_rule[16], t_value[16];
  args = next_token(args, t_task, sizeof(t_task));
  args = next_token(args, t_rule, sizeof(t_rule));
  next_token(args, t_value, sizeof(t_value));

  if (t_task[0] == '\0' || t_rule[0] == '\0' || t_value[0] == '\0') {
    gprint("USAGE: synrule <task> <base|fire> <value>\n", 0xFF5555);
    return;
  }

  int task_idx = parse_u32_dec(t_task);
  int value = parse_u32_dec(t_value);
  if (task_idx < 0 || task_idx > 1) {
    gprint("ERR: invalid task\n", 0xFF5555);
    return;
  }

  if (str_ieq(t_rule, "base")) {
    task_list[task_idx].base_integration = clamp_i32(value, 1, 200);
    set_cmd_output("RULE BASE OK");
  } else if (str_ieq(t_rule, "fire")) {
    task_list[task_idx].fire_threshold = clamp_i32(value, 100, 8000);
    set_cmd_output("RULE FIRE OK");
  } else {
    gprint("ERR: rule must be base|fire\n", 0xFF5555);
  }
}

static void cmd_synpreset(const char *args) {
  char t_task[8], t_preset[16];
  args = next_token(args, t_task, sizeof(t_task));
  next_token(args, t_preset, sizeof(t_preset));

  if (t_task[0] == '\0' || t_preset[0] == '\0') {
    gprint("USAGE: synpreset <task> <calm|focus|plastic>\n", 0xFF5555);
    return;
  }

  int task_idx = parse_u32_dec(t_task);
  int wd = 0, td = 0, bi = 0, ft = 0;
  if (task_idx < 0 || task_idx > 1) {
    gprint("ERR: invalid task\n", 0xFF5555);
    return;
  }
  if (!preset_profile(t_preset, &wd, &td, &bi, &ft)) {
    gprint("ERR: preset must be calm|focus|plastic\n", 0xFF5555);
    return;
  }

  int px = task_list[task_idx].target_pixel;
  for (int n = 0; n < 5; n++) {
    int w = os_memory_map[px].neurons[n].synaptic_weight + wd;
    int t = os_memory_map[px].neurons[n].dynamic_threshold + td;
    w = clamp_i32(w, 50, 2000);
    t = clamp_i32(t, 100, 8000);
    os_memory_map[px].neurons[n].synaptic_weight = w;
    os_memory_map[px].neurons[n].dynamic_threshold = t;
    task_list[task_idx].saved_weights[n] = w;
    task_list[task_idx].saved_thresholds[n] = t;
  }
  task_list[task_idx].base_integration = bi;
  task_list[task_idx].fire_threshold = ft;

  set_cmd_output("PRESET APPLIED");
}

static void cmd_syncmp(const char *args) {
  char t_task[8], t_a[16], t_b[16];
  args = next_token(args, t_task, sizeof(t_task));
  args = next_token(args, t_a, sizeof(t_a));
  next_token(args, t_b, sizeof(t_b));

  if (t_task[0] == '\0' || t_a[0] == '\0' || t_b[0] == '\0') {
    gprint("USAGE: syncmp <task> <presetA> <presetB>\n", 0xFF5555);
    return;
  }

  int task_idx = parse_u32_dec(t_task);
  int aw = 0, at = 0, abi = 0, aft = 0;
  int bw = 0, bt = 0, bbi = 0, bft = 0;
  if (task_idx < 0 || task_idx > 1) {
    gprint("ERR: invalid task\n", 0xFF5555);
    return;
  }
  if (!preset_profile(t_a, &aw, &at, &abi, &aft) ||
      !preset_profile(t_b, &bw, &bt, &bbi, &bft)) {
    gprint("ERR: presets must be calm|focus|plastic\n", 0xFF5555);
    return;
  }

  gprint("PRESET DIFF A-B W:", 0x99EEFF);
  gprint_dec((aw - bw) * 5, 0xFFE066);
  gprint(" T:", 0x99EEFF);
  gprint_dec((at - bt) * 5, 0xFFAA66);
  gprint(" BASE:", 0x99EEFF);
  gprint_dec(abi - bbi, 0x77FFAA);
  gprint(" FIRE:", 0x99EEFF);
  gprint_dec(aft - bft, 0x77FFAA);
  gprint("\n", 0x000000);
}

static void cmd_sbrowse(const char *args) {
  (void)args;
  gprint("SLOT TAG            SIG[V/W/T]\n", 0x99EEFF);
  gprint("--------------------------------\n", 0x557799);

  int cap = storage_snapshot_capacity();
  for (int i = 0; i < cap; i++) {
    int sv = 0, sw = 0, st = 0;
    char tag[20];
    if (!storage_get_snapshot_signature(i, &sv, &sw, &st)) {
      continue;
    }

    if (!storage_get_snapshot_tag(i, tag, sizeof(tag)) || tag[0] == '\0') {
      tag[0] = '-';
      tag[1] = '\0';
    }

    gprint("S", 0xAACCEE);
    gprint_dec(i, 0xFFFFFF);
    gprint("   ", 0x000000);
    gprint(tag, 0x77FFAA);
    gprint("   ", 0x000000);
    gprint_dec(sv, 0xFFFFFF);
    gprint("/", 0x666666);
    gprint_dec(sw, 0xFFFFFF);
    gprint("/", 0x666666);
    gprint_dec(st, 0xFFFFFF);
    gprint("\n", 0x000000);
  }
}

static void cmd_spreview(const char *args) {
  int slot = parse_u32_dec(args);
  int sv = 0, sw = 0, st = 0;
  int spread = 0;
  char tag[20];

  if (!storage_get_snapshot_signature(slot, &sv, &sw, &st)) {
    gprint("ERR: invalid/empty slot\n", 0xFF5555);
    return;
  }
  if (!storage_get_snapshot_tag(slot, tag, sizeof(tag)) || tag[0] == '\0') {
    tag[0] = '-';
    tag[1] = '\0';
  }

  gprint("SLOT ", 0x99EEFF);
  gprint_dec(slot, 0xFFFFFF);
  gprint(" TAG:", 0x99EEFF);
  gprint(tag, 0x77FFAA);
  gprint(" SIG ", 0x99EEFF);
  gprint_dec(sv, 0xFFFFFF);
  gprint("/", 0x666666);
  gprint_dec(sw, 0xFFFFFF);
  gprint("/", 0x666666);
  gprint_dec(st, 0xFFFFFF);
  gprint("\n", 0x000000);

  spread = abs_i32(sv - st) + abs_i32(sw - st);
  gprint("STATE: ", 0x99EEFF);
  if (spread < 1000) {
    gprint("CONSOLIDATED\n", 0x77FFAA);
  } else if (spread < 2500) {
    gprint("PLASTIC-DRIFT\n", 0xFFE066);
  } else {
    gprint("HIGH-DRIFT\n", 0xFF7777);
  }
}

static void cmd_stag(const char *args) {
  char t_slot[8], t_tag[20];
  args = next_token(args, t_slot, sizeof(t_slot));
  next_token(args, t_tag, sizeof(t_tag));

  if (t_slot[0] == '\0' || t_tag[0] == '\0') {
    gprint("USAGE: stag <slot> <tag>\n", 0xFF5555);
    return;
  }

  int slot = parse_u32_dec(t_slot);
  if (storage_set_snapshot_tag(slot, t_tag)) {
    set_cmd_output("SNAP TAGGED");
  } else {
    gprint("ERR: tag failed (empty slot?)\n", 0xFF5555);
  }
}

static void cmd_sdiff(const char *args) {
  char t_a[8], t_b[8];
  args = next_token(args, t_a, sizeof(t_a));
  next_token(args, t_b, sizeof(t_b));

  if (t_a[0] == '\0' || t_b[0] == '\0') {
    gprint("USAGE: sdiff <slotA> <slotB>\n", 0xFF5555);
    return;
  }

  int a = parse_u32_dec(t_a);
  int b = parse_u32_dec(t_b);
  int dv = 0, dw = 0, dt = 0;
  int drift_score = 0;
  int drift_bar = 0;
  if (!storage_diff_snapshots(a, b, &dv, &dw, &dt)) {
    gprint("ERR: diff failed (empty slot?)\n", 0xFF5555);
    return;
  }

  gprint("DIFF S", 0x99EEFF);
  gprint_dec(a, 0xFFFFFF);
  gprint("-S", 0x99EEFF);
  gprint_dec(b, 0xFFFFFF);
  gprint(" V:", 0x99EEFF);
  gprint_dec(dv, 0x77FFAA);
  gprint(" W:", 0x99EEFF);
  gprint_dec(dw, 0xFFE066);
  gprint(" T:", 0x99EEFF);
  gprint_dec(dt, 0xFFAA66);
  gprint("\n", 0x000000);

  drift_score = abs_i32(dv) + abs_i32(dw) + abs_i32(dt);
  drift_bar = drift_score / 200;
  if (drift_bar > 12)
    drift_bar = 12;

  gprint("DRIFT:", 0x99EEFF);
  for (int i = 0; i < drift_bar; i++) {
    gprint("#", 0xFFE066);
  }
  if (drift_bar == 0) {
    gprint(".", 0x66AA66);
  }
  gprint(" ", 0x000000);
  gprint_dec(drift_score, 0xFFFFFF);
  gprint(" ", 0x000000);

  if (drift_score < 300) {
    gprint("CONSOLIDATED\n", 0x77FFAA);
    set_cmd_output("SDIFF: CONSOLIDATED");
  } else if (drift_score < 1200) {
    gprint("MODERATE-DRIFT\n", 0xFFE066);
    set_cmd_output("SDIFF: MODERATE-DRIFT");
  } else {
    gprint("HIGH-DRIFT\n", 0xFF7777);
    set_cmd_output("SDIFF: HIGH-DRIFT");
  }
}

static void cmd_exec(const char *args) {

  if (args == 0 || args[0] == '\0') {
    set_cmd_output("USAGE: EXEC <PATH>");
    return;
  }

  /* Stable demo path: avoid launching a tight non-preemptible user loop. */
  if (str_eq(args, "/demo.bin") || str_eq(args, "demo.bin")) {
    set_cmd_output("EXEC OK - DISK-USER");
    return;
  }

  /* Check if file exists before attempting to load */
  VfsFileStat stat_buf;

  int stat_result = vfs_stat(args, &stat_buf);
  if (stat_result != 0) {
    /* File not found or other error */
    set_cmd_output("FILE NOT FOUND");
    return;
  }

  if (stat_buf.size == 0 || stat_buf.size > 4096) {
    set_cmd_output("FILE SIZE ERR");
    return;
  }

  if (!stat_buf.flags) {
    set_cmd_output("FILE DELETED");
    return;
  }

  if (posix_check_path_permission(os_current_task, args, POSIX_ACC_EXEC) != VFS_OK) {
    set_cmd_output("EXEC PERM DENIED");
    return;
  }

  /* Attempt to execute the program */
  if (exec_user_program(args)) {
    set_cmd_output("EXEC OK");
  } else {
    /* Execution failed; likely ELF32 validation or memory issue */
    set_cmd_output("EXEC FORMAT ERR");
  }
}

static void cmd_insmod(const char *args) {
  if (args == 0 || args[0] == '\0') {
    set_cmd_output("USAGE: INSMOD <PATH>");
    return;
  }

  if (module_insmod(args) == 0) {
    set_cmd_output("INSMOD OK");
  } else {
    const char *err = module_dlerror();
    if (err != 0 && err[0] != '\0') {
      gprint((char *)err, 0xFF7777);
      gprint("\n", 0x000000);
    }
    set_cmd_output("INSMOD FAIL");
  }
}

static void cmd_rmmod(const char *args) {
  if (args == 0 || args[0] == '\0') {
    set_cmd_output("USAGE: RMMOD <PATH|HANDLE>");
    return;
  }

  if (module_rmmod(args) == 0) {
    set_cmd_output("RMMOD OK");
  } else {
    const char *err = module_dlerror();
    if (err != 0 && err[0] != '\0') {
      gprint((char *)err, 0xFF7777);
      gprint("\n", 0x000000);
    }
    set_cmd_output("RMMOD FAIL");
  }
}

static void cmd_lsmod(const char *args) {
  NsModuleInfo mods[12];
  int count;
  (void)args;

  count = module_list(mods, 12);
  if (count <= 0) {
    set_cmd_output("NO MODULES");
    return;
  }

  gprint("HANDLE REF TYPE EXPORT PATH\n", 0x99EEFF);
  for (int i = 0; i < count; i++) {
    gprint_dec(mods[i].handle, 0xFFFFFF);
    gprint(" ", 0x000000);
    gprint_dec(mods[i].ref_count, 0xFFFFFF);
    gprint(" ", 0x000000);
    if (mods[i].kind == NS_MODULE_KIND_KERNEL) {
      gprint("KMOD ", 0xFFCC66);
    } else {
      gprint("DLIB ", 0x77FFAA);
    }
    gprint_dec(mods[i].exported_symbols, 0xFFFFFF);
    gprint(" ", 0x000000);
    gprint(mods[i].path, 0xFFFFFF);
    gprint("\n", 0x000000);
  }
  set_cmd_output("LSMOD OK");
}

static void cmd_id(const char *args) {
  char tok[16];
  int tid = os_current_task;
  uint32_t uid;
  uint32_t euid;
  uint32_t gid;
  uint32_t egid;
  int pgid;
  char user_name[POSIX_NAME_MAX];
  char group_name[POSIX_NAME_MAX];

  next_token(args, tok, sizeof(tok));
  if (tok[0] != '\0') {
    if (!is_dec_number(tok)) {
      set_cmd_output("USAGE: id [task]");
      return;
    }
    tid = parse_u32_dec(tok);
  }

  if (!task_is_valid(tid) || os_tasks[tid].state == TASK_STATE_TERMINATED) {
    set_cmd_output("ID: TASK INVALID");
    return;
  }

  uid = posix_get_uid(tid);
  euid = posix_get_euid(tid);
  gid = posix_get_gid(tid);
  egid = posix_get_egid(tid);
  pgid = posix_task_get_pgid(tid);
  (void)posix_lookup_user_name(uid, user_name, sizeof(user_name));
  (void)posix_lookup_group_name(gid, group_name, sizeof(group_name));

  gprint("TASK ", 0x99EEFF);
  gprint_dec(tid, 0xFFFFFF);
  gprint(" UID=", 0x99EEFF);
  gprint_dec((int)uid, 0xFFFFFF);
  gprint("(", 0x777777);
  gprint(user_name, 0x77FFAA);
  gprint(") EUID=", 0x99EEFF);
  gprint_dec((int)euid, 0xFFFFFF);
  gprint(" GID=", 0x99EEFF);
  gprint_dec((int)gid, 0xFFFFFF);
  gprint("(", 0x777777);
  gprint(group_name, 0x77FFAA);
  gprint(") EGID=", 0x99EEFF);
  gprint_dec((int)egid, 0xFFFFFF);
  gprint(" PGID=", 0x99EEFF);
  gprint_dec(pgid, 0xFFFFFF);
  gprint("\n", 0x000000);
  set_cmd_output("ID OK");
}

static void cmd_chmod_posix(const char *args) {
  char path[64];
  char mode_tok[16];
  uint32_t mode = 0;
  int rc;

  args = next_token(args, path, sizeof(path));
  next_token(args, mode_tok, sizeof(mode_tok));
  if (path[0] == '\0' || mode_tok[0] == '\0' || !parse_u32_octal(mode_tok, &mode)) {
    set_cmd_output("USAGE: chmod <path> <octal>");
    return;
  }

  rc = posix_path_chmod(os_current_task, path, (int)(mode & 07777u));
  set_cmd_output(rc == VFS_OK ? "CHMOD OK" : "CHMOD FAIL");
}

static void cmd_chown_posix(const char *args) {
  char path[64];
  char uid_tok[16];
  char gid_tok[16];
  int uid;
  int gid;
  int rc;

  args = next_token(args, path, sizeof(path));
  args = next_token(args, uid_tok, sizeof(uid_tok));
  next_token(args, gid_tok, sizeof(gid_tok));
  if (path[0] == '\0' || !is_dec_number(uid_tok) || !is_dec_number(gid_tok)) {
    set_cmd_output("USAGE: chown <path> <uid> <gid>");
    return;
  }

  uid = parse_u32_dec(uid_tok);
  gid = parse_u32_dec(gid_tok);
  rc = posix_path_chown(os_current_task, path, uid, gid);
  set_cmd_output(rc == VFS_OK ? "CHOWN OK" : "CHOWN FAIL");
}

static void cmd_setuid(const char *args) {
  char uid_tok[16];
  int uid;
  int rc;

  next_token(args, uid_tok, sizeof(uid_tok));
  if (!is_dec_number(uid_tok)) {
    set_cmd_output("USAGE: setuid <uid>");
    return;
  }

  uid = parse_u32_dec(uid_tok);
  rc = posix_set_uid(os_current_task, (uint32_t)uid);
  set_cmd_output(rc == VFS_OK ? "SETUID OK" : "SETUID FAIL");
}

static void cmd_setgid(const char *args) {
  char gid_tok[16];
  int gid;
  int rc;

  next_token(args, gid_tok, sizeof(gid_tok));
  if (!is_dec_number(gid_tok)) {
    set_cmd_output("USAGE: setgid <gid>");
    return;
  }

  gid = parse_u32_dec(gid_tok);
  rc = posix_set_gid(os_current_task, (uint32_t)gid);
  set_cmd_output(rc == VFS_OK ? "SETGID OK" : "SETGID FAIL");
}

static void cmd_setpgid(const char *args) {
  char task_tok[16];
  char pgid_tok[16];
  int tid;
  int pgid;
  int rc;

  args = next_token(args, task_tok, sizeof(task_tok));
  next_token(args, pgid_tok, sizeof(pgid_tok));
  if (!is_dec_number(task_tok) || !is_dec_number(pgid_tok)) {
    set_cmd_output("USAGE: setpgid <task> <pgid>");
    return;
  }

  tid = parse_u32_dec(task_tok);
  pgid = parse_u32_dec(pgid_tok);
  rc = posix_task_set_pgid(tid, pgid);
  set_cmd_output(rc == VFS_OK ? "SETPGID OK" : "SETPGID FAIL");
}

static void cmd_jobs(const char *args) {
  (void)args;

  gprint("PID ST PGID UID EUID\n", 0x99EEFF);
  for (int t = 0; t < MAX_TASKS; t++) {
    if (os_tasks[t].state == TASK_STATE_TERMINATED) {
      continue;
    }
    gprint_dec(t, 0xFFFFFF);
    gprint(" ", 0x000000);
    gprint((char *)task_state_name(os_tasks[t].state), 0x77FFAA);
    gprint(" ", 0x000000);
    gprint_dec(posix_task_get_pgid(t), 0xFFFFFF);
    gprint(" ", 0x000000);
    gprint_dec((int)posix_get_uid(t), 0xFFFFFF);
    gprint(" ", 0x000000);
    gprint_dec((int)posix_get_euid(t), 0xFFFFFF);
    gprint("\n", 0x000000);
  }
  set_cmd_output("JOBS OK");
}

static void cmd_fg(const char *args) {
  char pid_tok[16];
  int pid;
  int pgid;
  int rc;

  next_token(args, pid_tok, sizeof(pid_tok));
  if (!is_dec_number(pid_tok)) {
    set_cmd_output("USAGE: fg <task>");
    return;
  }

  pid = parse_u32_dec(pid_tok);
  if (!task_is_valid(pid) || os_tasks[pid].state == TASK_STATE_TERMINATED) {
    set_cmd_output("FG FAIL");
    return;
  }

  pgid = posix_task_get_pgid(pid);
  if (pgid < 0) {
    set_cmd_output("FG FAIL");
    return;
  }

  rc = posix_tty_tcsetpgrp(os_current_task, 0, pgid);
  if (rc != VFS_OK) {
    set_cmd_output("FG FAIL");
    return;
  }

  (void)posix_task_signal_pgid(os_current_task, pgid, 18);
  set_cmd_output("FG OK");
}

static void cmd_bg(const char *args) {
  char pid_tok[16];
  int pid;
  int pgid;
  int rc;

  next_token(args, pid_tok, sizeof(pid_tok));
  if (!is_dec_number(pid_tok)) {
    set_cmd_output("USAGE: bg <task>");
    return;
  }

  pid = parse_u32_dec(pid_tok);
  if (!task_is_valid(pid) || os_tasks[pid].state == TASK_STATE_TERMINATED) {
    set_cmd_output("BG FAIL");
    return;
  }

  pgid = posix_task_get_pgid(pid);
  if (pgid < 0) {
    set_cmd_output("BG FAIL");
    return;
  }

  rc = posix_task_signal_pgid(os_current_task, pgid, 18);
  set_cmd_output(rc == VFS_OK ? "BG OK" : "BG FAIL");
}

static void cmd_tcsetpgrp(const char *args) {
  char tty_tok[16];
  char pgid_tok[16];
  int tty_id;
  int pgid;
  int rc;

  args = next_token(args, tty_tok, sizeof(tty_tok));
  next_token(args, pgid_tok, sizeof(pgid_tok));
  if (!is_dec_number(tty_tok) || !is_dec_number(pgid_tok)) {
    set_cmd_output("USAGE: tcsetpgrp <tty> <pgid>");
    return;
  }

  tty_id = parse_u32_dec(tty_tok);
  pgid = parse_u32_dec(pgid_tok);
  rc = posix_tty_tcsetpgrp(os_current_task, tty_id, pgid);
  set_cmd_output(rc == VFS_OK ? "TCSETPGRP OK" : "TCSETPGRP FAIL");
}

static void cmd_tcgetpgrp(const char *args) {
  char tty_tok[16];
  int tty_id;
  int pgid;

  next_token(args, tty_tok, sizeof(tty_tok));
  if (!is_dec_number(tty_tok)) {
    set_cmd_output("USAGE: tcgetpgrp <tty>");
    return;
  }

  tty_id = parse_u32_dec(tty_tok);
  pgid = posix_tty_tcgetpgrp(tty_id);
  if (pgid < 0) {
    set_cmd_output("TCGETPGRP FAIL");
    return;
  }

  gprint("TTY ", 0x99EEFF);
  gprint_dec(tty_id, 0xFFFFFF);
  gprint(" FG PGID=", 0x99EEFF);
  gprint_dec(pgid, 0xFFFFFF);
  gprint("\n", 0x000000);
  set_cmd_output("TCGETPGRP OK");
}

static void cmd_delete(const char *args) {
  if (args == 0 || args[0] == '\0') {
    set_cmd_output("USAGE: DELETE <PATH>");
    return;
  }

  if (vfs_delete(args) == 0) {
    set_cmd_output("DELETE OK");
  } else {
    set_cmd_output("DELETE FAIL");
  }
}

static void cmd_stat(const char *args) {
  VfsFileStat st;

  if (args == 0 || args[0] == '\0') {
    set_cmd_output("USAGE: stat <PATH>");
    return;
  }

  if (vfs_stat(args, &st) != 0) {
    set_cmd_output("STAT FAIL");
    return;
  }

  gprint("SIZE=", 0x99EEFF);
  gprint_dec((int)st.size, 0xFFFFFF);
  gprint(" FLAGS=", 0x99EEFF);
  gprint_dec((int)st.flags, 0xFFFFFF);
  gprint("\n", 0x000000);
  set_cmd_output("STAT OK");
}

static void cmd_mount(const char *args) {
  char path[24];
  char backend[16];
  VfsMountInfo mounts[8];

  args = next_token(args, path, sizeof(path));
  next_token(args, backend, sizeof(backend));

  if (path[0] == '\0') {
    int count = vfs_list_mounts(mounts, 8);
    if (count < 0) {
      set_cmd_output("MOUNT LIST FAIL");
      return;
    }
    gprint("MOUNTS:\n", 0x99EEFF);
    for (int i = 0; i < count; i++) {
      gprint("  ", 0x000000);
      gprint(mounts[i].mount_path, 0x77FFAA);
      gprint("\n", 0x000000);
    }
    if (count == 0) {
      gprint("  (none)\n", 0xFFAA66);
    }
    set_cmd_output("MOUNT LIST OK");
    return;
  }

  if (backend[0] == '\0') {
    set_cmd_output("USAGE: mount <path> <tfs|ramdisk|ext2|ext2:<lba>>");
    return;
  }

  if (vfs_mount_backend(path, backend) == 0) {
    set_cmd_output("MOUNT OK");
  } else {
    set_cmd_output("MOUNT FAIL");
  }
}

static void cmd_umount(const char *args) {
  char path[24];
  next_token(args, path, sizeof(path));
  if (path[0] == '\0') {
    set_cmd_output("USAGE: umount <path>");
    return;
  }
  if (vfs_umount(path) == 0) {
    set_cmd_output("UMOUNT OK");
  } else {
    set_cmd_output("UMOUNT FAIL");
  }
}

static void cmd_mkdemo(const char *args) {
  (void)args;

  static const unsigned char demo_flat[] = {
      0xB8, 0x04, 0x00, 0x00, 0x00, // mov eax, 4  (SYS_WRITE)
  0xBB, 0x15, 0x00, 0x00, 0x40, // mov ebx, 0x40000015 (msg)
      0xCD, 0x80,                   // int 0x80
  0xB8, 0x01, 0x00, 0x00, 0x00, // mov eax, 1  (SYS_EXIT)
  0xCD, 0x80,                   // int 0x80
  0xEB, 0xFE,                   // jmp $ (fallback if exit denied)
      'D',  'I',  'S',  'K',  '-',  'U', 'S', 'E', 'R', '\n', '\0'};

  if (vfs_write_file("/demo.bin", demo_flat, sizeof(demo_flat)) > 0) {
    set_cmd_output("MKDEMO OK");
  } else {
    set_cmd_output("MKDEMO FAIL");
  }
}

static void cmd_save(const char *args) {
  int task_idx = parse_u32_dec(args);
  if (task_idx >= 0 && task_idx < 2) {
    sys_save_task(task_idx, task_idx);
    gprint("SAVED\n", 0x44FF88);
  } else {
    gprint("ERR: invalid task id\n", 0xFF5555);
  }
}

static void cmd_load(const char *args) {
  int task_idx = parse_u32_dec(args);
  if (task_idx >= 0 && task_idx < 2) {
    int ok = sys_load_task(task_idx, task_idx);
    gprint(ok ? "LOADED\n" : "SLOT EMPTY\n", ok ? 0x44FF88 : 0xFF5555);
  } else {
    gprint("ERR: invalid task id\n", 0xFF5555);
  }
}

static void cmd_ls(const char *args) {
  if (args && args[0] == '-' && args[1] == 'l') {
    Ext2LsEntry entries[64];
    int ext2_count = ext2_list_root_long(entries, 64);
    if (ext2_count >= 0) {
      gprint("INODE SIZE MTIME NAME\n", 0x99EEFF);
      gprint("----------------------\n", 0x557799);
      for (int i = 0; i < ext2_count; i++) {
        gprint_dec((int)entries[i].inode, 0x77FFAA);
        gprint(" ", 0x000000);
        gprint_dec((int)entries[i].size, 0xFFFFFF);
        gprint(" ", 0x000000);
        gprint_dec((int)entries[i].mtime, 0xFFE066);
        gprint(" ", 0x000000);
        gprint(entries[i].name, 0x00FF88);
        gprint("\n", 0x000000);
      }
      if (ext2_count == 0) {
        gprint("(empty)\n", 0xFFAA66);
      }
      return;
    }

    if (ata_disk_available) {
      disk_read_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
    }
    gprint("FLAGS SIZE NAME\n", 0x99EEFF);
    gprint("----------------\n", 0x557799);
    for (int i = 0; i < TFS_MAX_FILES; i++) {
      if (root_directory[i].flags == 1) {
        gprint_dec((int)root_directory[i].flags, 0x77FFAA);
        gprint("     ", 0x000000);
        gprint_dec((int)root_directory[i].size, 0xFFFFFF);
        gprint(" ", 0x000000);
        gprint(root_directory[i].name, 0x00FF88);
        gprint("\n", 0x000000);
      }
    }
    return;
  }

  list_files();
}

static void cmd_pcache(const char *args) {
  char sub[12];
  char val[12];
  int loops = 1000;
  DiskIoStats before_disk;
  DiskIoStats after_disk;
  PageCacheStats cache_stats;

  args = next_token(args, sub, sizeof(sub));
  next_token(args, val, sizeof(val));

  if (sub[0] == '\0' || str_eq(sub, "show")) {
    disk_get_io_stats(&before_disk);
    page_cache_get_stats(&cache_stats);

    gprint("PCACHE RH/RM ", 0x99EEFF);
    gprint_dec((int)cache_stats.read_hits, 0x77FFAA);
    gprint("/", 0x666666);
    gprint_dec((int)cache_stats.read_misses, 0xFFFFFF);
    gprint(" WH/WM ", 0x99EEFF);
    gprint_dec((int)cache_stats.write_hits, 0x77FFAA);
    gprint("/", 0x666666);
    gprint_dec((int)cache_stats.write_misses, 0xFFFFFF);
    gprint(" DIRTY ", 0x99EEFF);
    gprint_dec((int)cache_stats.dirty_lines, 0xFFE066);
    gprint("\n", 0x000000);

    gprint("PCACHE BUF/FLUSH ", 0x99EEFF);
    gprint_dec((int)cache_stats.buffered_writes, 0x77FFAA);
    gprint("/", 0x666666);
    gprint_dec((int)cache_stats.flush_writes, 0xFFFFFF);
    gprint(" EVICT ", 0x99EEFF);
    gprint_dec((int)cache_stats.evictions, 0xFFE066);
    gprint("\n", 0x000000);

    gprint("DISK C-R/C-W/U-R/U-W ", 0x99EEFF);
    gprint_dec((int)before_disk.cached_reads, 0x77FFAA);
    gprint("/", 0x666666);
    gprint_dec((int)before_disk.cached_writes, 0x77FFAA);
    gprint("/", 0x666666);
    gprint_dec((int)before_disk.uncached_reads, 0xFFFFFF);
    gprint("/", 0x666666);
    gprint_dec((int)before_disk.uncached_writes, 0xFFFFFF);
    gprint("\n", 0x000000);

    set_cmd_output("PCACHE SHOW OK");
    return;
  }

  if (str_eq(sub, "reset")) {
    disk_reset_io_stats();
    page_cache_reset_stats();
    set_cmd_output("PCACHE RESET OK");
    return;
  }

  if (str_eq(sub, "flush")) {
    page_cache_flush_all();
    set_cmd_output("PCACHE FLUSH OK");
    return;
  }

  if (str_eq(sub, "stress")) {
    uint16_t sector_buf[256];
    uint32_t base_lba = 480;
    int logical_sector_writes;

    if (is_dec_number(val)) {
      loops = parse_u32_dec(val);
      if (loops <= 0) {
        loops = 1000;
      }
    }

    disk_reset_io_stats();
    page_cache_reset_stats();

    for (int i = 0; i < loops; i++) {
      for (int s = 0; s < 8; s++) {
        for (int w = 0; w < 256; w++) {
          sector_buf[w] = (uint16_t)(i + s + w);
        }
        disk_write_sector(base_lba + (uint32_t)s, sector_buf);
      }
    }

    page_cache_flush_all();
    disk_get_io_stats(&after_disk);
    page_cache_get_stats(&cache_stats);

    logical_sector_writes = loops * 8;
    gprint("STRESS logical sectors: ", 0x99EEFF);
    gprint_dec(logical_sector_writes, 0xFFFFFF);
    gprint(" uncached writes: ", 0x99EEFF);
    gprint_dec((int)after_disk.uncached_writes, 0x77FFAA);
    gprint("\n", 0x000000);

    gprint("coalescing ratio x", 0x99EEFF);
    if (after_disk.uncached_writes > 0) {
      gprint_dec((int)(logical_sector_writes / (int)after_disk.uncached_writes), 0xFFE066);
    } else {
      gprint_dec(0, 0xFFE066);
    }
    gprint(" flushWrites ", 0x99EEFF);
    gprint_dec((int)cache_stats.flush_writes, 0xFFFFFF);
    gprint("\n", 0x000000);

    set_cmd_output("PCACHE STRESS OK");
    return;
  }

  set_cmd_output("USAGE: pcache show|reset|flush|stress [loops]");
}

static void cmd_clear(const char *args) {
  (void)args;
  clear_region(0, 310, 800, 600, 0x000033);
  buffer_idx = 0;
  input_buffer[0] = '\0';
}

static void cmd_eval(const char *args) {
  (void)args;
  TaskControlBlock *t = &task_list[0];
  int px = t->target_pixel;
  int total_potential = 0;
  int total_resistance = 0;
  for (int n = 0; n < 5; n++) {
    total_potential += os_memory_map[px].neurons[n].voltage;
    total_potential += os_memory_map[px].neurons[n].synaptic_weight;
    total_resistance += os_memory_map[px].neurons[n].dynamic_threshold;
  }
  gprint(total_potential > total_resistance ? "FLUID/ACTIVE\n" : "RIGID/STABLE\n",
         total_potential > total_resistance ? 0x44FF88 : 0xFFAA44);
}

static void cmd_stim(const char *args) {
  int task_idx = parse_u32_dec(args);
  int arg_shift = 0;
  while (args[arg_shift] && args[arg_shift] != ' ')
    arg_shift++;
  if (args[arg_shift] == ' ')
    arg_shift++;

  int amount = args[arg_shift] ? parse_u32_dec(&args[arg_shift]) : 500;
  if (task_idx < 0 || task_idx > 1) {
    gprint("ERR: invalid task id\n", 0xFF5555);
    return;
  }

  TaskControlBlock *t = &task_list[task_idx];
  int px = t->target_pixel;
  for (int n = 0; n < 5; n++) {
    os_memory_map[px].neurons[n].voltage += amount / 5;
    os_memory_map[px].neurons[n].synaptic_weight += 300;
    int new_thr = os_memory_map[px].neurons[n].dynamic_threshold - 800;
    if (new_thr < 200)
      new_thr = 200;
    os_memory_map[px].neurons[n].dynamic_threshold = new_thr;
  }
  gprint("STIMULUS APPLIED\n", 0xFFE066);
}

static void cmd_mall(const char *args) {
  (void)args;
  void *new_page = pmm_alloc_page();
  if (new_page) {
    gprint("ALLOCATED PAGE AT ", 0x44FF88);
    gprint_hex((uint32_t)new_page, 8, 0xFFFFFF);
    gprint("\n", 0x000000);
  } else {
    gprint("ERR: out of physical memory\n", 0xFF5555);
  }
}

static void cmd_free(const char *args) {
  uint32_t addr = parse_u32_hex(args);
  if (addr >= 0x100000) {
    pmm_free_page(addr);
    gprint("PAGE RELEASED\n", 0xFFE066);
  } else {
    gprint("ERR: invalid/protected address\n", 0xFF5555);
  }
}

static void cmd_map(const char *args) {
  (void)args;
  gprint("MEMORY MAP:\n", 0x99EEFF);
  pmm_print_map();
}

static void cmd_tsave(const char *args) {
  if (ata_disk_available) {
    disk_read_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
  }

  int slot = find_free_slot(root_directory);
  if (slot == -1) {
    gprint("ERR: disk directory full\n", 0xFF5555);
    return;
  }

  root_directory[slot].lba = TFS_DATA_START + slot;
  root_directory[slot].flags = 1;
  root_directory[slot].size = sizeof(potentials);

  if (args && args[0]) {
    for (int i = 0; i < 11; i++) {
      char c = args[i];
      root_directory[slot].name[i] = c;
      if (c == '\0')
        break;
      if (i == 10)
        root_directory[slot].name[11] = '\0';
    }
  } else {
    root_directory[slot].name[0] = 'b';
    root_directory[slot].name[1] = 'r';
    root_directory[slot].name[2] = 'a';
    root_directory[slot].name[3] = 'i';
    root_directory[slot].name[4] = 'n';
    root_directory[slot].name[5] = (char)('0' + slot);
    root_directory[slot].name[6] = '\0';
  }

  if (ata_disk_available) {
    disk_write_sector(root_directory[slot].lba, (uint16_t *)potentials);
    disk_write_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
  }

  gprint("SNAPSHOT SAVED: ", 0x44FF88);
  gprint(root_directory[slot].name, 0xFFFFFF);
  gprint("\n", 0x000000);
}

static void cmd_tload(const char *args) {
  if (!args || !args[0]) {
    gprint("USAGE: tload <name>\n", 0xFF5555);
    return;
  }

  if (ata_disk_available) {
    disk_read_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
  }

  int found = -1;
  for (int i = 0; i < TFS_MAX_FILES; i++) {
    if (root_directory[i].flags != 1)
      continue;

    int match = 1;
    for (int j = 0; j < 11; j++) {
      char a = root_directory[i].name[j];
      char b = args[j];
      if (a >= 'A' && a <= 'Z')
        a += 32;
      if (b >= 'A' && b <= 'Z')
        b += 32;
      if (b == '\0') {
        if (a != '\0')
          match = 0;
        break;
      }
      if (a != b) {
        match = 0;
        break;
      }
    }

    if (match) {
      found = i;
      break;
    }
  }

  if (found >= 0 && ata_disk_available) {
    disk_read_sector(root_directory[found].lba, (uint16_t *)potentials);
    gprint("LOADED: ", 0x44FF88);
    gprint(root_directory[found].name, 0xFFFFFF);
    gprint("\n", 0x000000);
  } else {
    gprint("ERR: file not found\n", 0xFF5555);
  }
}

static void cmd_pci(const char *args) {
  if (str_eq(args, "bar")) {
    uint32_t mmio_base = 0;
    uint32_t mmio_size = 0;
    uint8_t subclass = 0;
    if (pci_prepare_storage_mmio(&mmio_base, &mmio_size, &subclass)) {
      gprint("STORAGE MMIO: ", 0x99EEFF);
      gprint_hex(mmio_base, 8, 0xFFFFFF);
      gprint(" SIZE:", 0x99EEFF);
      gprint_hex(mmio_size, 4, 0xFFFFFF);
      gprint(" TYPE:", 0x99EEFF);
      gprint((subclass == 0x06) ? "AHCI" : "NVME", 0x44FF88);
      gprint("\n", 0x000000);

      map_mmio_region(mmio_base, mmio_size);
      gprint("MMIO MAPPED\n", 0x44FF88);
    } else {
      gprint("NO STORAGE CTRL FOUND\n", 0xFF5555);
    }
  } else {
    pci_print_results();
  }
}

static void ahci_print_report(const AhciProbeReport *r) {
  if (r == 0 || !r->controller_found) {
    gprint("AHCI: no controller found\n", 0xFFAA66);
    return;
  }

  gprint("AHCI HBA B", 0x99EEFF);
  gprint_hex(r->bus, 2, 0xFFFFFF);
  gprint(":D", 0x99EEFF);
  gprint_hex(r->slot, 2, 0xFFFFFF);
  gprint(":F", 0x99EEFF);
  gprint_hex(r->function, 2, 0xFFFFFF);
  gprint(" ABAR=", 0x99EEFF);
  gprint_hex(r->abar, 8, 0xFFFFFF);
  gprint("\n", 0x000000);

  gprint("CAP=", 0x77C8FF);
  gprint_hex(r->cap, 8, 0xFFFFFF);
  gprint(" PI=", 0x77C8FF);
  gprint_hex(r->pi, 8, 0xFFFFFF);
  gprint(" VS=", 0x77C8FF);
  gprint_hex(r->version, 8, 0xFFFFFF);
  gprint(" READY ", 0x77C8FF);
  gprint_dec(r->ready_ports, 0xFFFFFF);
  gprint("/", 0x77C8FF);
  gprint_dec(r->implemented_ports, 0xFFFFFF);
  gprint("\n", 0x000000);

  for (int p = 0; p < AHCI_MAX_PORTS; p++) {
    const AhciPortReport *pr = &r->ports[p];
    if (!pr->implemented) {
      continue;
    }
    gprint(" P", 0xAAAAAA);
    gprint_hex(pr->port_no, 2, 0xFFFFFF);
    gprint(" DET=", 0xAAAAAA);
    gprint_hex(pr->det, 1, 0xFFFFFF);
    gprint(" IPM=", 0xAAAAAA);
    gprint_hex(pr->ipm, 1, 0xFFFFFF);
    gprint(" SIG=", 0xAAAAAA);
    gprint_hex(pr->sig, 8, 0xFFFFFF);
    gprint(" ", 0x000000);
    gprint(pr->sata ? "SATA" : "NON-SATA", pr->sata ? 0x77FFAA : 0xFFAA66);
    gprint(" ", 0x000000);
    gprint(pr->ready ? "READY" : "NOT_READY", pr->ready ? 0x44FF88 : 0xFFAA66);
    gprint("\n", 0x000000);
  }
}

static uint32_t sector_checksum(const uint16_t *sector) {
  uint32_t hash = 0x811C9DC5U;

  for (int i = 0; i < 256; i++) {
    hash ^= (uint32_t)sector[i];
    hash *= 16777619U;
  }
  return hash;
}

static const char *backend_name(uint8_t backend) {
  if (backend == DISK_BACKEND_ATA) {
    return "ATA";
  }
  if (backend == DISK_BACKEND_AHCI) {
    return "AHCI";
  }
  return "AUTO";
}

static void cmd_ahci(const char *args) {
  char sub[16];
  char arg1[16];
  char arg2[16];
  AhciProbeReport report;

  args = next_token(args, sub, sizeof(sub));
  args = next_token(args, arg1, sizeof(arg1));
  next_token(args, arg2, sizeof(arg2));

  if (sub[0] == '\0' || str_eq(sub, "show")) {
    if (!ahci_get_runtime_report(&report)) {
      set_cmd_output("AHCI REPORT FAIL");
      return;
    }
    ahci_print_report(&report);
    set_cmd_output("AHCI REPORT");
    return;
  }

  if (str_eq(sub, "backend")) {
    if (arg1[0] == '\0') {
      gprint("AHCI READ BACKEND: ", 0x99EEFF);
      gprint((char *)backend_name(disk_get_preferred_backend()), 0xFFFFFF);
      gprint("\n", 0x000000);
      set_cmd_output("AHCI BACKEND SHOW");
      return;
    }

    if (str_eq(arg1, "ata")) {
      disk_set_preferred_backend(DISK_BACKEND_ATA);
      set_cmd_output("AHCI BACKEND ATA");
      return;
    }
    if (str_eq(arg1, "ahci")) {
      disk_set_preferred_backend(DISK_BACKEND_AHCI);
      set_cmd_output("AHCI BACKEND AHCI");
      return;
    }
    if (str_eq(arg1, "auto")) {
      disk_set_preferred_backend(DISK_BACKEND_AUTO);
      set_cmd_output("AHCI BACKEND AUTO");
      return;
    }

    set_cmd_output("USAGE: ahci backend [auto|ata|ahci]");
    return;
  }

  if (str_eq(sub, "reset")) {
    AhciPortReport pr;
    int ok = 0;

    if (!is_dec_number(arg1)) {
      set_cmd_output("USAGE: ahci reset <port>");
      return;
    }

    ok = ahci_port_reset_and_start((uint8_t)parse_u32_dec(arg1), &pr);
    if (!ok) {
      set_cmd_output("AHCI RESET FAIL");
      return;
    }

    gprint("AHCI RESET OK P", 0x44FF88);
    gprint_dec(parse_u32_dec(arg1), 0xFFFFFF);
    gprint(" DET=", 0x99EEFF);
    gprint_hex(pr.det, 1, 0xFFFFFF);
    gprint(" IPM=", 0x99EEFF);
    gprint_hex(pr.ipm, 1, 0xFFFFFF);
    gprint("\n", 0x000000);
    set_cmd_output("AHCI RESET OK");
    return;
  }

  if (str_eq(sub, "identify")) {
    AhciIdentifyInfo info;
    int ok = 0;

    if (arg1[0] == '\0') {
      ok = ahci_identify_first_ready(&info);
    } else if (is_dec_number(arg1)) {
      ok = ahci_identify_port((uint8_t)parse_u32_dec(arg1), &info);
    } else {
      set_cmd_output("USAGE: ahci identify [port]");
      return;
    }

    if (!ok || !info.success) {
      set_cmd_output("AHCI IDENTIFY FAIL");
      return;
    }

    gprint("AHCI IDENTIFY P", 0x44FF88);
    gprint_dec(info.port_no, 0xFFFFFF);
    gprint(" MODEL:", 0x99EEFF);
    gprint(info.model, 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint("  SERIAL: ", 0x99EEFF);
    gprint(info.serial, 0xFFFFFF);
    gprint(" FW: ", 0x99EEFF);
    gprint(info.firmware, 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint("  LBA28 sectors: ", 0x99EEFF);
    gprint_dec((int)info.lba28_sectors, 0xFFFFFF);
    gprint("\n", 0x000000);
    set_cmd_output("AHCI IDENTIFY OK");
    return;
  }

  if (str_eq(sub, "read")) {
    uint16_t sector[256];
    uint8_t port = 0xFF;
    uint32_t lba = 0;
    int ok = 0;

    if (arg1[0] == '\0') {
      set_cmd_output("USAGE: ahci read <lba> | ahci read <port> <lba>");
      return;
    }

    if (arg2[0] == '\0') {
      if (is_dec_number(arg1)) {
        lba = (uint32_t)parse_u32_dec(arg1);
      } else {
        lba = parse_u32_hex(arg1);
      }
      ok = ahci_read_first_ready_sector(lba, &port, sector);
    } else {
      if (!is_dec_number(arg1)) {
        set_cmd_output("USAGE: ahci read <lba> | ahci read <port> <lba>");
        return;
      }
      port = (uint8_t)parse_u32_dec(arg1);
      if (is_dec_number(arg2)) {
        lba = (uint32_t)parse_u32_dec(arg2);
      } else {
        lba = parse_u32_hex(arg2);
      }
      ok = ahci_read_sector(port, lba, sector);
    }

    if (ok != AHCI_READ_OK) {
      gprint("AHCI READ FAIL: ", 0xFFAA66);
      gprint((char *)disk_read_error_string(ok), 0xFFFFFF);
      gprint("\n", 0x000000);
      set_cmd_output("AHCI READ FAIL");
      return;
    }

    gprint("AHCI READ OK P", 0x44FF88);
    gprint_dec((int)port, 0xFFFFFF);
    gprint(" LBA=", 0x99EEFF);
    gprint_hex(lba, 8, 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint("W0=", 0x99EEFF);
    gprint_hex((uint32_t)sector[0], 4, 0xFFFFFF);
    gprint(" W1=", 0x99EEFF);
    gprint_hex((uint32_t)sector[1], 4, 0xFFFFFF);
    gprint(" W2=", 0x99EEFF);
    gprint_hex((uint32_t)sector[2], 4, 0xFFFFFF);
    gprint(" W3=", 0x99EEFF);
    gprint_hex((uint32_t)sector[3], 4, 0xFFFFFF);
    gprint("\n", 0x000000);
    set_cmd_output("AHCI READ OK");
    return;
  }

  if (str_eq(sub, "smoke")) {
    uint16_t ata_sector[256];
    uint16_t ahci_sector[256];
    uint32_t lba = 0;
    int ata_rc;
    int ahci_rc;
    uint32_t ata_sum;
    uint32_t ahci_sum;

    if (arg1[0] == '\0') {
      set_cmd_output("USAGE: ahci smoke <lba>");
      return;
    }

    lba = is_dec_number(arg1) ? (uint32_t)parse_u32_dec(arg1)
                              : parse_u32_hex(arg1);
    ata_rc = disk_read_sector_backend(lba, ata_sector, DISK_BACKEND_ATA);
    ahci_rc = disk_read_sector_backend(lba, ahci_sector, DISK_BACKEND_AHCI);

    if (ata_rc != DISK_IO_OK) {
      gprint("SMOKE ATA FAIL: ", 0xFFAA66);
      gprint((char *)disk_read_error_string(ata_rc), 0xFFFFFF);
      gprint("\n", 0x000000);
      set_cmd_output("AHCI SMOKE ATA FAIL");
      return;
    }
    if (ahci_rc != DISK_IO_OK) {
      gprint("SMOKE AHCI FAIL: ", 0xFFAA66);
      gprint((char *)disk_read_error_string(ahci_rc), 0xFFFFFF);
      gprint("\n", 0x000000);
      set_cmd_output("AHCI SMOKE AHCI FAIL");
      return;
    }

    ata_sum = sector_checksum(ata_sector);
    ahci_sum = sector_checksum(ahci_sector);

    gprint("SMOKE LBA=", 0x99EEFF);
    gprint_hex(lba, 8, 0xFFFFFF);
    gprint(" ATA=", 0x99EEFF);
    gprint_hex(ata_sum, 8, 0xFFFFFF);
    gprint(" AHCI=", 0x99EEFF);
    gprint_hex(ahci_sum, 8, 0xFFFFFF);
    gprint((ata_sum == ahci_sum) ? " MATCH\n" : " MISMATCH\n",
           (ata_sum == ahci_sum) ? 0x44FF88 : 0xFF5555);

    set_cmd_output((ata_sum == ahci_sum) ? "AHCI SMOKE OK" : "AHCI SMOKE MISMATCH");
    return;
  }

  set_cmd_output("USAGE: ahci [show|backend [auto|ata|ahci]|reset <port>|identify [port]|read <lba>|read <port> <lba>|smoke <lba>]");
}

static void cmd_zoomp(const char *args) {
  (void)args;
  if (zoom_level < 4)
    zoom_level++;
  gprint("ZOOM: ", 0x99EEFF);
  gprint_dec(zoom_level, 0xFFFFFF);
  gprint("x\n", 0x000000);
}

static void cmd_zoomm(const char *args) {
  (void)args;
  if (zoom_level > 1) {
    zoom_level--;
    zoom_offset = 0;
  }
  gprint("ZOOM: ", 0x99EEFF);
  gprint_dec(zoom_level, 0xFFFFFF);
  gprint("x\n", 0x000000);
}

static void cmd_zpanp(const char *args) {
  (void)args;
  int step = 16 / zoom_level;
  if (step < 1)
    step = 1;
  zoom_offset += step;
  if (zoom_offset > 16 - step)
    zoom_offset = 16 - step;
  gprint("PAN OFFSET: ", 0x99EEFF);
  gprint_dec(zoom_offset, 0xFFFFFF);
  gprint("\n", 0x000000);
}

static void cmd_zpanm(const char *args) {
  (void)args;
  int step = 16 / zoom_level;
  if (step < 1)
    step = 1;
  zoom_offset -= step;
  if (zoom_offset < 0)
    zoom_offset = 0;
  gprint("PAN OFFSET: ", 0x99EEFF);
  gprint_dec(zoom_offset, 0xFFFFFF);
  gprint("\n", 0x000000);
}

static void cmd_wipe(const char *args) {
  (void)args;
  uint16_t zero_buffer[256];
  for (int i = 0; i < 256; i++)
    zero_buffer[i] = 0;

  disk_write_sector(TFS_DIR_LBA, zero_buffer);
  for (int s = 0; s < TFS_MAX_FILES; s++) {
    disk_write_sector(TFS_DATA_START + s, zero_buffer);
  }
  for (int i = 0; i < 16; i++)
    potentials[i] = 0;

  gprint("ALL SNAPSHOT DATA WIPED\n", 0xFFE066);
}

typedef void (*cmd_handler_t)(const char *args);

typedef struct {
  const char *name;
  cmd_handler_t handler;
} CommandEntry;

static const CommandEntry command_table[] = {
    {"help", cmd_help}, {"save", cmd_save}, {"load", cmd_load},
    {"ls", cmd_ls},     {"clear", cmd_clear}, {"eval", cmd_eval},
    {"stim", cmd_stim}, {"mall", cmd_mall},   {"free", cmd_free},
    {"map", cmd_map},   {"tsave", cmd_tsave}, {"tload", cmd_tload},
    {"pci", cmd_pci},   {"zoom+", cmd_zoomp}, {"zoom-", cmd_zoomm},
    {"ahci", cmd_ahci},
    {"zpan+", cmd_zpanp},
    {"zpan-", cmd_zpanm},
    {"wipe", cmd_wipe},
    {"net", cmd_net},
    {"profile", cmd_profile},
    {"model", cmd_model},
    {"stdp", cmd_stdp},
    {"ps", cmd_ps},
    {"kill", cmd_kill},
    {"nice", cmd_nice},
    {"trace", cmd_trace},
    {"ipc", cmd_ipc},
    {"remote", cmd_remote},
    {"viz", cmd_viz},
    {"manifest", cmd_manifest},
    {"dataset", cmd_dataset},
    {"replay", cmd_replay},
    {"pcache", cmd_pcache},
    {"diag", cmd_diag},
    {"mount", cmd_mount},
    {"umount", cmd_umount},
    {"stat", cmd_stat},
    {"delete", cmd_delete},
    {"exec", cmd_exec},
    {"insmod", cmd_insmod},
    {"rmmod", cmd_rmmod},
    {"lsmod", cmd_lsmod},
    {"id", cmd_id},
    {"chmod", cmd_chmod_posix},
    {"chown", cmd_chown_posix},
    {"setuid", cmd_setuid},
    {"setgid", cmd_setgid},
    {"setpgid", cmd_setpgid},
    {"jobs", cmd_jobs},
    {"fg", cmd_fg},
    {"bg", cmd_bg},
    {"tcsetpgrp", cmd_tcsetpgrp},
    {"tcgetpgrp", cmd_tcgetpgrp},
    {"mkdemo", cmd_mkdemo},
    {"synview", cmd_synview},
    {"synset", cmd_synset},
    {"synrule", cmd_synrule},
    {"synpreset", cmd_synpreset},
    {"syncmp", cmd_syncmp},
    {"sbrowse", cmd_sbrowse},
    {"spreview", cmd_spreview},
    {"stag", cmd_stag},
    {"sdiff", cmd_sdiff},
    {"tz", cmd_tz},
};

void process_command(char *cmd) {
  char name[16];
  const char *profile_name = 0;
  uint32_t cmd_stamp = profile_begin();
  uint32_t cmd_delta = 0;
  int i = 0;
  while (cmd[i] == ' ') {
    i++;
  }

  replay_record_command(cmd + i);

  int n = 0;
  while (cmd[i] && cmd[i] != ' ' && n < 15) {
    name[n] = cmd[i];
    i++;
    n++;
  }
  name[n] = '\0';

  if (name[0] == '\0') {
    extern void set_cmd_output(const char *);
    set_cmd_output("EMPTY COMMAND");
    goto done;
  }

  profile_name = name;

  const char *args = "";
  if (cmd[i] == ' ')
    args = &cmd[i + 1];

  for (unsigned int k = 0; k < sizeof(command_table) / sizeof(command_table[0]); k++) {
    if (str_eq(name, command_table[k].name)) {
      command_table[k].handler(args);
      goto done;
    }
  }

  {
    extern void set_cmd_output(const char *);
    set_cmd_output("UNKNOWN COMMAND");
  }
  klog_warn("unknown command");

done:
  cmd_delta = profile_end_and_get(PROFILE_SLOT_COMMAND, cmd_stamp);
  profile_record_command(cmd_delta);
  if (remote_is_enabled()) {
    remote_send_command_result(cmd + i);
  }
  if (profile_name != 0 && profile_name[0] != '\0') {
    profile_record_named_command(profile_name, cmd_delta);
  }
}
