#include "dashboard.h"
#include "disk.h"
#include "pci.h"
#include "scheduler.h"
#include "storage_manager.h"
#include "net.h"
#include "profiling.h"
#include "model_manager.h"

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
  uint8_t current_phase;
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

extern uint32_t tick;
extern int zoom_level;
extern int zoom_offset;
extern int potentials[16];
extern volatile int buffer_idx;
extern volatile char input_buffer[32];
extern volatile int shell_dirty;
extern int cursor_x;
extern int cursor_y;
extern int shell_cursor_x;
extern int shell_cursor_y;
extern TaskControlBlock task_list[2];
extern NeuralPixel os_memory_map[2];
extern int pmm_get_free_pages(void);
extern FileEntry root_directory[25];
extern volatile int ata_disk_available;

extern void pulse_neurons(void);
extern void draw_cursor(uint32_t tick);
extern void draw_mouse_cursor(void);
extern void disk_read_sector(uint32_t lba, uint16_t *buffer);
extern void disk_write_sector(uint32_t lba, uint16_t *buffer);
extern int find_free_slot(FileEntry *dir);

extern void gprint(char *str, uint32_t color);
extern void gprint_dec(int val, uint32_t color);
extern void clear_region(int x0, int y0, int x1, int y1, uint32_t color);
extern void draw_hline(int y, int x0, int x1, uint32_t color);
extern void put_pixel(int x, int y, uint32_t color);
extern void flip_buffer(void);
extern volatile int flip_mutex;

static int viz_mode = VIZ_MODE_OFF;
static int viz_scrub = 0;
static int viz_compare_a = 0;
static int viz_compare_b = 1;
static int viz_autoplay = 0;
static int raster_head = 0;
static uint8_t raster_ring[64][16];

static uint8_t prev_phase[2] = {0, 0};

void viz_set_mode(int mode) {
  if (mode < VIZ_MODE_OFF || mode > VIZ_MODE_COMPARE) {
    return;
  }
  viz_mode = mode;
}

int viz_get_mode(void) { return viz_mode; }

void viz_set_scrub(int index) {
  if (index < 0) {
    index = 0;
  }
  if (index > 63) {
    index = 63;
  }
  viz_scrub = index;
}

int viz_get_scrub(void) { return viz_scrub; }

int viz_set_compare_slots(int slot_a, int slot_b) {
  if (slot_a < 0 || slot_b < 0 || slot_a >= storage_snapshot_capacity() ||
      slot_b >= storage_snapshot_capacity()) {
    return 0;
  }
  viz_compare_a = slot_a;
  viz_compare_b = slot_b;
  return 1;
}

void viz_set_autoplay(int enabled) { viz_autoplay = enabled ? 1 : 0; }

int viz_get_autoplay(void) { return viz_autoplay; }

void viz_step_scrub(int delta) {
  int next = viz_scrub + delta;
  while (next < 0) {
    next += 64;
  }
  while (next > 63) {
    next -= 64;
  }
  viz_scrub = next;
}

static int heat_color(int v, int lo, int hi) {
  if (hi <= lo) {
    return 0x335577;
  }
  int span = hi - lo;
  int x = (v - lo) * 255 / span;
  if (x < 0) {
    x = 0;
  }
  if (x > 255) {
    x = 255;
  }

  int r = x;
  int g = 255 - ((x - 128) < 0 ? (128 - x) : (x - 128));
  int b = 255 - x;
  if (g < 0) {
    g = 0;
  }
  return ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
}

