#include "ai_runtime.h"

#include "vfs.h"

#define AI_MAX_MODELS 8

typedef unsigned char uint8_t;

static AiModelInfo g_models[AI_MAX_MODELS];
static AiRuntimeStats g_stats;

typedef int (*AiBackendInferFn)(const AiModelInfo *m, int input_value,
                                uint32_t seed, int *out_value,
                                uint32_t *out_hash);

typedef struct {
  uint32_t id;
  uint32_t capability_flags;
  AiBackendInferFn infer;
} AiBackendAdapter;

#define AI_BACKEND_CAP_DETERMINISTIC 0x1u
#define AI_BACKEND_CAP_SPIKE_NATIVE 0x2u

static void copy_name(char *dst, int dst_len, const char *src);
static int name_valid(const char *name);
static uint32_t mix_u32(uint32_t x);

static int cpu_backend_infer(const AiModelInfo *m, int input_value,
                             uint32_t seed, int *out_value,
                             uint32_t *out_hash);
static int neuro_backend_infer(const AiModelInfo *m, int input_value,
                               uint32_t seed, int *out_value,
                               uint32_t *out_hash);

static const AiBackendAdapter g_backends[] = {
    {AI_BACKEND_CPU, AI_BACKEND_CAP_DETERMINISTIC, cpu_backend_infer},
    {AI_BACKEND_NEURO,
     AI_BACKEND_CAP_DETERMINISTIC | AI_BACKEND_CAP_SPIKE_NATIVE,
     neuro_backend_infer},
};

static const AiBackendAdapter *resolve_backend(uint32_t backend_id) {
  uint32_t i;
  for (i = 0; i < (uint32_t)(sizeof(g_backends) / sizeof(g_backends[0])); i++) {
    if (g_backends[i].id == backend_id) {
      return &g_backends[i];
    }
  }
  return 0;
}

static int cpu_backend_infer(const AiModelInfo *m, int input_value,
                             uint32_t seed, int *out_value,
                             uint32_t *out_hash) {
  uint32_t s;
  int out;
  if (m == 0 || out_value == 0) {
    return 0;
  }
  s = seed ? seed : m->seed;
  s = mix_u32(s ^ (uint32_t)(input_value + 0x13579));
  s = mix_u32(s ^ m->checksum);
  s = mix_u32(s ^ m->artifact_hash);
  out = (int)(s % 2048u) + input_value + m->bias;
  if (m->kind == AI_MODEL_KIND_SNN) {
    out += (int)((s >> 8) & 0x3Fu);
  }
  *out_value = out;
  if (out_hash != 0) {
    *out_hash = s;
  }
  return 1;
}

static int neuro_backend_infer(const AiModelInfo *m, int input_value,
                               uint32_t seed, int *out_value,
                               uint32_t *out_hash) {
  uint32_t s;
  int out;
  if (m == 0 || out_value == 0) {
    return 0;
  }
  s = seed ? seed : m->seed;
  s = mix_u32(s ^ (uint32_t)(input_value + 0x2468Bu));
  s = mix_u32(s ^ (m->checksum << 1));
  s = mix_u32(s ^ m->artifact_hash ^ 0x5A5AA11u);
  out = (int)(s % 1024u) + (input_value * 2) + m->bias;
  out += (int)((s >> 7) & 0x7Fu);
  if (m->kind == AI_MODEL_KIND_SNN) {
    out += (int)((s >> 3) & 0x3Fu);
  }
  *out_value = out;
  if (out_hash != 0) {
    *out_hash = s;
  }
  return 1;
}

static uint32_t hash_init(void) {
  return 0xC0FFEE11u;
}

static uint32_t hash_update(uint32_t h, const uint8_t *data, uint32_t len) {
  uint32_t i;
  if (data == 0) {
    return h;
  }
  for (i = 0; i < len; i++) {
    h ^= (uint32_t)data[i];
    h = mix_u32(h + 0x9E3779B9u + (h << 6) + (h >> 2));
  }
  return h;
}

