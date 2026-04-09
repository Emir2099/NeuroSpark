#ifndef MODULE_LOADER_H
#define MODULE_LOADER_H

typedef unsigned int uint32_t;

typedef struct {
  int used;
  int kind;
  int handle;
  int ref_count;
  int exported_symbols;
  char path[64];
} NsModuleInfo;

enum {
  NS_MODULE_KIND_LIBRARY = 1,
  NS_MODULE_KIND_KERNEL = 2,
};

void module_loader_init(void);

int module_dlopen(const char *path);
int module_dlclose(int handle);
uint32_t module_dlsym(int handle, const char *symbol);
const char *module_dlerror(void);

int module_dlopen_for_task(int task_id, const char *path);
int module_dlclose_for_task(int task_id, int handle);
const char *module_dlerror_for_task(int task_id);
int module_copy_dlerror_for_task(int task_id, char *out, uint32_t out_cap);

int module_resolve_symbol_global(const char *symbol, uint32_t *addr_out);

int module_insmod(const char *path);
int module_rmmod(const char *path_or_handle);

int module_list(NsModuleInfo *out, int max_entries);

#endif
