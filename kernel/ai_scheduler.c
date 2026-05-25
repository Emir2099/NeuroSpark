#include "ai_scheduler.h"

#include "ai_runtime.h"
#include "scheduler.h"

#define AI_QUEUE_CAP 64u

typedef struct {
  char model_name[16];
  int input_value;
  uint32_t seed;
  int *out_value;
  uint32_t *done_flag;
  uint32_t latency;
  int in_use;
} AiQueuedJob;

static AiSchedulerStats g_sched;
static AiQueuedJob g_stream_q[AI_QUEUE_CAP];
static AiQueuedJob g_batch_q[AI_QUEUE_CAP];
static uint32_t g_stream_head;
static uint32_t g_stream_tail;
static uint32_t g_batch_head;
static uint32_t g_batch_tail;

static void copy_name16(char out[16], const char *src) {
  int i = 0;
  if (src == 0) {
    out[0] = '\0';
    return;
  }
  while (src[i] && i < 15) {
    out[i] = src[i];
    i++;
  }
  out[i] = '\0';
}

static int queue_push(AiQueuedJob *q, uint32_t *head, uint32_t *tail,
                      const AiQueuedJob *in) {
  uint32_t next_tail = (*tail + 1u) % AI_QUEUE_CAP;
  if (next_tail == *head) {
    return 0;
  }
  q[*tail] = *in;
  q[*tail].in_use = 1;
  *tail = next_tail;
  return 1;
}

static int queue_pop(AiQueuedJob *q, uint32_t *head, uint32_t tail,
                     AiQueuedJob *out) {
  if (*head == tail) {
    return 0;
  }
  *out = q[*head];
  q[*head].in_use = 0;
  *head = (*head + 1u) % AI_QUEUE_CAP;
  return 1;
}

static uint32_t queue_depth(uint32_t head, uint32_t tail) {
  if (tail >= head) {
    return tail - head;
  }
  return (AI_QUEUE_CAP - head) + tail;
}

static void refresh_queue_depths(void) {
  g_sched.stream_queue_depth = queue_depth(g_stream_head, g_stream_tail);
  g_sched.batch_queue_depth = queue_depth(g_batch_head, g_batch_tail);
  g_sched.queue_depth = g_sched.stream_queue_depth + g_sched.batch_queue_depth;
  if (g_sched.queue_depth > g_sched.max_queue_depth) {
    g_sched.max_queue_depth = g_sched.queue_depth;
  }
}

static uint32_t scheduler_budget_jobs(void) {
  uint32_t jobs = 1u;
  if (g_sched.budget_pct >= 75u) {
    jobs = 4u;
  } else if (g_sched.budget_pct >= 50u) {
    jobs = 3u;
  } else if (g_sched.budget_pct >= 25u) {
    jobs = 2u;
  }
  if (g_sched.qos_mode == AI_QOS_THROUGHPUT) {
    jobs += 1u;
  }
  return jobs;
}

static void process_one_job(AiQueuedJob *job) {
  int out = 0;
  if (!ai_runtime_infer(job->model_name, job->input_value, job->seed, &out)) {
    if (job->done_flag != 0) {
      *job->done_flag = 1u;
    }
    g_sched.dropped++;
    return;
  }
  if (job->out_value != 0) {
    *job->out_value = out;
  }
  if (job->done_flag != 0) {
    *job->done_flag = 1u;
  }
  g_sched.completed++;
  g_sched.processed_jobs++;
}

static void drain_queues(uint32_t budget_jobs) {
  uint32_t processed = 0;
  AiQueuedJob job;
  while (processed < budget_jobs) {
    int got = 0;
    if (g_sched.qos_mode == AI_QOS_LATENCY) {
      got = queue_pop(g_stream_q, &g_stream_head, g_stream_tail, &job);
      if (!got) {
        got = queue_pop(g_batch_q, &g_batch_head, g_batch_tail, &job);
      }
    } else {
      got = queue_pop(g_batch_q, &g_batch_head, g_batch_tail, &job);
      if (!got) {
        got = queue_pop(g_stream_q, &g_stream_head, g_stream_tail, &job);
      }
    }
    if (!got) {
      break;
    }
    process_one_job(&job);
    processed++;
  }
  refresh_queue_depths();
}

static uint32_t estimate_latency(uint32_t mode, uint32_t budget_pct,
                                 uint32_t queue_depth) {
  uint32_t base = (mode == AI_QOS_LATENCY) ? 2u : 6u;
  uint32_t budget_penalty = (100u - budget_pct) / 10u;
  uint32_t queue_penalty = queue_depth / 2u;
  return base + budget_penalty + queue_penalty;
}

void ai_scheduler_init(void) {
  int i;
  g_sched.qos_mode = AI_QOS_LATENCY;
  g_sched.budget_pct = 50u;
  g_sched.queue_depth = 0;
  g_sched.stream_queue_depth = 0;
  g_sched.batch_queue_depth = 0;
  g_sched.max_queue_depth = 0;
  g_sched.submitted = 0;
  g_sched.completed = 0;
  g_sched.processed_jobs = 0;
  g_sched.dropped = 0;
  g_sched.latency_accum = 0;
  g_sched.latency_max = 0;
  g_sched.last_latency = 0;

  g_stream_head = 0;
  g_stream_tail = 0;
  g_batch_head = 0;
  g_batch_tail = 0;
  for (i = 0; i < (int)AI_QUEUE_CAP; i++) {
    g_stream_q[i].in_use = 0;
    g_batch_q[i].in_use = 0;
  }
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
  AiQueuedJob job;
  uint32_t now = scheduler_now_ticks();
  uint32_t done = 0;
  uint32_t spin_budget = AI_QUEUE_CAP * 2u;
  int enqueued;

  if (model_name == 0 || out_value == 0) {
    g_sched.dropped++;
    return 0;
  }

  refresh_queue_depths();
  if (g_sched.queue_depth >= (AI_QUEUE_CAP - 1u)) {
    g_sched.dropped++;
    return 0;
  }

  job.input_value = input_value;
  job.seed = seed ^ now;
  job.out_value = out_value;
  job.done_flag = &done;
  job.latency = 0;
  job.in_use = 1;
  copy_name16(job.model_name, model_name);

  g_sched.submitted++;

  if (g_sched.qos_mode == AI_QOS_LATENCY) {
    enqueued = queue_push(g_stream_q, &g_stream_head, &g_stream_tail, &job);
  } else {
    enqueued = queue_push(g_batch_q, &g_batch_head, &g_batch_tail, &job);
  }
  if (!enqueued) {
    g_sched.dropped++;
    return 0;
  }

  refresh_queue_depths();

  latency = estimate_latency(g_sched.qos_mode, g_sched.budget_pct,
                             g_sched.queue_depth);
  g_sched.last_latency = latency;
  g_sched.latency_accum += latency;
  if (latency > g_sched.latency_max) {
    g_sched.latency_max = latency;
  }

  while (!done && spin_budget > 0u) {
    drain_queues(scheduler_budget_jobs());
    spin_budget--;
  }

  if (!done) {
    g_sched.dropped++;
    return 0;
  }

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

  drain_queues(1024u);

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