static int hash_stream_range(int fd, uint32_t start, uint32_t len,
                             uint32_t *out_hash) {
  uint8_t chunk[512];
  uint32_t h;
  uint32_t remaining;

  if (fd < 0 || out_hash == 0) {
    return 0;
  }
  if (vfs_lseek(fd, (int)start, VFS_SEEK_SET) < 0) {
    return 0;
  }

  h = hash_init();
  remaining = len;
  while (remaining > 0) {
    uint32_t want = remaining;
    int got;
    if (want > (uint32_t)sizeof(chunk)) {
      want = (uint32_t)sizeof(chunk);
    }
    got = vfs_read(fd, chunk, want);
    if (got != (int)want) {
      return 0;
    }
    h = hash_update(h, chunk, want);
    remaining -= want;
  }

  *out_hash = h;
  return 1;
}

static void derive_name_from_path(const char *path, char *out, int out_len) {
  const char *base = path;
  int i = 0;

  if (out == 0 || out_len <= 0) {
    return;
  }

  out[0] = '\0';
  if (path == 0 || path[0] == '\0') {
    copy_name(out, out_len, "model");
    return;
  }

  for (i = 0; path[i] != '\0'; i++) {
    if (path[i] == '/') {
      base = path + i + 1;
    }
  }

  i = 0;
  while (base[i] != '\0' && base[i] != '.' && i < out_len - 1) {
    char c = base[i];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '_' || c == '-')) {
      c = '_';
    }
    out[i] = c;
    i++;
  }
  out[i] = '\0';

  if (!name_valid(out)) {
    copy_name(out, out_len, "model");
  }
}

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

static void copy_path(char *dst, int dst_len, const char *src) {
  copy_name(dst, dst_len, src);
}

static uint32_t model_package_hash(const char *name, uint32_t kind,
                                   uint32_t version,
                                   uint32_t preferred_backend,
                                   uint32_t seed, uint32_t provenance,
                                   int bias, uint32_t payload_bytes,
                                   uint32_t artifact_hash,
                                   uint32_t schema_version,
                                   uint32_t package_bytes) {
  uint32_t h = 0x5A17C0DEu;
  int i = 0;

  while (name && name[i]) {
    h = mix_u32(h ^ (uint32_t)(unsigned char)name[i]);
    i++;
  }

  h = mix_u32(h ^ kind);
  h = mix_u32(h ^ version);
  h = mix_u32(h ^ preferred_backend);
  h = mix_u32(h ^ seed);
  h = mix_u32(h ^ provenance);
  h = mix_u32(h ^ (uint32_t)bias);
  h = mix_u32(h ^ payload_bytes);
  h = mix_u32(h ^ artifact_hash);
  h = mix_u32(h ^ schema_version);
  h = mix_u32(h ^ package_bytes);
  return h;
}

static int valid_package_name(const char *name) {
  return name_valid(name);
}

static int valid_backend(uint32_t backend) {
  return backend == AI_BACKEND_CPU || backend == AI_BACKEND_NEURO;
}

static int valid_kind(uint32_t kind) {
  return kind == AI_MODEL_KIND_ANN || kind == AI_MODEL_KIND_SNN;
}

static int store_model(const char *name, const char *source_path,
                      uint32_t schema_version, uint32_t kind,
                      uint32_t version, uint32_t preferred_backend,
                      uint32_t seed, uint32_t provenance, int bias,
                      uint32_t payload_bytes, uint32_t artifact_hash,
                      uint32_t package_bytes, uint32_t checksum) {
  int i;
  int idx = -1;
  AiModelInfo *m;

  if (!valid_package_name(name) || !valid_kind(kind) ||
      !valid_backend(preferred_backend)) {
    return 0;
  }

  for (i = 0; i < AI_MAX_MODELS; i++) {
    if (g_models[i].active && str_eq_local(g_models[i].name, name)) {
      idx = i;
      break;
    }
  }

  if (idx < 0) {
    for (i = 0; i < AI_MAX_MODELS; i++) {
      if (!g_models[i].active) {
        idx = i;
        break;
      }
    }
  }

  if (idx < 0) {
    return 0;
  }

  m = &g_models[idx];
  m->active = 1;
  m->schema_version = schema_version;
  m->kind = kind;
  m->version = version;
  m->checksum = checksum;
  m->artifact_hash = artifact_hash;
  m->package_bytes = package_bytes;
  m->provenance = provenance;
  m->preferred_backend = preferred_backend;
  m->seed = seed;
  m->bias = bias;
  copy_name(m->name, sizeof(m->name), name);
  copy_path(m->source_path, sizeof(m->source_path), source_path);

  if (m->bias < -100000 || m->bias > 100000) {
    m->bias = 0;
  }

  g_stats.load_ok++;
  g_stats.last_hash = m->checksum;

  g_stats.model_count = 0;
  for (i = 0; i < AI_MAX_MODELS; i++) {
    if (g_models[i].active) {
      g_stats.model_count++;
    }
  }

  return 1;
}

