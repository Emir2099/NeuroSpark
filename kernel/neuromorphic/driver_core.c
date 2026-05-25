#include "uapi_neuro.h"

#define MAX_SIM_NEURONS 1024
static uint32_t current_backend = NEURO_BACKEND_SIMULATOR;

// Simulator State
static int sim_membrane[MAX_SIM_NEURONS];
static uint32_t sim_last_spike[MAX_SIM_NEURONS];

void neuro_init(void) {
    current_backend = NEURO_BACKEND_SIMULATOR;
    int i;
    for (i = 0; i < MAX_SIM_NEURONS; i++) {
        sim_membrane[i] = -65; // Resting potential
        sim_last_spike[i] = 0;
    }
}

int neuro_set_backend(uint32_t backend_id) {
    if (backend_id == NEURO_BACKEND_SIMULATOR || backend_id == NEURO_BACKEND_EXTERNAL) {
        current_backend = backend_id;
        return 1;
    }
    return 0;
}

uint32_t neuro_get_backend(void) {
    return current_backend;
}

void neuro_get_caps(NeuroCaps *out_caps) {
    if (!out_caps) return;
    
    out_caps->backend_id = current_backend;
    if (current_backend == NEURO_BACKEND_SIMULATOR) {
        out_caps->max_neurons = MAX_SIM_NEURONS;
        out_caps->max_synapses = MAX_SIM_NEURONS * 100;
        out_caps->supports_plasticity = 1;
        out_caps->max_spike_rate_hz = 1000;
        
        char *name = "Software Simulator Backend";
        int i = 0;
        while (name[i] && i < 31) {
            out_caps->name[i] = name[i];
            i++;
        }
        out_caps->name[i] = '\0';
    } else {
        out_caps->max_neurons = 0; 
        out_caps->max_synapses = 0;
        out_caps->supports_plasticity = 0;
        out_caps->max_spike_rate_hz = 0;
        
        char *name = "External Hardware (Offline)";
        int i = 0;
        while (name[i] && i < 31) {
            out_caps->name[i] = name[i];
            i++;
        }
        out_caps->name[i] = '\0';
    }
}

int neuro_inject_spike(uint32_t neuron_id, uint32_t timestamp) {
    if (current_backend == NEURO_BACKEND_EXTERNAL) {
        return 0; // External I/O not physically hooked up
    }
    
    if (neuron_id < MAX_SIM_NEURONS) {
        sim_membrane[neuron_id] += 15; // Synaptic weight
        sim_last_spike[neuron_id] = timestamp;
        
        if (sim_membrane[neuron_id] > 30) {
             sim_membrane[neuron_id] = -65; // Reset after spike threshold
        }
        return 1;
    }
    return 0;
}

int neuro_read_membrane(uint32_t neuron_id, int *out_voltage) {
    if (current_backend == NEURO_BACKEND_EXTERNAL) {
        return 0;
    }
    
    if (neuron_id < MAX_SIM_NEURONS && out_voltage) {
        // Simple decay logic on read: Decay towards -65
        if (sim_membrane[neuron_id] > -65) {
            sim_membrane[neuron_id] -= 1; // Decay by 1mV per read
        }
        *out_voltage = sim_membrane[neuron_id];
        return 1;
    }
    return 0;
}