static void draw_viz_heatmap(void) {
  const int x0 = 184;
  const int y0 = 132;
  const int cell_w = 24;
  const int cell_h = 68;
  int min_w = 32767;
  int max_w = -32768;

  for (int p = 0; p < 2; p++) {
    for (int n = 0; n < 5; n++) {
      int w = os_memory_map[p].neurons[n].synaptic_weight;
      if (w < min_w)
        min_w = w;
      if (w > max_w)
        max_w = w;
    }
  }

  clear_region(x0 - 6, y0 - 20, x0 + (10 * cell_w) + 6, y0 + cell_h + 30,
               0x001933);
  cursor_x = x0;
  cursor_y = y0 - 16;
  gprint("VIZ HEATMAP (weights)", 0x9AF0C8);

  for (int p = 0; p < 2; p++) {
    for (int n = 0; n < 5; n++) {
      int idx = (p * 5) + n;
      int cx = x0 + idx * cell_w;
      int cy = y0;
      int color = heat_color(os_memory_map[p].neurons[n].synaptic_weight, min_w, max_w);
      clear_region(cx, cy, cx + cell_w - 2, cy + cell_h, color);
      if (n == 0) {
        cursor_x = cx;
        cursor_y = y0 + cell_h + 2;
        gprint((p == 0) ? "T0" : "T1", 0xAACCEE);
      }
    }
  }
}

static void record_raster_sample(void) {
  int row = raster_head & 63;
  for (int i = 0; i < 16; i++) {
    int p = i / 8;
    int n = i % 5;
    uint8_t spike = 0;
    if (n < 5 && p < 2) {
      spike = (os_memory_map[p].neurons[n].voltage >=
               os_memory_map[p].neurons[n].dynamic_threshold)
                  ? 1
                  : 0;
    }
    raster_ring[row][i] = spike;
  }
  raster_head = (raster_head + 1) & 63;
}

static void draw_viz_raster(void) {
  const int x0 = 184;
  const int y0 = 120;
  const int w = 400;
  const int h = 170;
  int scrub = viz_scrub;
  if (scrub < 0)
    scrub = 0;
  if (scrub > 63)
    scrub = 63;

  clear_region(x0 - 6, y0 - 16, x0 + w + 8, y0 + h + 26, 0x001933);
  cursor_x = x0;
  cursor_y = y0 - 14;
  gprint("VIZ RASTER", 0xB6E8FF);
  gprint(" scrub:", 0xAACCEE);
  gprint_dec(scrub, 0xFFFFFF);

  for (int t = 0; t < 64; t++) {
    int ring_idx = (raster_head + t) & 63;
    int x = x0 + (t * w) / 64;
    for (int n = 0; n < 10; n++) {
      int y = y0 + (n * h) / 10;
      uint32_t c = raster_ring[ring_idx][n] ? 0x66FF66 : 0x22384A;
      put_pixel(x, y, c);
      if (x + 1 < x0 + w)
        put_pixel(x + 1, y, c);
    }
  }

  {
    int sx = x0 + (scrub * w) / 64;
    for (int y = y0; y < y0 + h; y++) {
      put_pixel(sx, y, 0xFFE066);
    }
  }
}

