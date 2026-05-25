#ifndef UAPI_NEURO_H
#define UAPI_NEURO_H

typedef unsigned int uint32_t;

#define NEURO_BACKEND_SIMULATOR 0
#define NEURO_BACKEND_EXTERNAL  1

typedef struct {
    uint32_t backend_id;
    uint32_t max_neurons;
    uint32_t max_synapses;
    uint32_t supports_plasticity;
    uint32_t max_spike_rate_hz;
    char name[32];
} NeuroCaps;

void neuro_init(void);
int neuro_set_backend(uint32_t backend_id);
uint32_t neuro_get_backend(void);
void neuro_get_caps(NeuroCaps *out_caps);
int neuro_inject_spike(uint32_t neuron_id, uint32_t timestamp);
int neuro_read_membrane(uint32_t neuron_id, int *out_voltage);

#endif
