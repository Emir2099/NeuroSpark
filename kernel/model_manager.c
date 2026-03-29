#include "model_manager.h"

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

typedef struct {
  int tau;
  int refractory_ticks;
  int adaptation_step;
  int learning_rate;
  int weight_min;
  int weight_max;
} ModelNeuronParams;

#define MODEL_TASK_COUNT 2
#define MODEL_NEURON_COUNT 5

extern TaskControlBlock task_list[2];
extern NeuralPixel os_memory_map[2];

static int task_model[MODEL_TASK_COUNT];
static int stdp_enabled = 0;
static ModelNeuronParams params[MODEL_TASK_COUNT][MODEL_NEURON_COUNT];

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

static int clamp_i32(int value, int lo, int hi) {
  if (value < lo)
    return lo;
  if (value > hi)
    return hi;
  return value;
}

static int model_name_to_kind(const char *name) {
  if (str_eq_local(name, "lif")) {
    return MODEL_KIND_LIF;
  }
  if (str_eq_local(name, "adapt")) {
    return MODEL_KIND_ADAPT;
  }
  if (str_eq_local(name, "stdp")) {
    return MODEL_KIND_STDP;
  }
  return -1;
}

static int task_index_valid(int task_id) {
  return task_id >= 0 && task_id < MODEL_TASK_COUNT;
}

static int neuron_index_valid(int neuron_id) {
  return neuron_id >= 0 && neuron_id < MODEL_NEURON_COUNT;
}

static ModelNeuronParams *params_at(int task_id, int neuron_id) {
  return &params[task_id][neuron_id];
}

void model_manager_init(void) {
  stdp_enabled = 0;
  task_model[0] = MODEL_KIND_LIF;
  task_model[1] = MODEL_KIND_ADAPT;

  for (int t = 0; t < MODEL_TASK_COUNT; t++) {
    for (int n = 0; n < MODEL_NEURON_COUNT; n++) {
      ModelNeuronParams *p = params_at(t, n);
      p->tau = 1;
      p->refractory_ticks = 10;
      p->adaptation_step = 200;
      p->learning_rate = 10;
      p->weight_min = 50;
      p->weight_max = 2000;
    }
  }
}

int model_select(int task_id, const char *name) {
  int kind = model_name_to_kind(name);
  if (kind < 0) {
    return 0;
  }

  if (task_id < 0) {
    for (int t = 0; t < MODEL_TASK_COUNT; t++) {
      task_model[t] = kind;
    }
    return 1;
  }

  if (!task_index_valid(task_id)) {
    return 0;
  }
  task_model[task_id] = kind;
  return 1;
}

const char *model_name(int task_id) {
  if (!task_index_valid(task_id)) {
    return "na";
  }
  if (task_model[task_id] == MODEL_KIND_LIF) {
    return "lif";
  }
  if (task_model[task_id] == MODEL_KIND_ADAPT) {
    return "adapt";
  }
  if (task_model[task_id] == MODEL_KIND_STDP) {
    return "stdp";
  }
  return "na";
}

int model_set_param(int task_id, int neuron_id, const char *key, int value) {
  int t0 = 0;
  int t1 = MODEL_TASK_COUNT - 1;
  int n0 = 0;
  int n1 = MODEL_NEURON_COUNT - 1;

  if (key == 0 || key[0] == '\0') {
    return 0;
  }

  if (task_id >= 0) {
    if (!task_index_valid(task_id)) {
      return 0;
    }
    t0 = task_id;
    t1 = task_id;
  }

  if (neuron_id >= 0) {
    if (!neuron_index_valid(neuron_id)) {
      return 0;
    }
    n0 = neuron_id;
    n1 = neuron_id;
  }

  for (int t = t0; t <= t1; t++) {
    for (int n = n0; n <= n1; n++) {
      ModelNeuronParams *p = params_at(t, n);
      if (str_eq_local(key, "tau")) {
        p->tau = clamp_i32(value, 1, 8);
      } else if (str_eq_local(key, "refractory") || str_eq_local(key, "ref")) {
        p->refractory_ticks = clamp_i32(value, 1, 40);
      } else if (str_eq_local(key, "adapt") || str_eq_local(key, "astp")) {
        p->adaptation_step = clamp_i32(value, 10, 600);
      } else if (str_eq_local(key, "lr") || str_eq_local(key, "learn")) {
        p->learning_rate = clamp_i32(value, 1, 80);
      } else if (str_eq_local(key, "wmin")) {
        p->weight_min = clamp_i32(value, 1, 2000);
        if (p->weight_min > p->weight_max) {
          p->weight_min = p->weight_max;
        }
      } else if (str_eq_local(key, "wmax")) {
        p->weight_max = clamp_i32(value, 10, 4000);
        if (p->weight_max < p->weight_min) {
          p->weight_max = p->weight_min;
        }
      } else {
        return 0;
      }
    }
  }

  return 1;
}

void model_set_stdp(int enabled) { stdp_enabled = enabled ? 1 : 0; }

int model_get_stdp(void) { return stdp_enabled; }

int model_adjust_base_gain(int task_id, int neuron_id, int base_gain, uint32_t tick) {
  int gain = base_gain;
  int kind;
  ModelNeuronParams *p;
  int tau;

  if (!task_index_valid(task_id) || !neuron_index_valid(neuron_id)) {
    return base_gain;
  }

  kind = task_model[task_id];
  p = params_at(task_id, neuron_id);

  tau = p->tau;
  if (tau <= 0) {
    tau = 1;
  }

  gain = gain / tau;
  if (gain < 1) {
    gain = 1;
  }

  if (kind == MODEL_KIND_ADAPT) {
    gain += (p->adaptation_step / 40);
  } else if (kind == MODEL_KIND_STDP) {
    if (tick % 3 == 0) {
      gain += (p->learning_rate / 3);
    }
  }

  return gain;
}