static void draw_viz_compare(void) {
  int dv = 0, dw = 0, dt = 0;
  int dvv[5], dww[5], dtt[5];
  clear_region(180, 124, 620, 296, 0x001933);
  cursor_x = 188;
  cursor_y = 128;
  gprint("VIZ COMPARE", 0xFFD37A);
  gprint(" slots ", 0xAACCEE);
  gprint_dec(viz_compare_a, 0xFFFFFF);
  gprint(" vs ", 0xAACCEE);
  gprint_dec(viz_compare_b, 0xFFFFFF);

  cursor_x = 188;
  cursor_y = 148;
  if (!storage_diff_snapshots(viz_compare_a, viz_compare_b, &dv, &dw, &dt)) {
    gprint("No snapshot data for selected slots", 0xFF7777);
    return;
  }

  if (!storage_diff_snapshots_vector(viz_compare_a, viz_compare_b, dvv, dww, dtt, 5)) {
    gprint("Vector diff unavailable", 0xFF7777);
    return;
  }

  gprint("drift V:", 0xAACCEE);
  gprint_dec(dv, 0xFFFFFF);
  gprint("  W:", 0xAACCEE);
  gprint_dec(dw, 0xFFFFFF);
  gprint("  T:", 0xAACCEE);
  gprint_dec(dt, 0xFFFFFF);

  cursor_x = 188;
  cursor_y = 166;
  gprint("divergence markers: ", 0xAACCEE);
  gprint((dv > 2000 || dv < -2000) ? "V! " : "v ", (dv > 2000 || dv < -2000) ? 0xFF7777 : 0x77FF77);
  gprint((dw > 600 || dw < -600) ? "W! " : "w ", (dw > 600 || dw < -600) ? 0xFF7777 : 0x77FF77);
  gprint((dt > 1200 || dt < -1200) ? "T!" : "t", (dt > 1200 || dt < -1200) ? 0xFF7777 : 0x77FF77);

  cursor_x = 188;
  cursor_y = 184;
  gprint("param diff summary", 0xAACCEE);
  cursor_x = 188;
  cursor_y = 198;
  gprint("task0 model:", 0x88E0FF);
  gprint((char *)model_name(0), 0xFFFFFF);
  cursor_x = 188;
  cursor_y = 212;
  gprint("task1 model:", 0x88E0FF);
  gprint((char *)model_name(1), 0xFFFFFF);

  cursor_x = 188;
  cursor_y = 232;
  gprint("N dV dW dT", 0x9AF0C8);
  for (int i = 0; i < 5; i++) {
    cursor_x = 188;
    cursor_y = 246 + (i * 10);
    gprint_dec(i, 0xFFFFFF);
    gprint(" ", 0x666666);
    gprint_dec(dvv[i], 0xFFFFFF);
    gprint(" ", 0x666666);
    gprint_dec(dww[i], 0xFFFFFF);
    gprint(" ", 0x666666);
    gprint_dec(dtt[i], 0xFFFFFF);
  }
}

static void draw_phase13_viz(void) {
  if (viz_mode == VIZ_MODE_HEATMAP) {
    draw_viz_heatmap();
  } else if (viz_mode == VIZ_MODE_RASTER) {
    draw_viz_raster();
  } else if (viz_mode == VIZ_MODE_COMPARE) {
    draw_viz_compare();
  }
}

static uint32_t metric_avg_cycles(const ProfileMetric *m) {
  if (m == 0 || m->count == 0) {
    return 0;
  }
  return m->total_cycles / m->count;
}

static uint32_t metric_roll_cycles(uint32_t slot) {
  return profile_metric_rolling_avg(slot);
}

static char task_state_code(uint32_t state) {
  if (state == TASK_STATE_RUNNING) {
    return 'R';
  }
  if (state == TASK_STATE_READY) {
    return 'Y';
  }
  if (state == TASK_STATE_BLOCKED) {
    return 'B';
  }
  if (state == TASK_STATE_SLEEPING) {
    return 'S';
  }
  if (state == TASK_STATE_TERMINATED) {
    return 'T';
  }
  return '?';
}

