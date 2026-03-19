#include "scheduler.h"

typedef unsigned int uint32_t;

typedef struct {
  uint32_t esp;
  uint32_t ebp;
  uint32_t eip;
  uint32_t page_dir;
  uint32_t state;
  int task_id;
} TCB;

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

extern void switch_task(TCB *old, TCB *new);
extern TCB os_tasks[2];
extern int os_current_task;
extern TaskControlBlock task_list[2];
extern NeuralPixel os_memory_map[2];

void schedule(void) {
  int old_task = os_current_task;
  os_current_task = (os_current_task + 1) % 2;
  switch_task(&os_tasks[old_task], &os_tasks[os_current_task]);
}

void switch_tasks(void) {
  for (int p = 0; p < 2; p++) {
    for (int t = 0; t < 2; t++) {
      if (task_list[t].target_pixel == p) {
        for (int n = 0; n < 5; n++) {
          task_list[t].saved_voltages[n] = os_memory_map[p].neurons[n].voltage;
          task_list[t].saved_weights[n] = os_memory_map[p].neurons[n].synaptic_weight;
          task_list[t].saved_thresholds[n] = os_memory_map[p].neurons[n].dynamic_threshold;
        }
        break;
      }
    }
  }

  TaskControlBlock temp = task_list[0];
  task_list[0] = task_list[1];
  task_list[1] = temp;
  task_list[0].target_pixel = 0;
  task_list[1].target_pixel = 1;

  for (int p = 0; p < 2; p++) {
    for (int t = 0; t < 2; t++) {
      if (task_list[t].target_pixel == p) {
        for (int n = 0; n < 5; n++) {
          os_memory_map[p].neurons[n].voltage = task_list[t].saved_voltages[n];
          os_memory_map[p].neurons[n].synaptic_weight = task_list[t].saved_weights[n];
          os_memory_map[p].neurons[n].dynamic_threshold = task_list[t].saved_thresholds[n];
        }
        break;
      }
    }
  }
}
