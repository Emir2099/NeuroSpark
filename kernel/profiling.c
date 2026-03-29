#include "profiling.h"
#include "vfs.h"

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t tick;
  ProfileSnapshot snapshot;
} ProfileExportBlob;

#define PROFILE_EXPORT_MAGIC 0x50524639u /* PRF9 */
#define PROFILE_EXPORT_VERSION 2u

typedef struct {
  uint32_t samples[PROFILE_ROLLING_WINDOW];
  uint32_t sum;
  uint32_t index;
  uint32_t count;
} RollingMetric;

typedef struct {
  int used;
  char name[16];
  ProfileMetric metric;
  RollingMetric rolling;
} NamedCommandMetric;

static volatile int profile_enabled = 0;
static volatile int profile_hud_mode = PROFILE_HUD_COMPACT;
static ProfileMetric profile_metrics[PROFILE_SLOT_COUNT];
static uint32_t profile_cmd_hist[PROFILE_CMD_HIST_BUCKETS];
static RollingMetric profile_rolling[PROFILE_SLOT_COUNT];
static NamedCommandMetric named_cmds[PROFILE_NAMED_CMD_MAX];

extern uint32_t tick;

static uint32_t profile_now_cycles(void) {
  uint32_t lo;
  uint32_t hi;
  __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
  (void)hi;
  return lo;
}

static void metric_reset(ProfileMetric *m) {
  if (m == 0) {
    return;
  }
  m->count = 0;
  m->total_cycles = 0;
  m->last_cycles = 0;
  m->min_cycles = 0xFFFFFFFFu;
  m->max_cycles = 0;
}

static void rolling_reset(RollingMetric *r) {
  if (r == 0) {
    return;
  }
  r->sum = 0;
  r->index = 0;
  r->count = 0;
  for (int i = 0; i < PROFILE_ROLLING_WINDOW; i++) {
    r->samples[i] = 0;
  }
}

static void rolling_push(RollingMetric *r, uint32_t sample) {
  if (r == 0) {
    return;
  }

  if (r->count < PROFILE_ROLLING_WINDOW) {
    r->samples[r->index] = sample;
    r->sum += sample;
    r->count++;
    r->index++;
    if (r->index >= PROFILE_ROLLING_WINDOW) {
      r->index = 0;
    }
    return;
  }

  r->sum -= r->samples[r->index];
  r->samples[r->index] = sample;
  r->sum += sample;
  r->index++;
  if (r->index >= PROFILE_ROLLING_WINDOW) {
    r->index = 0;
  }
}

static uint32_t rolling_avg(const RollingMetric *r) {
  if (r == 0 || r->count == 0) {
    return 0;
  }
  return r->sum / r->count;
}

static int str_eq_local(const char *a, const char *b) {
  int i = 0;
  if (a == 0 || b == 0) {
    return 0;
  }
  while (a[i] && b[i]) {
    if (a[i] != b[i]) {
      return 0;
    }
    i++;
  }
  return a[i] == '\0' && b[i] == '\0';
}