static void draw_hud_telemetry(void) {
  const int y0 = 110;
  const int y1 = 326;
  const int left_x0 = 0;
  const int left_x1 = 170;
  const int right_x0 = 630;
  const int right_x1 = 800;
  const int total_user_pages = (0x2000000 - 0x100000) / 4096;
  int free_pages = pmm_get_free_pages();
  int used_pages = total_user_pages - free_pages;
  int pressure_pct = 0;
  int running = 0;
  int ready = 0;
  int blocked = 0;
  int sleeping = 0;
  int safe_task_count = os_task_count;
  int safe_current_task = os_current_task;

  if (safe_task_count < 0) {
    safe_task_count = 0;
  }
  if (safe_task_count > MAX_TASKS) {
    safe_task_count = MAX_TASKS;
  }
  if (safe_current_task < 0 || safe_current_task >= safe_task_count) {
    safe_current_task = 0;
  }

  if (used_pages < 0) {
    used_pages = 0;
  }
  if (total_user_pages > 0) {
    pressure_pct = (used_pages * 100) / total_user_pages;
  }

  clear_region(left_x0, y0, left_x1, y1, 0x00142A);
  clear_region(right_x0, y0, right_x1, y1, 0x00142A);
  draw_hline(y0, left_x0, left_x1, 0x224466);
  draw_hline(y1, left_x0, left_x1, 0x224466);
  draw_hline(y0, right_x0, right_x1, 0x224466);
  draw_hline(y1, right_x0, right_x1, 0x224466);

  cursor_x = 8;
  cursor_y = 116;
  gprint("SPIKE HUD", 0x66E0FF);
  cursor_x = 8;
  cursor_y = 132;
  gprint("P0 SPS:", 0xAACCEE);
  gprint_dec(task_list[0].spikes_per_second, 0x44FF88);
  cursor_x = 8;
  cursor_y = 146;
  gprint("P1 SPS:", 0xAACCEE);
  gprint_dec(task_list[1].spikes_per_second, 0x44FF88);

  cursor_x = 8;
  cursor_y = 164;
  gprint("RECENT:", 0xAACCEE);
  gprint_dec(os_memory_map[0].pixel_recent_spikes, 0xFFE066);
  gprint("/", 0x777777);
  gprint_dec(os_memory_map[1].pixel_recent_spikes, 0xFFE066);

  cursor_x = 8;
  cursor_y = 182;
  gprint("PHASE0:", 0xAACCEE);
  gprint((os_memory_map[0].current_phase == 1) ? "RIGID" : "FLUID",
         (os_memory_map[0].current_phase == 1) ? 0xFF7777 : 0x77FF77);
  cursor_x = 8;
  cursor_y = 196;
  gprint("PHASE1:", 0xAACCEE);
  gprint((os_memory_map[1].current_phase == 1) ? "RIGID" : "FLUID",
         (os_memory_map[1].current_phase == 1) ? 0xFF7777 : 0x77FF77);

  for (int i = 0; i < safe_task_count; i++) {
    if (os_tasks[i].state == TASK_STATE_RUNNING) {
      running++;
    } else if (os_tasks[i].state == TASK_STATE_READY) {
      ready++;
    } else if (os_tasks[i].state == TASK_STATE_BLOCKED) {
      blocked++;
    } else if (os_tasks[i].state == TASK_STATE_SLEEPING) {
      sleeping++;
    }
  }

  cursor_x = 638;
  cursor_y = 116;
  gprint("SYSTEM HUD", 0x66E0FF);

  cursor_x = 638;
  cursor_y = 132;
  gprint("TASK:", 0xAACCEE);
  gprint_dec(safe_current_task, 0xFFFFFF);
  gprint(" S:", 0xAACCEE);
  char s[2] = {task_state_code(os_tasks[safe_current_task].state), '\0'};
  gprint(s, 0xFFE066);

  cursor_x = 638;
  cursor_y = 146;
  gprint("RUN:", 0xAACCEE);
  gprint_dec(running, 0x77FF77);
  gprint(" RDY:", 0xAACCEE);
  gprint_dec(ready, 0x77FF77);

  cursor_x = 638;
  cursor_y = 160;
  gprint("BLK:", 0xAACCEE);
  gprint_dec(blocked, 0xFFAA66);
  gprint(" SLP:", 0xAACCEE);
  gprint_dec(sleeping, 0xFFAA66);

  cursor_x = 638;
  cursor_y = 178;
  gprint("SLICE:", 0xAACCEE);
  gprint_dec((int)os_tasks[safe_current_task].time_slice, 0xFFFFFF);

  cursor_x = 638;
  cursor_y = 192;
  gprint("RT:", 0xAACCEE);
  gprint_dec((int)os_tasks[safe_current_task].runtime_ticks, 0xFFFFFF);
  gprint("t", 0xAACCEE);

  cursor_x = 638;
  cursor_y = 206;
  gprint("CSW:", 0xAACCEE);
  gprint_dec((int)os_tasks[safe_current_task].context_switches, 0xFFE066);

  cursor_x = 638;
  cursor_y = 220;
  gprint("MEM:", 0xAACCEE);
  gprint_dec(free_pages, 0x77FF77);
  gprint(" free", 0xAACCEE);

  cursor_x = 638;
  cursor_y = 234;
  gprint("PRESS:", 0xAACCEE);
  gprint_dec(pressure_pct, 0xFFE066);
  gprint("%", 0xAACCEE);

  cursor_x = 638;
  cursor_y = 248;
  gprint("SNAP:", 0xAACCEE);
  gprint_dec(storage_snapshot_used_count(), 0x88FFCC);
  gprint("/", 0xAACCEE);
  gprint_dec(storage_snapshot_capacity(), 0x88FFCC);
  gprint(" slots", 0xAACCEE);

  cursor_x = 638;
  cursor_y = 262;
  gprint("DISK:", 0xAACCEE);
  gprint(ata_disk_available ? "ONLINE" : "OFFLINE",
         ata_disk_available ? 0x77FF77 : 0xFF7777);

  cursor_x = 638;
  cursor_y = 276;
  gprint("NET:", 0xAACCEE);
  gprint(net_is_ready() ? "ONLINE" : "OFFLINE",
         net_is_ready() ? 0x77FF77 : 0xFF7777);

  {
    ProfileSnapshot snap;
    uint32_t render_avg;
    uint32_t render_roll;
    uint32_t sched_avg;
    uint32_t sched_roll;
    uint32_t spike_avg;
    uint32_t spike_roll;
    uint32_t cmd_avg;
    uint32_t cmd_roll;
    int hud_mode = profile_get_hud_mode();
    profile_snapshot(&snap);

    render_avg = metric_avg_cycles(&snap.slots[PROFILE_SLOT_RENDER_PASS]);
    render_roll = metric_roll_cycles(PROFILE_SLOT_RENDER_PASS);
    sched_avg = metric_avg_cycles(&snap.slots[PROFILE_SLOT_SCHED_TICK]);
    sched_roll = metric_roll_cycles(PROFILE_SLOT_SCHED_TICK);
    spike_avg = metric_avg_cycles(&snap.slots[PROFILE_SLOT_SPIKE_UPDATE]);
    spike_roll = metric_roll_cycles(PROFILE_SLOT_SPIKE_UPDATE);
    cmd_avg = metric_avg_cycles(&snap.slots[PROFILE_SLOT_COMMAND]);
    cmd_roll = metric_roll_cycles(PROFILE_SLOT_COMMAND);

    cursor_x = 638;
    cursor_y = 286;
    gprint("PRF:", 0xAACCEE);
    gprint(snap.enabled ? "ON" : "OFF", snap.enabled ? 0x77FF77 : 0xFFAA66);
    gprint(" ", 0x000000);
    gprint(hud_mode == PROFILE_HUD_DETAILED ? "D" : "C", 0xFFFFFF);

    cursor_x = 638;
    cursor_y = 298;
    if (hud_mode == PROFILE_HUD_DETAILED) {
      gprint("R", 0xAACCEE);
      gprint_dec((int)render_avg, 0xFFFFFF);
      gprint("/", 0x666666);
      gprint_dec((int)render_roll, 0xFFFFFF);
      gprint(" S", 0xAACCEE);
      gprint_dec((int)sched_avg, 0xFFFFFF);
      gprint("/", 0x666666);
      gprint_dec((int)sched_roll, 0xFFFFFF);

      cursor_x = 728;
      cursor_y = 286;
      gprint("C", 0xAACCEE);
      gprint_dec((int)cmd_avg, 0xFFFFFF);
      gprint("/", 0x666666);
      gprint_dec((int)cmd_roll, 0xFFFFFF);

      cursor_x = 728;
      cursor_y = 298;
      gprint("P", 0xAACCEE);
      gprint_dec((int)spike_avg, 0xFFFFFF);
      gprint("/", 0x666666);
      gprint_dec((int)spike_roll, 0xFFFFFF);
    } else {
      gprint("R:", 0xAACCEE);
      gprint_dec((int)render_avg, 0xFFFFFF);
      gprint(" C:", 0xAACCEE);
      gprint_dec((int)cmd_avg, 0xFFFFFF);
      gprint(" S:", 0xAACCEE);
      gprint_dec((int)sched_avg, 0xFFFFFF);
    }

    cursor_x = 638;
    cursor_y = 310;
    gprint("P0 ", 0xAACCEE);
    gprint((char[]){task_state_code(os_tasks[0].state), '\0'}, 0x77FFAA);
    gprint(" pr", 0xAACCEE);
    gprint_dec((int)os_tasks[0].priority, 0xFFFFFF);
    gprint(" tr", 0xAACCEE);
    gprint(os_tasks[0].trace_enabled ? "1" : "0", os_tasks[0].trace_enabled ? 0x44FF88 : 0xFFAA66);
    gprint(" rt", 0xAACCEE);
    gprint_dec((int)os_tasks[0].runtime_ticks, 0xFFFFFF);

    cursor_x = 638;
    cursor_y = 322;
    if (os_task_count > 1) {
      gprint("P1 ", 0xAACCEE);
      gprint((char[]){task_state_code(os_tasks[1].state), '\0'}, 0x77FFAA);
      gprint(" pr", 0xAACCEE);
      gprint_dec((int)os_tasks[1].priority, 0xFFFFFF);
      gprint(" tr", 0xAACCEE);
      gprint(os_tasks[1].trace_enabled ? "1" : "0", os_tasks[1].trace_enabled ? 0x44FF88 : 0xFFAA66);
      gprint(" rt", 0xAACCEE);
      gprint_dec((int)os_tasks[1].runtime_ticks, 0xFFFFFF);
    } else {
      gprint("P1 --", 0x666666);
    }
  }
}

