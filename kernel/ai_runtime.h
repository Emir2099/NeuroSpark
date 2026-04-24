#ifndef AI_RUNTIME_H
#define AI_RUNTIME_H

#ifndef _UINT32_T_DEFINED
#define _UINT32_T_DEFINED
typedef unsigned int uint32_t;
#endif

typedef struct {
  uint32_t id;
  uint32_t active;
  uint32_t schema_version;
  uint32_t kind;
  uint32_t version;
  uint32_t checksum;
  uint32_t artifact_hash;
  uint32_t package_bytes;
  uint32_t provenance;
  uint32_t preferred_backend;
  uint32_t seed;
  int bias;
  char source_path[48];
  char name[16];
} AiModelInfo;

typedef struct {
  uint32_t model_count;
  uint32_t active_backend;
  uint32_t load_ok;
  uint32_t load_fail;
  uint32_t verify_ok;
  uint32_t verify_fail;
  uint32_t infer_ok;
  uint32_t infer_fail;
  uint32_t last_hash;
} AiRuntimeStats;

typedef struct __attribute__((packed)) {
  uint32_t magic;
  uint32_t schema_version;
  uint32_t header_size;
  uint32_t package_size;
  uint32_t model_kind;
  uint32_t preferred_backend;
  uint32_t model_version;
  uint32_t seed;
  uint32_t provenance;
  int bias;
  uint32_t payload_bytes;
  uint32_t checksum;
  char model_name[16];
  char source_label[16];
  char reserved[16];
} AiModelPackageHeaderV1;

typedef struct {
  uint32_t schema_version;
  uint32_t package_bytes;
  uint32_t payload_bytes;
  uint32_t checksum;
  uint32_t artifact_hash;
  uint32_t kind;
  uint32_t preferred_backend;
  uint32_t version;
  uint32_t seed;
  uint32_t provenance;
  int bias;
  char name[16];
  char source_path[48];
} AiModelPackageInfo;

enum {
  AI_MODEL_KIND_ANN = 1,
  AI_MODEL_KIND_SNN = 2
};

enum {
  AI_BACKEND_CPU = 0,
  AI_BACKEND_NEURO = 1
};

#define AI_MODEL_PACKAGE_MAGIC 0x314D4941u /* "AIM1" */
#define AI_MODEL_PACKAGE_SCHEMA_V1 1u
#define AI_MODEL_PACKAGE_MAX_BYTES (64u * 1024u * 1024u)

void ai_runtime_init(void);
int ai_runtime_load_model(const char *name, uint32_t kind, uint32_t version,
                          uint32_t preferred_backend, uint32_t seed,
                          uint32_t provenance);
int ai_runtime_import_model_package(const char *path, const char *alias,
                                    AiModelPackageInfo *out_info);
int ai_runtime_verify_model(const char *name, uint32_t *out_checksum);
int ai_runtime_list_models(AiModelInfo *out_models, int max_models);
int ai_runtime_set_backend(uint32_t backend);
uint32_t ai_runtime_get_backend(void);
int ai_runtime_infer(const char *name, int input_value, uint32_t seed,
                     int *out_value);
int ai_runtime_get_model_bias(const char *name, int *out_bias);
int ai_runtime_set_model_bias(const char *name, int bias);
void ai_runtime_get_stats(AiRuntimeStats *out_stats);

#endif