static void copy_name(char *dst, const char *src) {
  int i = 0;
  if (dst == 0) {
    return;
  }
  if (src == 0) {
    dst[0] = '\0';
    return;
  }
  while (src[i] && i < 15) {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
}

static void metric_accumulate(ProfileMetric *m, uint32_t delta_cycles) {
  if (m == 0) {
    return;
  }
  m->count++;
  m->total_cycles += delta_cycles;
  m->last_cycles = delta_cycles;
  if (delta_cycles < m->min_cycles) {
    m->min_cycles = delta_cycles;
  }
  if (delta_cycles > m->max_cycles) {
    m->max_cycles = delta_cycles;
  }
}

static int metric_avg_cmp(const ProfileMetric *a, const ProfileMetric *b) {
  uint32_t aa = 0;
  uint32_t bb = 0;
  if (a != 0 && a->count > 0) {
    aa = a->total_cycles / a->count;
  }
  if (b != 0 && b->count > 0) {
    bb = b->total_cycles / b->count;
  }
  if (aa > bb) {
    return -1;
  }
  if (aa < bb) {
    return 1;
  }
  return 0;
}

void profile_init(void) {
  profile_enabled = 0;
  profile_hud_mode = PROFILE_HUD_COMPACT;
  profile_reset();
}

void profile_set_enabled(int enabled) { profile_enabled = enabled ? 1 : 0; }

int profile_is_enabled(void) { return profile_enabled; }

void profile_reset(void) {
  for (int i = 0; i < PROFILE_SLOT_COUNT; i++) {
    metric_reset(&profile_metrics[i]);
    rolling_reset(&profile_rolling[i]);
  }
  for (int i = 0; i < PROFILE_CMD_HIST_BUCKETS; i++) {
    profile_cmd_hist[i] = 0;
  }
  for (int i = 0; i < PROFILE_NAMED_CMD_MAX; i++) {
    named_cmds[i].used = 0;
    named_cmds[i].name[0] = '\0';
    metric_reset(&named_cmds[i].metric);
    rolling_reset(&named_cmds[i].rolling);
  }
}

uint32_t profile_begin(void) {
  if (!profile_enabled) {
    return 0;
  }
  return profile_now_cycles();
}

void profile_record(uint32_t slot, uint32_t delta_cycles) {
  ProfileMetric *m;

  if (!profile_enabled || slot >= PROFILE_SLOT_COUNT) {
    return;
  }

  m = &profile_metrics[slot];
  metric_accumulate(m, delta_cycles);
  rolling_push(&profile_rolling[slot], delta_cycles);
}

void profile_end(uint32_t slot, uint32_t start_stamp) {
  uint32_t now;

  if (start_stamp == 0 || !profile_enabled || slot >= PROFILE_SLOT_COUNT) {
    return;
  }

  now = profile_now_cycles();
  profile_record(slot, now - start_stamp);
}

uint32_t profile_end_and_get(uint32_t slot, uint32_t start_stamp) {
  uint32_t now;
  uint32_t delta;

  if (start_stamp == 0 || !profile_enabled || slot >= PROFILE_SLOT_COUNT) {
    return 0;
  }

  now = profile_now_cycles();
  delta = now - start_stamp;
  profile_record(slot, delta);
  return delta;
}

void profile_record_command(uint32_t delta_cycles) {
  if (!profile_enabled) {
    return;
  }

  if (delta_cycles < 50000u) {
    profile_cmd_hist[0]++;
  } else if (delta_cycles < 200000u) {
    profile_cmd_hist[1]++;
  } else if (delta_cycles < 500000u) {
    profile_cmd_hist[2]++;
  } else if (delta_cycles < 1000000u) {
    profile_cmd_hist[3]++;
  } else {
    profile_cmd_hist[4]++;
  }
}

void profile_record_named_command(const char *name, uint32_t delta_cycles) {
  int free_index = -1;
  int best_index = 0;
  uint32_t best_count = 0xFFFFFFFFu;

  if (!profile_enabled || name == 0 || name[0] == '\0' || delta_cycles == 0) {
    return;
  }

  for (int i = 0; i < PROFILE_NAMED_CMD_MAX; i++) {
    if (named_cmds[i].used) {
      if (str_eq_local(named_cmds[i].name, name)) {
        metric_accumulate(&named_cmds[i].metric, delta_cycles);
        rolling_push(&named_cmds[i].rolling, delta_cycles);
        return;
      }
      if (named_cmds[i].metric.count < best_count) {
        best_count = named_cmds[i].metric.count;
        best_index = i;
      }
    } else if (free_index < 0) {
      free_index = i;
    }
  }

  if (free_index >= 0) {
    best_index = free_index;
  }

  named_cmds[best_index].used = 1;
  copy_name(named_cmds[best_index].name, name);
  metric_reset(&named_cmds[best_index].metric);
  rolling_reset(&named_cmds[best_index].rolling);
  metric_accumulate(&named_cmds[best_index].metric, delta_cycles);
  rolling_push(&named_cmds[best_index].rolling, delta_cycles);
}

uint32_t profile_metric_rolling_avg(uint32_t slot) {
  if (slot >= PROFILE_SLOT_COUNT) {
    return 0;
  }
  return rolling_avg(&profile_rolling[slot]);
}

int profile_get_command_stats(ProfileCommandStat *out, int max_entries) {
  int count = 0;

  if (out == 0 || max_entries <= 0) {
    return 0;
  }

  for (int i = 0; i < PROFILE_NAMED_CMD_MAX && count < max_entries; i++) {
    if (!named_cmds[i].used) {
      continue;
    }
    copy_name(out[count].name, named_cmds[i].name);
    out[count].metric = named_cmds[i].metric;
    out[count].rolling_avg_cycles = rolling_avg(&named_cmds[i].rolling);
    count++;
  }

  for (int i = 0; i < count; i++) {
    for (int j = i + 1; j < count; j++) {
      if (metric_avg_cmp(&out[i].metric, &out[j].metric) > 0) {
        ProfileCommandStat tmp = out[i];
        out[i] = out[j];
        out[j] = tmp;
      }
    }
  }

  return count;
}

void profile_set_hud_mode(int mode) {
  if (mode == PROFILE_HUD_DETAILED) {
    profile_hud_mode = PROFILE_HUD_DETAILED;
  } else {
    profile_hud_mode = PROFILE_HUD_COMPACT;
  }
}

int profile_get_hud_mode(void) { return profile_hud_mode; }

void profile_snapshot(ProfileSnapshot *out) {
  if (out == 0) {
    return;
  }

  out->enabled = profile_enabled ? 1u : 0u;
  for (int i = 0; i < PROFILE_SLOT_COUNT; i++) {
    out->slots[i] = profile_metrics[i];
  }
  for (int i = 0; i < PROFILE_CMD_HIST_BUCKETS; i++) {
    out->cmd_hist[i] = profile_cmd_hist[i];
  }
}

int profile_export(const char *path) {
  ProfileExportBlob blob;
  int wrote;

  if (path == 0 || path[0] == '\0') {
    return 0;
  }

  blob.magic = PROFILE_EXPORT_MAGIC;
  blob.version = PROFILE_EXPORT_VERSION;
  blob.tick = tick;
  profile_snapshot(&blob.snapshot);

  wrote = vfs_write_file(path, &blob, sizeof(blob));
  return wrote == (int)sizeof(blob);
}