static void draw_command_overlay(void) {
  clear_region(0, 346, 800, 490, 0x000A22);
  draw_hline(346, 0, 800, 0x223355);
  draw_hline(360, 0, 800, 0x112244);
  draw_hline(374, 0, 800, 0x112244);
  draw_hline(388, 0, 800, 0x112244);
  draw_hline(402, 0, 800, 0x112244);
  draw_hline(416, 0, 800, 0x112244);
  draw_hline(430, 0, 800, 0x112244);
  draw_hline(444, 0, 800, 0x112244);
  draw_hline(458, 0, 800, 0x112244);
  draw_hline(472, 0, 800, 0x112244);

  cursor_x = 4;
  cursor_y = 349;
  gprint("HUD CMDS: help | ls | save <id> | load <id> | mkdemo | exec demo.bin",
         0x77C8FF);
  cursor_x = 4;
  cursor_y = 363;
  gprint("FAST KEYS: F1 stim0  F2 switch  F3 stim1  F4 save0  F5 load0",
         0x66EEAA);
  cursor_x = 4;
  cursor_y = 377;
  gprint("RESEARCH: synview synset synrule synpreset syncmp | sbrowse spreview stag sdiff",
    0xFFD37A);
  cursor_x = 4;
  cursor_y = 391;
  gprint("NET: net up|cfg|status|tx|export <slot>|profile  remote on|off|token", 0x88E0FF);
  cursor_x = 4;
  cursor_y = 405;
  gprint("PHASE8: manifest save|load|show  replay rec on|off|run  dataset export|import",
         0xA0E0FF);
  cursor_x = 4;
  cursor_y = 419;
  gprint("PHASE9: profile on|off|show|reset|export <path>|hud compact|detail", 0x9AF0C8);
  cursor_x = 4;
  cursor_y = 433;
  gprint("PHASE10: model show|select|param  stdp on|off", 0xC9F59A);
  cursor_x = 4;
  cursor_y = 447;
  gprint("PHASE11: ps  kill <pid>  nice <pid> <0..3>  trace <pid> on|off|show", 0xF5C89A);
  cursor_x = 4;
  cursor_y = 461;
  gprint("PHASE11.1: ipc send|recv|stat <ch> [val]", 0xE8B6FF);
  cursor_x = 4;
  cursor_y = 475;
  gprint("PHASE12: net up|cfg|status|profile  remote on|off|token <hex>", 0xB6E8FF);
  cursor_x = 4;
  cursor_y = 489;
  gprint("PHASE13: viz heatmap|raster|off|show  viz scrub <0..63|+|->  viz play on|off", 0xFFE0A0);
}

