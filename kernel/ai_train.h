#ifndef AI_TRAIN_H
#define AI_TRAIN_H

#ifndef _UINT32_T_DEFINED
#define _UINT32_T_DEFINED
typedef unsigned int uint32_t;
#endif

typedef struct {
  uint32_t active;
  uint32_t paused;
  uint32_t total_steps;
  uint32_t checkpoints;
  uint32_t rollbacks;
  uint32_t divergence_events;
  uint32_t drift_alarms;
  uint32_t learning_rate_ppm;
  uint32_t max_drift;
  int current_bias;
  int checkpoint_bias;
  char model_name[16];
} AiTrainStats;

void ai_train_init(void);
int ai_train_start(const char *model_name, uint32_t learning_rate_ppm,
                   uint32_t max_drift);
int ai_train_step(uint32_t steps);
int ai_train_pause(void);
int ai_train_resume(void);
int ai_train_checkpoint(void);
int ai_train_rollback(void);
void ai_train_get_stats(AiTrainStats *out_stats);

#endif