static int name_valid(const char *name) {
  int i = 0;
  if (name == 0 || name[0] == '\0') {
    return 0;
  }
  while (name[i]) {
    if (!((name[i] >= 'a' && name[i] <= 'z') ||
          (name[i] >= 'A' && name[i] <= 'Z') ||
          (name[i] >= '0' && name[i] <= '9') || name[i] == '_' ||
          name[i] == '-')) {
      return 0;
    }
    i++;
  }
  return 1;
}

static uint32_t mix_u32(uint32_t x) {
  x ^= x >> 16;
  x *= 0x7FEB352Du;
  x ^= x >> 15;
  x *= 0x846CA68Bu;
  x ^= x >> 16;
  return x;
}

static int find_model_index(const char *name) {
  int i;
  for (i = 0; i < AI_MAX_MODELS; i++) {
    if (g_models[i].active && str_eq_local(g_models[i].name, name)) {
      return i;
    }
  }
  return -1;
}

void ai_runtime_init(void) {
  int i;
  for (i = 0; i < AI_MAX_MODELS; i++) {
    g_models[i].active = 0;
    g_models[i].id = (uint32_t)i;
    g_models[i].schema_version = 0;
    g_models[i].name[0] = '\0';
    g_models[i].source_path[0] = '\0';
    g_models[i].bias = 0;
  }

  g_stats.model_count = 0;
  g_stats.active_backend = AI_BACKEND_CPU;
  g_stats.load_ok = 0;
  g_stats.load_fail = 0;
  g_stats.verify_ok = 0;
  g_stats.verify_fail = 0;
  g_stats.infer_ok = 0;
  g_stats.infer_fail = 0;
  g_stats.last_hash = 0;
}

int ai_runtime_load_model(const char *name, uint32_t kind, uint32_t version,
                          uint32_t preferred_backend, uint32_t seed,
                          uint32_t provenance) {
  uint32_t checksum;
  uint32_t artifact_hash;

  if (!name_valid(name) || !valid_kind(kind) || !valid_backend(preferred_backend)) {
    g_stats.load_fail++;
    return 0;
  }

  artifact_hash = mix_u32(seed ^ provenance ^ (version << 1) ^ kind ^ preferred_backend);
  checksum = model_package_hash(name, kind, version, preferred_backend, seed,
                                provenance, 0, 0u, artifact_hash, 0u, 0u);

  if (!store_model(name, "<memory>", 0u, kind, version, preferred_backend,
                   seed, provenance, 0, 0u, artifact_hash, 0u, checksum)) {
    g_stats.load_fail++;
    return 0;
  }

  return 1;
}