void draw_status_bar(void) {
  clear_region(0, 0, 800, 100, 0x001122);
  draw_hline(100, 0, 800, 0x00FFFF);
  draw_hline(101, 0, 800, 0x006666);

  cursor_x = 4;
  cursor_y = 8;
  gprint("NEUROSPARK HUD | ", 0x00FFFF);

  gprint("PCI: ", 0xFFFF00);
  gprint_dec(pci_found_count, 0xFFFFFF);
  gprint(" dev", 0xFFFF00);

  cursor_x = 4;
  cursor_y = 22;
  gprint("MEM FREE: ", 0xAAAAAA);
  int free_pages = pmm_get_free_pages();
  gprint_dec(free_pages, 0x88FF88);
  gprint(" pages\n", 0xAAAAAA);

  cursor_x = 4;
  cursor_y = 36;
  gprint("TICK: ", 0xAAAAAA);
  gprint_dec((int)tick, 0xFF8800);
  gprint("  SPS[0]: ", 0xAAAAAA);
  gprint_dec(task_list[0].spikes_per_second, 0xFF5500);
  gprint(" SPS[1]: ", 0xAAAAAA);
  gprint_dec(task_list[1].spikes_per_second, 0xFF5500);

  cursor_x = 4;
  cursor_y = 50;
  gprint("ZOOM: ", 0xAAAAAA);
  gprint_dec(zoom_level, 0x00FFFF);
  gprint("x\n", 0xAAAAAA);

  cursor_x = 4;
  cursor_y = 64;
  for (int p = 0; p < 2; p++) {
    const char *ph_str = (os_memory_map[p].current_phase == 1) ? "RIGID" : "FLUID";
    uint32_t ph_clr = (os_memory_map[p].current_phase == 1) ? 0xFF4444 : 0x44FF44;
    gprint((char *)task_list[p].task_name, 0xFFFF00);
    gprint(": ", 0x446666);
    gprint((char *)ph_str, ph_clr);
    gprint("  ", 0x000000);
  }

  for (int p = 0; p < 2; p++) {
    if (prev_phase[p] == 0 && os_memory_map[p].current_phase == 1) {
      if (ata_disk_available) {
        disk_read_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
        int slot = find_free_slot(root_directory);
        if (slot != -1) {
          root_directory[slot].lba = TFS_DATA_START + slot;
          root_directory[slot].flags = 1;
          root_directory[slot].size = sizeof(potentials);
          root_directory[slot].name[0] = 'a';
          root_directory[slot].name[1] = 'u';
          root_directory[slot].name[2] = 't';
          root_directory[slot].name[3] = 'o';
          root_directory[slot].name[4] = '_';
          root_directory[slot].name[5] = (char)('0' + p);
          root_directory[slot].name[6] = '\0';
          disk_write_sector(root_directory[slot].lba, (uint16_t *)potentials);
          disk_write_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
        }
      }
    }
    prev_phase[p] = os_memory_map[p].current_phase;
  }
}

