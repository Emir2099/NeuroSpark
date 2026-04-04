#include "storage_manager.h"
#include "disk.h"
#include "klog.h"
#include "vfs.h"
#include "model_manager.h"

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;

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

#define SNAPSHOT_SLOTS 4
#define SNAPSHOT_TAG_LEN 16
#define MANIFEST_MAGIC 0x4D463838 /* MF88 */
#define DATASET_MAGIC 0x44533838  /* DS88 */

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t tick;
  int zoom_level;
  int zoom_offset;
  int task_base_integration[2];
  int task_fire_threshold[2];
  int task_target_pixel[2];
  int potentials[16];
  uint32_t snapshot_used_mask;
  char lineage_tags[SNAPSHOT_SLOTS][SNAPSHOT_TAG_LEN];
} SessionManifestV1;

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t tick;
  uint32_t checksum;
  int zoom_level;
  int zoom_offset;
  int task_base_integration[2];
  int task_fire_threshold[2];
  int task_target_pixel[2];
  int potentials[16];
  uint32_t snapshot_used_mask;
  char lineage_tags[SNAPSHOT_SLOTS][SNAPSHOT_TAG_LEN];
  ModelManifestData model_data;
} SessionManifest;

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t slots;
  VirtualFile files[SNAPSHOT_SLOTS];
} DatasetBlob;

extern volatile VirtualFile synapse_disk[SNAPSHOT_SLOTS];
extern TaskControlBlock task_list[2];
extern NeuralPixel os_memory_map[2];
extern uint32_t tick;
extern int zoom_level;
extern int zoom_offset;
extern int potentials[16];

static uint32_t manifest_checksum(const void *data, int len) {
  const uint8_t *bytes = (const uint8_t *)data;
  uint32_t sum = 0;

  for (int i = 0; i < len; i++) {
    sum = (sum << 5) | (sum >> 27);
    sum ^= bytes[i];
  }

  return sum;
}

static void build_backup_path(const char *path, char *out, int out_len) {
  int i = 0;

  if (out == 0 || out_len <= 0) {
    return;
  }
  if (path == 0 || path[0] == '\0') {
    out[0] = '\0';
    return;
  }

  while (path[i] && i < out_len - 5) {
    out[i] = path[i];
    i++;
  }
  out[i++] = '.';
  out[i++] = 'b';
  out[i++] = 'a';
  out[i++] = 'k';
  out[i] = '\0';
}

static int manifest_blob_valid(const SessionManifest *mf, int bytes_read) {
  const SessionManifestV1 *mf_v1 = (const SessionManifestV1 *)mf;

  if (bytes_read < (int)sizeof(SessionManifestV1)) {
    return 0;
  }
  if (mf_v1->magic != MANIFEST_MAGIC) {
    return 0;
  }
  if (mf_v1->version != 1 && mf_v1->version != 2 && mf_v1->version != 3) {
    return 0;
  }
  if (mf_v1->version == 3) {
    if (bytes_read < (int)sizeof(SessionManifest)) {
      return 0;
    }
    SessionManifest tmp = *mf;
    uint32_t expected = tmp.checksum;
    tmp.checksum = 0;
    if (expected != manifest_checksum(&tmp, (int)sizeof(tmp))) {
      return 0;
    }
  }
  return 1;
}

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

static void append_text(char *out, int out_len, int *cursor, const char *text) {
  int i = 0;
  if (out == 0 || cursor == 0 || text == 0 || out_len <= 0)
    return;
  while (text[i] && *cursor < out_len - 1) {
    out[*cursor] = text[i];
    (*cursor)++;
    i++;
  }
  out[*cursor] = '\0';
}

static void append_dec(char *out, int out_len, int *cursor, int value) {
  char tmp[16];
  int i = 0;
  int neg = 0;

  if (value == 0) {
    append_text(out, out_len, cursor, "0");
    return;
  }
  if (value < 0) {
    neg = 1;
    value = -value;
  }
  while (value > 0 && i < 15) {
    tmp[i++] = (char)('0' + (value % 10));
    value /= 10;
  }
  if (neg && i < 15) {
    tmp[i++] = '-';
  }
  while (i > 0) {
    char c[2];
    c[0] = tmp[--i];
    c[1] = '\0';
    append_text(out, out_len, cursor, c);
  }
}