int model_get_refractory_ticks(int task_id, int neuron_id) {
  if (!task_index_valid(task_id) || !neuron_index_valid(neuron_id)) {
    return 10;
  }
  {
    int v = params_at(task_id, neuron_id)->refractory_ticks;
    if (v <= 0) {
      v = 10;
    }
    return v;
  }
}

int model_get_threshold_raise_step(int task_id, int neuron_id) {
  int kind;
  ModelNeuronParams *p;
  int step;

  if (!task_index_valid(task_id) || !neuron_index_valid(neuron_id)) {
    return 200;
  }

  kind = task_model[task_id];
  p = params_at(task_id, neuron_id);
  step = p->adaptation_step;
  if (step <= 0) {
    step = 200;
  }

  if (kind == MODEL_KIND_LIF) {
    step = step / 2;
    if (step < 40) {
      step = 40;
    }
  }

  return step;
}

int model_get_threshold_decay_step(int task_id, int neuron_id) {
  int kind;
  ModelNeuronParams *p;

  if (!task_index_valid(task_id) || !neuron_index_valid(neuron_id)) {
    return 50;
  }

  kind = task_model[task_id];
  p = params_at(task_id, neuron_id);
  if (p->adaptation_step <= 0) {
    p->adaptation_step = 200;
  }
  if (p->learning_rate <= 0) {
    p->learning_rate = 10;
  }

  if (kind == MODEL_KIND_LIF) {
    return 35;
  }
  if (kind == MODEL_KIND_STDP) {
    return 45 + (p->learning_rate / 8);
  }
  return 50 + (p->adaptation_step / 12);
}

void model_on_spike(int task_id, int pixel_id, int neuron_id) {
  int kind;
  int lr;
  int prev;
  int next;
  Neuron *n;
  Neuron *prev_n;
  Neuron *next_n;
  ModelNeuronParams *p;

  if (!task_index_valid(task_id) || !neuron_index_valid(neuron_id) ||
      pixel_id < 0 || pixel_id >= MODEL_TASK_COUNT) {
    return;
  }

  kind = task_model[task_id];
  p = params_at(task_id, neuron_id);
  n = &os_memory_map[pixel_id].neurons[neuron_id];

  lr = p->learning_rate;
  if (lr <= 0) {
    lr = 1;
  }
  if (kind == MODEL_KIND_LIF) {
    lr = lr / 2;
    if (lr < 1) {
      lr = 1;
    }
  }

  n->synaptic_weight += lr;

  if (stdp_enabled || kind == MODEL_KIND_STDP) {
    prev = (neuron_id + MODEL_NEURON_COUNT - 1) % MODEL_NEURON_COUNT;
    next = (neuron_id + 1) % MODEL_NEURON_COUNT;
    prev_n = &os_memory_map[pixel_id].neurons[prev];
    next_n = &os_memory_map[pixel_id].neurons[next];

    if (prev_n->voltage > (prev_n->dynamic_threshold / 2)) {
      n->synaptic_weight += lr;
    }
    if (next_n->voltage < (next_n->dynamic_threshold / 4)) {
      n->synaptic_weight -= (lr / 2);
    }
  }

  if (n->synaptic_weight < p->weight_min) {
    n->synaptic_weight = p->weight_min;
  }
  if (n->synaptic_weight > p->weight_max) {
    n->synaptic_weight = p->weight_max;
  }
}

int model_manifest_capture(ModelManifestData *out) {
  if (out == 0) {
    return 0;
  }

  out->version = 1;
  out->stdp_enabled = (uint32_t)(stdp_enabled ? 1 : 0);

  for (int t = 0; t < MODEL_TASK_COUNT; t++) {
    out->task_model[t] = (uint32_t)task_model[t];
    for (int n = 0; n < MODEL_NEURON_COUNT; n++) {
      ModelNeuronParams *p = params_at(t, n);
      out->tau[t][n] = p->tau;
      out->refractory_ticks[t][n] = p->refractory_ticks;
      out->adaptation_step[t][n] = p->adaptation_step;
      out->learning_rate[t][n] = p->learning_rate;
      out->weight_min[t][n] = p->weight_min;
      out->weight_max[t][n] = p->weight_max;
    }
  }

  return 1;
}

int model_manifest_apply(const ModelManifestData *in) {
  if (in == 0 || in->version != 1) {
    return 0;
  }

  stdp_enabled = in->stdp_enabled ? 1 : 0;

  for (int t = 0; t < MODEL_TASK_COUNT; t++) {
    int tm = (int)in->task_model[t];
    if (tm < MODEL_KIND_LIF || tm > MODEL_KIND_STDP) {
      tm = MODEL_KIND_LIF;
    }
    task_model[t] = tm;

    for (int n = 0; n < MODEL_NEURON_COUNT; n++) {
      ModelNeuronParams *p = params_at(t, n);
      p->tau = clamp_i32(in->tau[t][n], 1, 8);
      p->refractory_ticks = clamp_i32(in->refractory_ticks[t][n], 1, 40);
      p->adaptation_step = clamp_i32(in->adaptation_step[t][n], 10, 600);
      p->learning_rate = clamp_i32(in->learning_rate[t][n], 1, 80);
      p->weight_min = clamp_i32(in->weight_min[t][n], 1, 2000);
      p->weight_max = clamp_i32(in->weight_max[t][n], 10, 4000);
      if (p->weight_min > p->weight_max) {
        p->weight_min = p->weight_max;
      }
    }
  }

  return 1;
}