void draw_waveform(void) {
  const int WIN_Y0 = 110;
  const int WIN_Y1 = 300;
  const int WIN_X0 = 176;
  const int WIN_X1 = 624;
  const int WIN_W = WIN_X1 - WIN_X0;
  const int WIN_H = WIN_Y1 - WIN_Y0;
  const int BASE_Y = WIN_Y0 + WIN_H - 10;
  int safe_zoom = zoom_level;
  if (safe_zoom < 1) {
    safe_zoom = 1;
  }

  int BAR_W = 4 * safe_zoom;
  if (BAR_W > 20)
    BAR_W = 20;

  clear_region(WIN_X0, WIN_Y0, WIN_X1, WIN_Y1, 0x000033);
  draw_hline(WIN_Y0 - 1, WIN_X0, WIN_X1, 0x334455);
  draw_hline(WIN_Y1, WIN_X0, WIN_X1, 0x334455);

  cursor_x = WIN_X0 + 6;
  cursor_y = WIN_Y0;
  gprint("NEURAL WAVEFORM [HUD CORE]", 0x3366AA);

  int neurons_visible = 16 / safe_zoom;
  if (neurons_visible < 1)
    neurons_visible = 1;
  int start_n = zoom_offset;
  if (start_n + neurons_visible > 16)
    start_n = 16 - neurons_visible;
  if (start_n < 0)
    start_n = 0;

  int spacing = WIN_W / (neurons_visible + 1);
  int prev_px = -1, prev_py = -1;

  for (int idx = 0; idx < neurons_visible; idx++) {
    int i = start_n + idx;
    int potential = potentials[i];
    if (potential < 0)
      potential = 0;
    if (potential > 1000)
      potential = 1000;

    int bar_h = (potential * (WIN_H - 20)) / 1000;
    int px = WIN_X0 + spacing + idx * spacing;
    int py = BASE_Y - bar_h;

    uint32_t color;
    if (potential < 333)
      color = 0x00CC44;
    else if (potential < 666)
      color = 0xFFDD00;
    else
      color = 0xFF3300;

    for (int dy = 0; dy < bar_h && dy < WIN_H - 10; dy++) {
      for (int dx = 0; dx < BAR_W; dx++) {
        put_pixel(px - BAR_W / 2 + dx, BASE_Y - dy, color);
      }
    }

    if (prev_px >= 0) {
      int dx = px - prev_px;
      int ddx = dx > 0 ? 1 : -1;
      for (int xi = prev_px; xi != px; xi += ddx) {
        int t_y = prev_py + (py - prev_py) * (xi - prev_px) / (dx ? dx : 1);
        put_pixel(xi, t_y, 0x005588);
      }
    }
    prev_px = px;
    prev_py = py;
  }
}

