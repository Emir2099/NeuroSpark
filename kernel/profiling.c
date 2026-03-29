#include "profiling.h"
#include "vfs.h"

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t tick;
  ProfileSnapshot snapshot;
} ProfileExportBlob;

#define PROFILE_EXPORT_MAGIC 0x50524639u /* PRF9 */
#define PROFILE_EXPORT_VERSION 1u

static volatile int profile_enabled = 0;
static ProfileMetric profile_metrics[PROFILE_SLOT_COUNT];
static uint32_t profile_cmd_hist[PROFILE_CMD_HIST_BUCKETS];

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

void profile_init(void) {
  profile_enabled = 0;
  profile_reset();
}

void profile_set_enabled(int enabled) { profile_enabled = enabled ? 1 : 0; }

int profile_is_enabled(void) { return profile_enabled; }

void profile_reset(void) {
  for (int i = 0; i < PROFILE_SLOT_COUNT; i++) {
    metric_reset(&profile_metrics[i]);
  }
  for (int i = 0; i < PROFILE_CMD_HIST_BUCKETS; i++) {
    profile_cmd_hist[i] = 0;
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
