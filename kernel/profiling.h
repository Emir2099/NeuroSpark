#ifndef PROFILING_H
#define PROFILING_H

typedef unsigned int uint32_t;

#define PROFILE_SLOT_SCHED_TICK 0
#define PROFILE_SLOT_RENDER_PASS 1
#define PROFILE_SLOT_SPIKE_UPDATE 2
#define PROFILE_SLOT_COMMAND 3
#define PROFILE_SLOT_COUNT 4

#define PROFILE_CMD_HIST_BUCKETS 5
#define PROFILE_ROLLING_WINDOW 32
#define PROFILE_NAMED_CMD_MAX 16

#define PROFILE_HUD_COMPACT 0
#define PROFILE_HUD_DETAILED 1

typedef struct {
  uint32_t count;
  uint32_t total_cycles;
  uint32_t last_cycles;
  uint32_t min_cycles;
  uint32_t max_cycles;
} ProfileMetric;

typedef struct {
  uint32_t enabled;
  ProfileMetric slots[PROFILE_SLOT_COUNT];
  uint32_t cmd_hist[PROFILE_CMD_HIST_BUCKETS];
} ProfileSnapshot;

typedef struct {
  char name[16];
  ProfileMetric metric;
  uint32_t rolling_avg_cycles;
} ProfileCommandStat;

void profile_init(void);
void profile_set_enabled(int enabled);
int profile_is_enabled(void);
void profile_reset(void);

uint32_t profile_begin(void);
void profile_end(uint32_t slot, uint32_t start_stamp);
uint32_t profile_end_and_get(uint32_t slot, uint32_t start_stamp);
void profile_record(uint32_t slot, uint32_t delta_cycles);
void profile_record_command(uint32_t delta_cycles);
void profile_record_named_command(const char *name, uint32_t delta_cycles);
uint32_t profile_metric_rolling_avg(uint32_t slot);
int profile_get_command_stats(ProfileCommandStat *out, int max_entries);

void profile_set_hud_mode(int mode);
int profile_get_hud_mode(void);

void profile_snapshot(ProfileSnapshot *out);
int profile_export(const char *path);

#endif