void shell_render(void) {
  extern char cmd_output[256];
  extern int cmd_output_valid;

  clear_region(0, shell_cursor_y, 800, shell_cursor_y + 10, 0x000033);

  int saved_cx = cursor_x;
  int saved_cy = cursor_y;
  cursor_x = 0;
  cursor_y = shell_cursor_y;
  gprint("> ", 0x00FFFF);

  for (int i = 0; i < buffer_idx && i < 32; i++) {
    char ch[2] = {input_buffer[i], '\0'};
    gprint(ch, 0xFFFFFF);
  }

  shell_cursor_x = 16 + (buffer_idx * 8);

  /* Persistent command output line below prompt */
  clear_region(0, 330, 800, 345, 0x000033);
  if (cmd_output_valid) {
    cursor_x = 0;
    cursor_y = 330;
    gprint(cmd_output, 0x44FF88);
  }

  cursor_x = saved_cx;
  cursor_y = saved_cy;
  shell_dirty = 0;
}

void neuro_task_entry(void) {
  while (1) {
    uint32_t render_profile_stamp = profile_begin();

    pulse_neurons();
    record_raster_sample();
    if (viz_mode == VIZ_MODE_RASTER && viz_autoplay) {
      viz_step_scrub(1);
    }

    clear_region(0, 102, 800, 310, 0x000033);
    draw_status_bar();
    draw_hud_telemetry();
    draw_waveform();
    draw_phase13_viz();

    shell_render();
    draw_command_overlay();
    draw_cursor(tick);
    draw_mouse_cursor();

    cursor_x = 0;
    cursor_y = 312;

    /* Kernel dashboard task flips directly to avoid syscall privilege ambiguity. */
    __asm__ volatile("cli");
    flip_mutex = 1;
    flip_buffer();
    flip_mutex = 0;
    __asm__ volatile("sti");

    profile_end(PROFILE_SLOT_RENDER_PASS, render_profile_stamp);

    for (volatile int d = 0; d < 400000; d++)
      ;

    schedule();
  }
}

void draw_neural_spike(int neuron_id, uint32_t intensity) {
  int start_x = 100 + (neuron_id * 40);
  for (int i = 0; i < 30; i++) {
    for (int j = 0; j < (int)(intensity / 10); j++) {
      put_pixel(start_x + i, 500 - j, 0x00FF00);
    }
  }
}
