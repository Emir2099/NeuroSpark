#ifndef AI_SCHEDULER_H
#define AI_SCHEDULER_H

#ifndef _UINT32_T_DEFINED
#define _UINT32_T_DEFINED
typedef unsigned int uint32_t;
#endif

enum {
  AI_QOS_LATENCY = 0,
  AI_QOS_THROUGHPUT = 1
};

typedef struct {
  uint32_t qos_mode;
  uint32_t budget_pct;
  uint32_t queue_depth;
  uint32_t max_queue_depth;
  uint32_t submitted;
  uint32_t completed;
  uint32_t dropped;
  uint32_t latency_accum;
  uint32_t latency_max;
  uint32_t last_latency;
} AiSchedulerStats;

void ai_scheduler_init(void);
int ai_scheduler_set_qos(uint32_t mode, uint32_t budget_pct);
int ai_scheduler_submit_infer(const char *model_name, int input_value,
                              uint32_t seed, int *out_value);
int ai_scheduler_run_bench(const char *model_name, int rounds, int input_value,
                           uint32_t seed, uint32_t *out_ok,
                           uint32_t *out_avg_latency,
                           uint32_t *out_max_latency);
void ai_scheduler_get_stats(AiSchedulerStats *out_stats);

#endif