int ai_runtime_import_model_package(const char *path, const char *alias,
                                    AiModelPackageInfo *out_info) {
  VfsFileStat st;
  AiModelPackageHeaderV1 hdr;
  const AiModelPackageHeaderV1 *hdrp;
  uint32_t package_bytes;
  uint32_t header_bytes;
  uint32_t payload_bytes;
  uint32_t payload_hash;
  uint32_t package_hash;
  uint32_t expected_hash;
  uint32_t schema_version;
  uint32_t model_kind;
  uint32_t preferred_backend;
  uint32_t model_version;
  uint32_t seed;
  uint32_t provenance;
  int bias;
  int fd = -1;
  char name_buf[16];
  uint32_t i;

  if (out_info != 0) {
    out_info->schema_version = 0;
    out_info->package_bytes = 0;
    out_info->payload_bytes = 0;
    out_info->checksum = 0;
    out_info->artifact_hash = 0;
    out_info->kind = 0;
    out_info->preferred_backend = 0;
    out_info->version = 0;
    out_info->seed = 0;
    out_info->provenance = 0;
    out_info->bias = 0;
    out_info->name[0] = '\0';
    out_info->source_path[0] = '\0';
  }

  if (path == 0 || path[0] == '\0') {
    g_stats.load_fail++;
    return 0;
  }

  if (vfs_stat(path, &st) < 0 || st.size < sizeof(AiModelPackageHeaderV1) ||
      st.size > AI_MODEL_PACKAGE_MAX_BYTES) {
    g_stats.load_fail++;
    return 0;
  }

  package_bytes = st.size;
  fd = vfs_open(path, VFS_O_RDONLY);
  if (fd < 0) {
    g_stats.load_fail++;
    return 0;
  }

  if (vfs_read(fd, &hdr, (uint32_t)sizeof(hdr)) != (int)sizeof(hdr)) {
    g_stats.load_fail++;
    vfs_close(fd);
    return 0;
  }

  hdrp = &hdr;
  header_bytes = hdrp->header_size;
  payload_bytes = hdrp->payload_bytes;

  if (hdrp->magic == AI_MODEL_PACKAGE_MAGIC &&
      hdrp->schema_version == AI_MODEL_PACKAGE_SCHEMA_V1 &&
      header_bytes == sizeof(AiModelPackageHeaderV1) &&
      hdrp->package_size == package_bytes &&
      valid_kind(hdrp->model_kind) && valid_backend(hdrp->preferred_backend) &&
      payload_bytes <= (package_bytes - header_bytes)) {
    schema_version = hdrp->schema_version;
    model_kind = hdrp->model_kind;
    preferred_backend = hdrp->preferred_backend;
    model_version = hdrp->model_version;
    seed = hdrp->seed;
    provenance = hdrp->provenance;
    bias = hdrp->bias;

    if (!hash_stream_range(fd, header_bytes, payload_bytes, &payload_hash)) {
      g_stats.load_fail++;
      vfs_close(fd);
      return 0;
    }

    for (i = 0; i < sizeof(name_buf); i++) {
      name_buf[i] = '\0';
    }
    copy_name(name_buf, sizeof(name_buf), hdrp->model_name);
    if (alias != 0 && alias[0] != '\0') {
      copy_name(name_buf, sizeof(name_buf), alias);
    }

    if (!valid_package_name(name_buf)) {
      g_stats.load_fail++;
      vfs_close(fd);
      return 0;
    }

    expected_hash = model_package_hash(name_buf, model_kind,
                                       model_version,
                                       preferred_backend, seed,
                                       provenance, bias, payload_bytes,
                                       payload_hash, schema_version,
                                       package_bytes);

    package_hash = hdrp->checksum;
    if (package_hash != 0 && package_hash != expected_hash) {
      g_stats.load_fail++;
      vfs_close(fd);
      return 0;
    }
  } else {
    schema_version = 0;
    model_kind = AI_MODEL_KIND_ANN;
    preferred_backend = AI_BACKEND_CPU;
    model_version = 1;
    seed = package_bytes;
    provenance = mix_u32(package_bytes ^ 0xA11C0DEu);
    bias = 0;
    payload_bytes = package_bytes;

    if (!hash_stream_range(fd, 0u, package_bytes, &payload_hash)) {
      g_stats.load_fail++;
      vfs_close(fd);
      return 0;
    }

    if (alias != 0 && alias[0] != '\0') {
      copy_name(name_buf, sizeof(name_buf), alias);
    } else {
      derive_name_from_path(path, name_buf, sizeof(name_buf));
    }

    if (!valid_package_name(name_buf)) {
      g_stats.load_fail++;
      vfs_close(fd);
      return 0;
    }

    expected_hash = model_package_hash(name_buf, model_kind,
                                       model_version,
                                       preferred_backend, seed,
                                       provenance, bias, payload_bytes,
                                       payload_hash, schema_version,
                                       package_bytes);
  }

  vfs_close(fd);

  if (!store_model(name_buf, path, schema_version, model_kind,
                   model_version, preferred_backend, seed,
                   provenance, bias, payload_bytes, payload_hash,
                   package_bytes, expected_hash)) {
    g_stats.load_fail++;
    return 0;
  }

  if (out_info != 0) {
    out_info->schema_version = schema_version;
    out_info->package_bytes = package_bytes;
    out_info->payload_bytes = payload_bytes;
    out_info->checksum = expected_hash;
    out_info->artifact_hash = payload_hash;
    out_info->kind = model_kind;
    out_info->preferred_backend = preferred_backend;
    out_info->version = model_version;
    out_info->seed = seed;
    out_info->provenance = provenance;
    out_info->bias = bias;
    copy_name(out_info->name, sizeof(out_info->name), name_buf);
    copy_path(out_info->source_path, sizeof(out_info->source_path), path);
  }

  return 1;
}

