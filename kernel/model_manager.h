#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

typedef unsigned int uint32_t;

#define MODEL_KIND_LIF 0
#define MODEL_KIND_ADAPT 1
#define MODEL_KIND_STDP 2

typedef struct {
  uint32_t version;
  uint32_t stdp_enabled;
  uint32_t task_model[2];
  int tau[2][5];
  int refractory_ticks[2][5];
  int adaptation_step[2][5];
  int learning_rate[2][5];
  int weight_min[2][5];
  int weight_max[2][5];
} ModelManifestData;

void model_manager_init(void);

int model_select(int task_id, const char *name);
const char *model_name(int task_id);

int model_set_param(int task_id, int neuron_id, const char *key, int value);

void model_set_stdp(int enabled);
int model_get_stdp(void);

int model_adjust_base_gain(int task_id, int neuron_id, int base_gain, uint32_t tick);
int model_get_refractory_ticks(int task_id, int neuron_id);
int model_get_threshold_raise_step(int task_id, int neuron_id);
int model_get_threshold_decay_step(int task_id, int neuron_id);
void model_on_spike(int task_id, int pixel_id, int neuron_id);

int model_manifest_capture(ModelManifestData *out);
int model_manifest_apply(const ModelManifestData *in);

#endif
