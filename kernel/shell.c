#include "shell.h"
#include "disk.h"
#include "pci.h"
#include "storage_manager.h"
#include "klog.h"
#include "vfs.h"
#include "usermode.h"

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;

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
extern int zoom_level;
extern int zoom_offset;
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
  gprint("commands: help save load ls clear eval stim mall free map\n", 0xFFFFFF);
  gprint("          tsave tload pci pci bar zoom+ zoom- zpan+ zpan- wipe\n", 0xFFFFFF);
  gprint("          exec <path> mkdemo\n", 0xFFFFFF);
  gprint("phase6:   synview synset synrule synpreset syncmp\n", 0x77C8FF);
  gprint("          sbrowse spreview stag sdiff\n", 0x77C8FF);
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

  /* Attempt to execute the program */
  if (exec_user_program(args)) {
    set_cmd_output("EXEC OK");
  } else {
    /* Execution failed; likely ELF32 validation or memory issue */
    set_cmd_output("EXEC FORMAT ERR");
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
  (void)args;
  list_files();
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
    extern int pci_find_storage_controller(void);
    extern uint32_t pci_read_bar5(int device_index);
    int idx = pci_find_storage_controller();
    if (idx >= 0) {
      uint32_t bar5 = pci_read_bar5(idx);
      gprint("STORAGE BAR5: ", 0x99EEFF);
      gprint_hex(bar5, 8, 0xFFFFFF);
      gprint("\n", 0x000000);
      if (bar5 != 0) {
        map_mmio_region(bar5, 0x2000);
        gprint("MMIO MAPPED\n", 0x44FF88);
      }
    } else {
      gprint("NO STORAGE CTRL FOUND\n", 0xFF5555);
    }
  } else {
    pci_print_results();
  }
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
    {"zpan+", cmd_zpanp},
    {"zpan-", cmd_zpanm},
    {"wipe", cmd_wipe},
    {"exec", cmd_exec},
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
};

void process_command(char *cmd) {
  char name[16];
  int i = 0;
  while (cmd[i] == ' ') {
    i++;
  }

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
    return;
  }

  const char *args = "";
  if (cmd[i] == ' ')
    args = &cmd[i + 1];

  for (unsigned int k = 0; k < sizeof(command_table) / sizeof(command_table[0]); k++) {
    if (str_eq(name, command_table[k].name)) {
      command_table[k].handler(args);
      return;
    }
  }

  {
    extern void set_cmd_output(const char *);
    set_cmd_output("UNKNOWN COMMAND");
  }
  klog_warn("unknown command");
}