int ai_runtime_verify_model(const char *name, uint32_t *out_checksum) {
  int idx = find_model_index(name);
  AiModelInfo *m;
  uint32_t computed;

  if (idx < 0) {
    g_stats.verify_fail++;
    return 0;
  }

  m = &g_models[idx];
  computed = model_package_hash(m->name, m->kind, m->version,
                                m->preferred_backend, m->seed, m->provenance,
                                m->bias, m->package_bytes, m->artifact_hash,
                                m->schema_version, m->package_bytes);
  if (computed != m->checksum) {
    g_stats.verify_fail++;
    return 0;
  }

  if (out_checksum != 0) {
    *out_checksum = computed;
  }
  g_stats.verify_ok++;
  g_stats.last_hash = computed;
  return 1;
}

int ai_runtime_list_models(AiModelInfo *out_models, int max_models) {
  int i;
  int copied = 0;

  if (out_models == 0 || max_models <= 0) {
    return 0;
  }

  for (i = 0; i < AI_MAX_MODELS && copied < max_models; i++) {
    if (!g_models[i].active) {
      continue;
    }
    out_models[copied++] = g_models[i];
  }

  return copied;
}

int ai_runtime_set_backend(uint32_t backend) {
  if (backend != AI_BACKEND_CPU && backend != AI_BACKEND_NEURO) {
    return 0;
  }
  g_stats.active_backend = backend;
  return 1;
}

uint32_t ai_runtime_get_backend(void) { return g_stats.active_backend; }

int ai_runtime_infer(const char *name, int input_value, uint32_t seed,
                     int *out_value) {
  int idx = find_model_index(name);
  AiModelInfo *m;
  uint32_t s = 0;
  const AiBackendAdapter *backend;

  if (idx < 0 || out_value == 0) {
    g_stats.infer_fail++;
    return 0;
  }

  m = &g_models[idx];
  backend = resolve_backend(g_stats.active_backend);
  if (backend == 0 || backend->infer == 0) {
    g_stats.infer_fail++;
    return 0;
  }

  if (!backend->infer(m, input_value, seed, out_value, &s)) {
    g_stats.infer_fail++;
    return 0;
  }

  g_stats.infer_ok++;
  g_stats.last_hash = s;
  return 1;
}

int ai_runtime_get_model_bias(const char *name, int *out_bias) {
  int idx = find_model_index(name);
  if (idx < 0 || out_bias == 0) {
    return 0;
  }
  *out_bias = g_models[idx].bias;
  return 1;
}

int ai_runtime_set_model_bias(const char *name, int bias) {
  int idx = find_model_index(name);
  if (idx < 0) {
    return 0;
  }
  if (bias < -100000) {
    bias = -100000;
  }
  if (bias > 100000) {
    bias = 100000;
  }
  g_models[idx].bias = bias;
  return 1;
}

void ai_runtime_get_stats(AiRuntimeStats *out_stats) {
  if (out_stats == 0) {
    return;
  }
  *out_stats = g_stats;
}