void sys_save_task(int task_id, int slot) {
  if (slot < 0 || slot >= SNAPSHOT_SLOTS || task_id < 0 || task_id > 1)
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

  {
    int px = t->target_pixel;
    for (int n = 0; n < 5; n++) {
      f->voltages[n] = os_memory_map[px].neurons[n].voltage;
      f->weights[n] = os_memory_map[px].neurons[n].synaptic_weight;
      f->thresholds[n] = os_memory_map[px].neurons[n].dynamic_threshold;
    }
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
  if (slot < 0 || slot >= SNAPSHOT_SLOTS || task_id < 0 || task_id > 1)
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

  {
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

int storage_snapshot_capacity(void) { return SNAPSHOT_SLOTS; }

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

int storage_diff_snapshots_vector(int slot_a, int slot_b, int *voltage_delta_vec,
                                  int *weight_delta_vec, int *threshold_delta_vec,
                                  int vec_len) {
  if (slot_a < 0 || slot_b < 0 || slot_a >= SNAPSHOT_SLOTS || slot_b >= SNAPSHOT_SLOTS) {
    return 0;
  }
  if (vec_len < 5 || voltage_delta_vec == 0 || weight_delta_vec == 0 ||
      threshold_delta_vec == 0) {
    return 0;
  }
  if (!synapse_disk[slot_a].is_used || !synapse_disk[slot_b].is_used) {
    return 0;
  }

  for (int i = 0; i < 5; i++) {
    voltage_delta_vec[i] = synapse_disk[slot_a].voltages[i] - synapse_disk[slot_b].voltages[i];
    weight_delta_vec[i] = synapse_disk[slot_a].weights[i] - synapse_disk[slot_b].weights[i];
    threshold_delta_vec[i] = synapse_disk[slot_a].thresholds[i] - synapse_disk[slot_b].thresholds[i];
  }
  return 1;
}

int storage_manifest_save(const char *path) {
  SessionManifest mf;
  char backup_path[80];

  if (path == 0 || path[0] == '\0')
    return 0;

  mf.magic = MANIFEST_MAGIC;
  mf.version = 2;
  mf.tick = tick;
  mf.zoom_level = zoom_level;
  mf.zoom_offset = zoom_offset;

  for (int t = 0; t < 2; t++) {
    mf.task_base_integration[t] = task_list[t].base_integration;
    mf.task_fire_threshold[t] = task_list[t].fire_threshold;
    mf.task_target_pixel[t] = task_list[t].target_pixel;
  }

  for (int i = 0; i < 16; i++) {
    mf.potentials[i] = potentials[i];
  }

  mf.snapshot_used_mask = 0;
  for (int s = 0; s < SNAPSHOT_SLOTS; s++) {
    if (synapse_disk[s].is_used) {
      mf.snapshot_used_mask |= (1u << s);
    }
    copy_text(mf.lineage_tags[s], (const char *)synapse_disk[s].snapshot_tag,
              SNAPSHOT_TAG_LEN);
  }

  model_manifest_capture(&mf.model_data);
  mf.version = 3;
  mf.checksum = 0;
  mf.checksum = manifest_checksum(&mf, (int)sizeof(mf));

  build_backup_path(path, backup_path, sizeof(backup_path));
  if (backup_path[0] != '\0' && vfs_write_file(backup_path, &mf, sizeof(mf)) != (int)sizeof(mf)) {
    return 0;
  }

  return vfs_write_file(path, &mf, sizeof(mf)) == (int)sizeof(mf);
}

int storage_manifest_load(const char *path) {
  SessionManifest mf;
  SessionManifest mf_bak;
  int n;
  int n_bak = 0;
  SessionManifestV1 *mf_v1 = (SessionManifestV1 *)&mf;
  char backup_path[80];

  if (path == 0 || path[0] == '\0')
    return 0;

  build_backup_path(path, backup_path, sizeof(backup_path));

  n = vfs_read_file(path, &mf, sizeof(mf));
  if (!manifest_blob_valid(&mf, n) && backup_path[0] != '\0') {
    n_bak = vfs_read_file(backup_path, &mf_bak, sizeof(mf_bak));
    if (manifest_blob_valid(&mf_bak, n_bak)) {
      mf = mf_bak;
      n = n_bak;
    }
  }

  if (!manifest_blob_valid(&mf, n))
    return 0;

  zoom_level = mf_v1->zoom_level;
  if (zoom_level < 1)
    zoom_level = 1;
  if (zoom_level > 4)
    zoom_level = 4;

  zoom_offset = mf_v1->zoom_offset;
  if (zoom_offset < 0)
    zoom_offset = 0;
  if (zoom_offset > 15)
    zoom_offset = 15;

  for (int t = 0; t < 2; t++) {
    task_list[t].base_integration = mf_v1->task_base_integration[t];
    task_list[t].fire_threshold = mf_v1->task_fire_threshold[t];
    task_list[t].target_pixel = mf_v1->task_target_pixel[t];
  }

  for (int i = 0; i < 16; i++) {
    potentials[i] = mf_v1->potentials[i];
  }

  for (int s = 0; s < SNAPSHOT_SLOTS; s++) {
    copy_text((char *)synapse_disk[s].snapshot_tag, mf_v1->lineage_tags[s],
              SNAPSHOT_TAG_LEN);
  }

  if (mf_v1->version >= 2 && n >= (int)sizeof(SessionManifestV1)) {
    model_manifest_apply(&mf.model_data);
  }

  return 1;
}

int storage_manifest_summary(char *out, int out_len) {
  int cursor = 0;
  int used = storage_snapshot_used_count();

  if (out == 0 || out_len <= 0)
    return 0;

  out[0] = '\0';
  append_text(out, out_len, &cursor, "tick=");
  append_dec(out, out_len, &cursor, (int)tick);
  append_text(out, out_len, &cursor, " zoom=");
  append_dec(out, out_len, &cursor, zoom_level);
  append_text(out, out_len, &cursor, "x@");
  append_dec(out, out_len, &cursor, zoom_offset);
  append_text(out, out_len, &cursor, " snaps=");
  append_dec(out, out_len, &cursor, used);
  append_text(out, out_len, &cursor, "/");
  append_dec(out, out_len, &cursor, storage_snapshot_capacity());
  append_text(out, out_len, &cursor, " lineage=");

  for (int s = 0; s < SNAPSHOT_SLOTS; s++) {
    if (!synapse_disk[s].is_used)
      continue;
    append_text(out, out_len, &cursor, "S");
    append_dec(out, out_len, &cursor, s);
    append_text(out, out_len, &cursor, "[");
    if (synapse_disk[s].snapshot_tag[0]) {
      append_text(out, out_len, &cursor,
                  (const char *)synapse_disk[s].snapshot_tag);
    } else {
      append_text(out, out_len, &cursor, "-");
    }
    append_text(out, out_len, &cursor, "]");
  }

  append_text(out, out_len, &cursor, " model=");
  append_text(out, out_len, &cursor, model_name(0));
  append_text(out, out_len, &cursor, "/");
  append_text(out, out_len, &cursor, model_name(1));
  append_text(out, out_len, &cursor, " stdp=");
  append_text(out, out_len, &cursor, model_get_stdp() ? "on" : "off");

  return 1;
}

int storage_dataset_export(const char *path) {
  DatasetBlob blob;

  if (path == 0 || path[0] == '\0')
    return 0;

  blob.magic = DATASET_MAGIC;
  blob.version = 1;
  blob.slots = SNAPSHOT_SLOTS;

  for (int s = 0; s < SNAPSHOT_SLOTS; s++) {
    copy_bytes(&blob.files[s], (const void *)&synapse_disk[s],
               (int)sizeof(VirtualFile));
  }

  return vfs_write_file(path, &blob, sizeof(blob)) == (int)sizeof(blob);
}

int storage_dataset_import(const char *path) {
  DatasetBlob blob;
  int n;

  if (path == 0 || path[0] == '\0')
    return 0;

  n = vfs_read_file(path, &blob, sizeof(blob));
  if (n < (int)sizeof(blob))
    return 0;
  if (blob.magic != DATASET_MAGIC || blob.version != 1 ||
      blob.slots != SNAPSHOT_SLOTS) {
    return 0;
  }

  for (int s = 0; s < SNAPSHOT_SLOTS; s++) {
    copy_bytes((void *)&synapse_disk[s], &blob.files[s], (int)sizeof(VirtualFile));

    if (ata_disk_available) {
      uint16_t sector_buf[256];
      for (int i = 0; i < 256; i++) {
        sector_buf[i] = 0;
      }
      copy_bytes((void *)sector_buf, (const void *)&synapse_disk[s],
                 (int)sizeof(VirtualFile));
      disk_write_sector(DISK_DATA_OFFSET + s, sector_buf);
    }
  }

  return 1;
}
