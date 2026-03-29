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
  char snapshot_tag[16];
  int voltages[5];
  int weights[5];
  int thresholds[5];
} VirtualFile;

extern volatile VirtualFile synapse_disk[4];
extern TaskControlBlock task_list[2];
extern NeuralPixel os_memory_map[2];
extern void disk_write_sector(uint32_t lba, unsigned short *buffer);
extern void disk_read_sector(uint32_t lba, unsigned short *buffer);

#define SNAPSHOT_SLOTS 4
#define SNAPSHOT_TAG_LEN 16

static void copy_bytes(void *dst, const void *src, int len) {
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;
  for (int i = 0; i < len; i++) {
    d[i] = s[i];
  }
}

static void copy_text(char *dst, const char *src, int max_len) {
  int i = 0;
  if (max_len <= 0) {
    return;
  }
  if (src == 0) {
    dst[0] = '\0';
    return;
  }
  while (src[i] && i < max_len - 1) {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
}

void sys_save_task(int task_id, int slot) {
  if (slot < 0 || slot >= SNAPSHOT_SLOTS)
    return;

  volatile VirtualFile *f = &synapse_disk[slot];
  TaskControlBlock *t = &task_list[task_id];
  int first_save = (f->is_used == 0);

  for (int i = 0; i < 8; i++)
    f->name[i] = t->task_name[i];
  f->is_used = 1;
  if (first_save) {
    f->snapshot_tag[0] = '\0';
  }

  int px = t->target_pixel;
  for (int n = 0; n < 5; n++) {
    f->voltages[n] = os_memory_map[px].neurons[n].voltage;
    f->weights[n] = os_memory_map[px].neurons[n].synaptic_weight;
    f->thresholds[n] = os_memory_map[px].neurons[n].dynamic_threshold;
  }

  __asm__ volatile("" ::: "memory");

  if (ata_disk_available) {
    uint16_t sector_buf[256];
    for (int i = 0; i < 256; i++) {
      sector_buf[i] = 0;
    }
    copy_bytes((void *)sector_buf, (const void *)f, (int)sizeof(VirtualFile));
    disk_write_sector(DISK_DATA_OFFSET + slot, sector_buf);
    klog_info("snapshot persisted to disk");
  } else {
    klog_warn("disk unavailable, snapshot kept in memory");
  }
}

int sys_load_task(int task_id, int slot) {
  if (slot < 0 || slot >= SNAPSHOT_SLOTS)
    return 0;

  if (ata_disk_available) {
    uint16_t sector_buf[256];
    disk_read_sector(DISK_DATA_OFFSET + slot, sector_buf);
    copy_bytes((void *)&synapse_disk[slot], (const void *)sector_buf,
               (int)sizeof(VirtualFile));
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

int storage_snapshot_used_count(void) {
  int used = 0;
  for (int i = 0; i < SNAPSHOT_SLOTS; i++) {
    if (synapse_disk[i].is_used) {
      used++;
    }
  }
  return used;
}

int storage_snapshot_capacity(void) {
  return SNAPSHOT_SLOTS;
}

int storage_set_snapshot_tag(int slot, const char *tag) {
  if (slot < 0 || slot >= SNAPSHOT_SLOTS || tag == 0 || tag[0] == '\0') {
    return 0;
  }
  if (!synapse_disk[slot].is_used) {
    return 0;
  }

  copy_text((char *)synapse_disk[slot].snapshot_tag, tag, SNAPSHOT_TAG_LEN);

  if (ata_disk_available) {
    uint16_t sector_buf[256];
    for (int i = 0; i < 256; i++) {
      sector_buf[i] = 0;
    }
    copy_bytes((void *)sector_buf, (const void *)&synapse_disk[slot],
               (int)sizeof(VirtualFile));
    disk_write_sector(DISK_DATA_OFFSET + slot, sector_buf);
  }

  return 1;
}

int storage_get_snapshot_tag(int slot, char *out, int out_len) {
  if (slot < 0 || slot >= SNAPSHOT_SLOTS || out == 0 || out_len <= 0) {
    return 0;
  }
  if (!synapse_disk[slot].is_used) {
    out[0] = '\0';
    return 0;
  }

  copy_text(out, (const char *)synapse_disk[slot].snapshot_tag, out_len);
  return 1;
}

int storage_get_snapshot_signature(int slot, int *voltage_sum, int *weight_sum,
                                   int *threshold_sum) {
  int v = 0;
  int w = 0;
  int t = 0;

  if (slot < 0 || slot >= SNAPSHOT_SLOTS || voltage_sum == 0 || weight_sum == 0 ||
      threshold_sum == 0) {
    return 0;
  }
  if (!synapse_disk[slot].is_used) {
    return 0;
  }

  for (int i = 0; i < 5; i++) {
    v += synapse_disk[slot].voltages[i];
    w += synapse_disk[slot].weights[i];
    t += synapse_disk[slot].thresholds[i];
  }

  *voltage_sum = v;
  *weight_sum = w;
  *threshold_sum = t;
  return 1;
}

int storage_diff_snapshots(int slot_a, int slot_b, int *voltage_delta,
                           int *weight_delta, int *threshold_delta) {
  int av = 0, aw = 0, at = 0;
  int bv = 0, bw = 0, bt = 0;

  if (voltage_delta == 0 || weight_delta == 0 || threshold_delta == 0) {
    return 0;
  }

  if (!storage_get_snapshot_signature(slot_a, &av, &aw, &at) ||
      !storage_get_snapshot_signature(slot_b, &bv, &bw, &bt)) {
    return 0;
  }

  *voltage_delta = av - bv;
  *weight_delta = aw - bw;
  *threshold_delta = at - bt;
  return 1;
}
