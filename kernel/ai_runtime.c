#include "ai_runtime.h"

#define AI_MAX_MODELS 8

static AiModelInfo g_models[AI_MAX_MODELS];
static AiRuntimeStats g_stats;

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

static uint32_t model_hash(const char *name, uint32_t kind, uint32_t version,
                           uint32_t preferred_backend, uint32_t seed,
                           uint32_t provenance) {
  uint32_t h = 0xA15E3201u;
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
  return h;
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
    g_models[i].name[0] = '\0';
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
  int i;
  int idx;
  AiModelInfo *m;

  if (!name_valid(name) || (kind != AI_MODEL_KIND_ANN && kind != AI_MODEL_KIND_SNN) ||
      (preferred_backend != AI_BACKEND_CPU && preferred_backend != AI_BACKEND_NEURO)) {
    g_stats.load_fail++;
    return 0;
  }

  idx = find_model_index(name);
  if (idx < 0) {
    for (i = 0; i < AI_MAX_MODELS; i++) {
      if (!g_models[i].active) {
        idx = i;
        break;
      }
    }
  }

  if (idx < 0) {
    g_stats.load_fail++;
    return 0;
  }

  m = &g_models[idx];
  m->active = 1;
  m->kind = kind;
  m->version = version;
  m->preferred_backend = preferred_backend;
  m->seed = seed;
  m->provenance = provenance;
  m->checksum = model_hash(name, kind, version, preferred_backend, seed, provenance);
  copy_name(m->name, sizeof(m->name), name);

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

int ai_runtime_verify_model(const char *name, uint32_t *out_checksum) {
  int idx = find_model_index(name);
  AiModelInfo *m;
  uint32_t computed;

  if (idx < 0) {
    g_stats.verify_fail++;
    return 0;
  }

  m = &g_models[idx];
  computed = model_hash(m->name, m->kind, m->version, m->preferred_backend,
                        m->seed, m->provenance);
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
  uint32_t s;
  int out;

  if (idx < 0 || out_value == 0) {
    g_stats.infer_fail++;
    return 0;
  }

  m = &g_models[idx];
  s = seed ? seed : m->seed;
  s = mix_u32(s ^ (uint32_t)(input_value + 0x13579));
  s = mix_u32(s ^ m->checksum);

  if (g_stats.active_backend == AI_BACKEND_CPU) {
    out = (int)(s % 2048u) + input_value + m->bias;
  } else {
    out = (int)(s % 1024u) + (input_value * 2) + m->bias;
  }

  if (m->kind == AI_MODEL_KIND_SNN) {
    out += (int)((s >> 8) & 0x3Fu);
  }

  *out_value = out;
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
