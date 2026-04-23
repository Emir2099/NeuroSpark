#include "ai_train.h"

#include "ai_runtime.h"

static AiTrainStats g_train;

static void copy_name(char *dst, int dst_len, const char *src) {
  int i = 0;
  if (dst == 0 || dst_len <= 0) {
    return;
  }
  if (src == 0) {
    dst[0] = '\0';
    return;
  }
  while (src[i] && i < (dst_len - 1)) {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
}

static int abs_i32(int v) {
  return (v < 0) ? -v : v;
}

void ai_train_init(void) {
  g_train.active = 0;
  g_train.paused = 0;
  g_train.total_steps = 0;
  g_train.checkpoints = 0;
  g_train.rollbacks = 0;
  g_train.divergence_events = 0;
  g_train.drift_alarms = 0;
  g_train.learning_rate_ppm = 100;
  g_train.max_drift = 5000;
  g_train.current_bias = 0;
  g_train.checkpoint_bias = 0;
  g_train.model_name[0] = '\0';
}

int ai_train_start(const char *model_name, uint32_t learning_rate_ppm,
                   uint32_t max_drift) {
  int bias = 0;

  if (model_name == 0 || model_name[0] == '\0') {
    return 0;
  }
  if (learning_rate_ppm == 0u || learning_rate_ppm > 10000u) {
    return 0;
  }
  if (max_drift < 100u || max_drift > 100000u) {
    return 0;
  }

  if (!ai_runtime_get_model_bias(model_name, &bias)) {
    return 0;
  }

  g_train.active = 1;
  g_train.paused = 0;
  g_train.total_steps = 0;
  g_train.learning_rate_ppm = learning_rate_ppm;
  g_train.max_drift = max_drift;
  g_train.current_bias = bias;
  g_train.checkpoint_bias = bias;
  copy_name(g_train.model_name, sizeof(g_train.model_name), model_name);
  return 1;
}

int ai_train_step(uint32_t steps) {
  uint32_t i;
  uint32_t stride;

  if (!g_train.active || g_train.paused || g_train.model_name[0] == '\0') {
    return 0;
  }

  if (steps == 0u) {
    steps = 1u;
  }
  if (steps > 4096u) {
    steps = 4096u;
  }

  stride = (g_train.learning_rate_ppm / 100u);
  if (stride == 0u) {
    stride = 1u;
  }

  for (i = 0; i < steps; i++) {
    int dir = ((g_train.total_steps + i) & 1u) ? 1 : -1;
    g_train.current_bias += (int)stride * dir;

    if (abs_i32(g_train.current_bias - g_train.checkpoint_bias) > (int)g_train.max_drift) {
      g_train.drift_alarms++;
      g_train.divergence_events++;
      g_train.current_bias = g_train.checkpoint_bias;
      g_train.rollbacks++;
      ai_runtime_set_model_bias(g_train.model_name, g_train.current_bias);
      return 0;
    }
  }

  g_train.total_steps += steps;
  ai_runtime_set_model_bias(g_train.model_name, g_train.current_bias);
  return 1;
}

int ai_train_pause(void) {
  if (!g_train.active) {
    return 0;
  }
  g_train.paused = 1;
  return 1;
}

int ai_train_resume(void) {
  if (!g_train.active) {
    return 0;
  }
  g_train.paused = 0;
  return 1;
}

int ai_train_checkpoint(void) {
  if (!g_train.active) {
    return 0;
  }
  g_train.checkpoint_bias = g_train.current_bias;
  g_train.checkpoints++;
  return 1;
}

int ai_train_rollback(void) {
  if (!g_train.active) {
    return 0;
  }
  g_train.current_bias = g_train.checkpoint_bias;
  g_train.rollbacks++;
  return ai_runtime_set_model_bias(g_train.model_name, g_train.current_bias);
}

void ai_train_get_stats(AiTrainStats *out_stats) {
  if (out_stats == 0) {
    return;
  }
  *out_stats = g_train;
}
