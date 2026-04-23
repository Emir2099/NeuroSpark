#include "ai_scheduler.h"

#include "ai_runtime.h"

static AiSchedulerStats g_sched;

static uint32_t estimate_latency(uint32_t mode, uint32_t budget_pct,
                                 uint32_t queue_depth) {
  uint32_t base = (mode == AI_QOS_LATENCY) ? 2u : 6u;
  uint32_t budget_penalty = (100u - budget_pct) / 10u;
  uint32_t queue_penalty = queue_depth / 2u;
  return base + budget_penalty + queue_penalty;
}

void ai_scheduler_init(void) {
  g_sched.qos_mode = AI_QOS_LATENCY;
  g_sched.budget_pct = 50u;
  g_sched.queue_depth = 0;
  g_sched.max_queue_depth = 0;
  g_sched.submitted = 0;
  g_sched.completed = 0;
  g_sched.dropped = 0;
  g_sched.latency_accum = 0;
  g_sched.latency_max = 0;
  g_sched.last_latency = 0;
}

int ai_scheduler_set_qos(uint32_t mode, uint32_t budget_pct) {
  if ((mode != AI_QOS_LATENCY && mode != AI_QOS_THROUGHPUT) ||
      budget_pct == 0u || budget_pct > 100u) {
    return 0;
  }

  g_sched.qos_mode = mode;
  g_sched.budget_pct = budget_pct;
  return 1;
}

int ai_scheduler_submit_infer(const char *model_name, int input_value,
                              uint32_t seed, int *out_value) {
  uint32_t latency;

  if (model_name == 0 || out_value == 0) {
    g_sched.dropped++;
    return 0;
  }

  if (g_sched.queue_depth >= 64u) {
    g_sched.dropped++;
    return 0;
  }

  g_sched.submitted++;
  g_sched.queue_depth++;
  if (g_sched.queue_depth > g_sched.max_queue_depth) {
    g_sched.max_queue_depth = g_sched.queue_depth;
  }

  latency = estimate_latency(g_sched.qos_mode, g_sched.budget_pct,
                             g_sched.queue_depth);
  g_sched.last_latency = latency;
  g_sched.latency_accum += latency;
  if (latency > g_sched.latency_max) {
    g_sched.latency_max = latency;
  }

  if (!ai_runtime_infer(model_name, input_value, seed, out_value)) {
    g_sched.queue_depth--;
    g_sched.dropped++;
    return 0;
  }

  g_sched.completed++;
  g_sched.queue_depth--;
  return 1;
}

int ai_scheduler_run_bench(const char *model_name, int rounds, int input_value,
                           uint32_t seed, uint32_t *out_ok,
                           uint32_t *out_avg_latency,
                           uint32_t *out_max_latency) {
  uint32_t ok = 0;
  uint32_t lat_sum = 0;
  uint32_t lat_max = 0;
  int i;

  if (model_name == 0 || rounds <= 0 || rounds > 4096) {
    return 0;
  }

  for (i = 0; i < rounds; i++) {
    int out = 0;
    if (ai_scheduler_submit_infer(model_name, input_value + i,
                                  seed + (uint32_t)i, &out)) {
      ok++;
      lat_sum += g_sched.last_latency;
      if (g_sched.last_latency > lat_max) {
        lat_max = g_sched.last_latency;
      }
    }
  }

  if (out_ok != 0) {
    *out_ok = ok;
  }
  if (out_avg_latency != 0) {
    *out_avg_latency = (ok == 0u) ? 0u : (lat_sum / ok);
  }
  if (out_max_latency != 0) {
    *out_max_latency = lat_max;
  }

  return 1;
}

void ai_scheduler_get_stats(AiSchedulerStats *out_stats) {
  if (out_stats == 0) {
    return;
  }
  *out_stats = g_sched;
}
