#include "storage_manager.h"
#include "disk.h"
#include "klog.h"

typedef unsigned int uint32_t;

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
  char name[8];
  int is_used;
  int voltages[5];
  int weights[5];
  int thresholds[5];
} VirtualFile;

extern volatile VirtualFile synapse_disk[4];
extern TaskControlBlock task_list[2];
extern NeuralPixel os_memory_map[2];
extern void disk_write_sector(uint32_t lba, unsigned short *buffer);
extern void disk_read_sector(uint32_t lba, unsigned short *buffer);

void sys_save_task(int task_id, int slot) {
  if (slot >= 4)
    return;

  volatile VirtualFile *f = &synapse_disk[slot];
  TaskControlBlock *t = &task_list[task_id];

  for (int i = 0; i < 8; i++)
    f->name[i] = t->task_name[i];
  f->is_used = 1;

  int px = t->target_pixel;
  for (int n = 0; n < 5; n++) {
    f->voltages[n] = os_memory_map[px].neurons[n].voltage;
    f->weights[n] = os_memory_map[px].neurons[n].synaptic_weight;
    f->thresholds[n] = os_memory_map[px].neurons[n].dynamic_threshold;
  }

  __asm__ volatile("" ::: "memory");

  if (ata_disk_available) {
    disk_write_sector(DISK_DATA_OFFSET + slot, (unsigned short *)f);
    klog_info("snapshot persisted to disk");
  } else {
    klog_warn("disk unavailable, snapshot kept in memory");
  }
}

int sys_load_task(int task_id, int slot) {
  if (slot >= 4)
    return 0;

  if (ata_disk_available) {
    disk_read_sector(DISK_DATA_OFFSET + slot, (unsigned short *)&synapse_disk[slot]);
  }

  __asm__ volatile("" ::: "memory");

  if (!synapse_disk[slot].is_used)
    return 0;

  volatile VirtualFile *f = &synapse_disk[slot];
  TaskControlBlock *t = &task_list[task_id];

  int px = t->target_pixel;
  for (int n = 0; n < 5; n++) {
    os_memory_map[px].neurons[n].voltage = f->voltages[n];
    os_memory_map[px].neurons[n].synaptic_weight = f->weights[n];
    os_memory_map[px].neurons[n].dynamic_threshold = f->thresholds[n];

    t->saved_voltages[n] = f->voltages[n];
    t->saved_weights[n] = f->weights[n];
    t->saved_thresholds[n] = f->thresholds[n];
  }

  klog_info("snapshot loaded");
  return 1;
}
