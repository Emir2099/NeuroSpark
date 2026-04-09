#include "module_loader.h"

#include "task.h"
#include "vfs.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;

typedef struct {
  uint8_t e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry;
  uint32_t e_phoff;
  uint32_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} __attribute__((packed)) Elf32_Ehdr;

typedef struct {
  uint32_t sh_name;
  uint32_t sh_type;
  uint32_t sh_flags;
  uint32_t sh_addr;
  uint32_t sh_offset;
  uint32_t sh_size;
  uint32_t sh_link;
  uint32_t sh_info;
  uint32_t sh_addralign;
  uint32_t sh_entsize;
} __attribute__((packed)) Elf32_Shdr;

typedef struct {
  uint32_t st_name;
  uint32_t st_value;
  uint32_t st_size;
  uint8_t st_info;
  uint8_t st_other;
  uint16_t st_shndx;
} __attribute__((packed)) Elf32_Sym;

typedef struct {
  uint32_t r_offset;
  uint32_t r_info;
} __attribute__((packed)) Elf32_Rel;

typedef struct {
  uint32_t r_offset;
  uint32_t r_info;
  int32_t r_addend;
} __attribute__((packed)) Elf32_Rela;

typedef int (*module_entry_t)(void);

typedef struct {
  char name[48];
  uint32_t addr;
} NsExport;

typedef struct {
  int used;
  int kind;
  int handle;
  int ref_count;
  int export_count;
  int init_called;
  int kernel_refs;
  uint32_t image_base;
  uint32_t init_addr;
  uint32_t fini_addr;
  int dep_count;
  int deps[8];
  int task_refs[MAX_TASKS];
  char path[64];
  NsExport exports[64];
} NsLoadedObject;

typedef struct {
  char name[48];
  uint32_t addr;
} NsKernelExport;

typedef struct {
  int used;
  int target_slot;
  uint32_t place;
  uint32_t addend;
  uint32_t rel_type;
  char symbol[48];
} NsPendingReloc;

#define NS_MODULE_MAX 12
#define NS_KERNEL_EXPORTS_MAX 64
#define NS_MODULE_POOL_SIZE (256 * 1024)
#define NS_MODULE_LOAD_LIMIT (64 * 1024)
#define NS_MODULE_MAX_SECTIONS 96
#define NS_MODULE_ABI_MAGIC 0x4E534D31u
#define NS_DLERROR_LEN 96
#define NS_PENDING_RELOC_MAX 192

#define ELF_ET_REL 1
#define ELF_ET_DYN 3
#define ELF_EM_386 3

#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_DYNSYM 11

#define SHF_ALLOC 0x2u

#define SHN_UNDEF 0

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2

#define ELF32_ST_BIND(v) ((v) >> 4)
#define ELF32_R_SYM(v) ((v) >> 8)
#define ELF32_R_TYPE(v) ((v)&0xFF)

#define R_386_NONE 0
#define R_386_32 1
#define R_386_PC32 2
#define R_386_GLOB_DAT 6
#define R_386_JMP_SLOT 7
#define R_386_RELATIVE 8

extern int os_current_task;
extern void gprint(char *str, uint32_t color);
extern void set_cmd_output(const char *text);
extern int vfs_open(const char *path, int flags);
extern int vfs_close(int fd);
extern int vfs_read(int fd, void *buf, uint32_t size);
extern int vfs_write(int fd, const void *buf, uint32_t size);
extern int vfs_read_file(const char *path, void *buf, uint32_t max_size);
extern int vfs_write_file(const char *path, const void *buf, uint32_t size);
extern int vfs_stat(const char *path, VfsFileStat *stat_out);

static NsLoadedObject g_modules[NS_MODULE_MAX];
static NsKernelExport g_kernel_exports[NS_KERNEL_EXPORTS_MAX];
static NsPendingReloc g_pending_relocs[NS_PENDING_RELOC_MAX];
static char g_dlerror[MAX_TASKS][NS_DLERROR_LEN];
static int g_kernel_export_count = 0;

static uint8_t g_module_pool[NS_MODULE_POOL_SIZE];
static uint32_t g_module_pool_cursor = 0;

static uint8_t g_load_buf[NS_MODULE_LOAD_LIMIT];

static int text_copy(char *dst, int cap, const char *src);

static int current_task_slot(void) {
  if (os_current_task < 0 || os_current_task >= MAX_TASKS) {
    return 0;
  }
  return os_current_task;
}

static void set_dlerror_for_task(int task_id, const char *msg) {
  if (task_id < 0 || task_id >= MAX_TASKS || msg == 0) {
    return;
  }
  (void)text_copy(g_dlerror[task_id], NS_DLERROR_LEN, msg);
}

static void clear_dlerror_for_task(int task_id) {
  if (task_id < 0 || task_id >= MAX_TASKS) {
    return;
  }
  g_dlerror[task_id][0] = '\0';
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

static int parse_u32_dec_local(const char *s, uint32_t *out) {
  uint32_t v = 0;
  int i = 0;
  if (s == 0 || s[0] == '\0' || out == 0) {
    return 0;
  }
  while (s[i]) {
    if (s[i] < '0' || s[i] > '9') {
      return 0;
    }
    v = (v * 10u) + (uint32_t)(s[i] - '0');
    i++;
  }
  *out = v;
  return 1;
}

static int text_copy(char *dst, int cap, const char *src) {
  int i = 0;
  if (dst == 0 || src == 0 || cap <= 0) {
    return 0;
  }
  while (src[i] && i < cap - 1) {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
  return i;
}

static uint32_t align_up_u32(uint32_t value, uint32_t align) {
  if (align == 0u || align == 1u) {
    return value;
  }
  return (value + align - 1u) & ~(align - 1u);
}

static uint32_t pool_alloc(uint32_t size, uint32_t align) {
  uint32_t off;
  uint32_t end;

  off = align_up_u32(g_module_pool_cursor, align == 0u ? 1u : align);
  end = off + size;
  if (end < off || end > NS_MODULE_POOL_SIZE) {
    return 0;
  }
  g_module_pool_cursor = end;
  return (uint32_t)&g_module_pool[off];
}

static void mem_zero_local(void *dst, uint32_t size) {
  uint8_t *d = (uint8_t *)dst;
  for (uint32_t i = 0; i < size; i++) {
    d[i] = 0;
  }
}

static void mem_copy_local(void *dst, const void *src, uint32_t size) {
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;
  for (uint32_t i = 0; i < size; i++) {
    d[i] = s[i];
  }
}

static void add_kernel_export(const char *name, uint32_t addr) {
  if (name == 0 || name[0] == '\0' || addr == 0 ||
      g_kernel_export_count >= NS_KERNEL_EXPORTS_MAX) {
    return;
  }
  for (int i = 0; i < g_kernel_export_count; i++) {
    if (str_eq_local(g_kernel_exports[i].name, name)) {
      g_kernel_exports[i].addr = addr;
      return;
    }
  }
  text_copy(g_kernel_exports[g_kernel_export_count].name,
            sizeof(g_kernel_exports[g_kernel_export_count].name), name);
  g_kernel_exports[g_kernel_export_count].addr = addr;
  g_kernel_export_count++;
}

static int find_module_slot_by_path(const char *path, int kind) {
  for (int i = 0; i < NS_MODULE_MAX; i++) {
    if (!g_modules[i].used) {
      continue;
    }
    if (g_modules[i].kind == kind && str_eq_local(g_modules[i].path, path)) {
      return i;
    }
  }
  return -1;
}

static int alloc_module_slot(void) {
  for (int i = 0; i < NS_MODULE_MAX; i++) {
    if (!g_modules[i].used) {
      return i;
    }
  }
  return -1;
}

static int module_add_export(NsLoadedObject *obj, const char *name, uint32_t addr) {
  if (obj == 0 || name == 0 || name[0] == '\0' || addr == 0) {
    return 0;
  }
  if (obj->export_count >= (int)(sizeof(obj->exports) / sizeof(obj->exports[0]))) {
    return 0;
  }

  for (int i = 0; i < obj->export_count; i++) {
    if (str_eq_local(obj->exports[i].name, name)) {
      obj->exports[i].addr = addr;
      return 1;
    }
  }

  text_copy(obj->exports[obj->export_count].name,
            sizeof(obj->exports[obj->export_count].name), name);
  obj->exports[obj->export_count].addr = addr;
  obj->export_count++;
  return 1;
}

static int resolve_external_symbol(const char *name, uint32_t *addr_out,
                                   int *owner_slot_out) {
  if (name == 0 || name[0] == '\0' || addr_out == 0) {
    return 0;
  }

  for (int i = 0; i < g_kernel_export_count; i++) {
    if (str_eq_local(g_kernel_exports[i].name, name)) {
      *addr_out = g_kernel_exports[i].addr;
      if (owner_slot_out != 0) {
        *owner_slot_out = -1;
      }
      return 1;
    }
  }

  for (int s = 0; s < NS_MODULE_MAX; s++) {
    if (!g_modules[s].used) {
      continue;
    }
    for (int e = 0; e < g_modules[s].export_count; e++) {
      if (str_eq_local(g_modules[s].exports[e].name, name)) {
        *addr_out = g_modules[s].exports[e].addr;
        if (owner_slot_out != 0) {
          *owner_slot_out = s;
        }
        return 1;
      }
    }
  }

  return 0;
}

int module_resolve_symbol_global(const char *symbol, uint32_t *addr_out) {
  return resolve_external_symbol(symbol, addr_out, 0);
}

static int add_dependency(NsLoadedObject *obj, int owner_slot) {
  if (obj == 0 || owner_slot < 0 || owner_slot >= NS_MODULE_MAX) {
    return 1;
  }
  for (int i = 0; i < obj->dep_count; i++) {
    if (obj->deps[i] == owner_slot) {
      return 1;
    }
  }
  if (obj->dep_count >= (int)(sizeof(obj->deps) / sizeof(obj->deps[0]))) {
    return 0;
  }
  obj->deps[obj->dep_count++] = owner_slot;
  g_modules[owner_slot].ref_count++;
  return 1;
}

static int compute_symbol_addr(const Elf32_Sym *sym, const uint32_t *sec_addrs,
                               uint16_t shnum, uint32_t *addr_out) {
  if (sym == 0 || sec_addrs == 0 || addr_out == 0) {
    return 0;
  }
  if (sym->st_shndx == SHN_UNDEF || sym->st_shndx >= shnum) {
    return 0;
  }
  if (sec_addrs[sym->st_shndx] == 0) {
    return 0;
  }
  *addr_out = sec_addrs[sym->st_shndx] + sym->st_value;
  return 1;
}

static int lazy_type_supported(uint32_t r_type) {
  return r_type == R_386_JMP_SLOT || r_type == R_386_GLOB_DAT;
}

static int add_pending_reloc(int target_slot, uint32_t place, uint32_t rel_type,
                             uint32_t addend, const char *symbol) {
  for (int i = 0; i < NS_PENDING_RELOC_MAX; i++) {
    if (!g_pending_relocs[i].used) {
      g_pending_relocs[i].used = 1;
      g_pending_relocs[i].target_slot = target_slot;
      g_pending_relocs[i].place = place;
      g_pending_relocs[i].addend = addend;
      g_pending_relocs[i].rel_type = rel_type;
      text_copy(g_pending_relocs[i].symbol, sizeof(g_pending_relocs[i].symbol),
                symbol == 0 ? "" : symbol);
      return 1;
    }
  }
  return 0;
}

static int try_resolve_pending_relocs(void) {
  int unresolved = 0;

  for (int i = 0; i < NS_PENDING_RELOC_MAX; i++) {
    uint32_t sym_addr = 0;
    int owner = -1;

    if (!g_pending_relocs[i].used) {
      continue;
    }

    if (!resolve_external_symbol(g_pending_relocs[i].symbol, &sym_addr, &owner)) {
      unresolved++;
      continue;
    }

    if (g_pending_relocs[i].rel_type == R_386_JMP_SLOT ||
        g_pending_relocs[i].rel_type == R_386_GLOB_DAT) {
      *(uint32_t *)g_pending_relocs[i].place = sym_addr;
    } else if (g_pending_relocs[i].rel_type == R_386_32) {
      *(uint32_t *)g_pending_relocs[i].place = g_pending_relocs[i].addend + sym_addr;
    } else if (g_pending_relocs[i].rel_type == R_386_PC32) {
      *(uint32_t *)g_pending_relocs[i].place = g_pending_relocs[i].addend +
                                               sym_addr - g_pending_relocs[i].place;
    }

    if (owner >= 0 && owner < NS_MODULE_MAX &&
        g_pending_relocs[i].target_slot >= 0 &&
        g_pending_relocs[i].target_slot < NS_MODULE_MAX) {
      (void)add_dependency(&g_modules[g_pending_relocs[i].target_slot], owner);
    }

    g_pending_relocs[i].used = 0;
  }

  return unresolved;
}

static int module_has_dependents(int slot) {
  for (int i = 0; i < NS_MODULE_MAX; i++) {
    if (!g_modules[i].used || i == slot) {
      continue;
    }
    for (int d = 0; d < g_modules[i].dep_count; d++) {
      if (g_modules[i].deps[d] == slot) {
        return 1;
      }
    }
  }
  return 0;
}

static int load_object_from_image(NsLoadedObject *obj, const uint8_t *image,
                                  uint32_t image_size) {
  const Elf32_Ehdr *eh;
  const Elf32_Shdr *shdr;
  uint32_t sec_addrs[NS_MODULE_MAX_SECTIONS];
  int symtab_idx = -1;
  int dynsym_idx = -1;
  int applied_relocs = 0;

  if (obj == 0 || image == 0 || image_size < sizeof(Elf32_Ehdr)) {
    return 0;
  }

  eh = (const Elf32_Ehdr *)image;

  if (!(eh->e_ident[0] == 0x7F && eh->e_ident[1] == 'E' && eh->e_ident[2] == 'L' &&
        eh->e_ident[3] == 'F' && eh->e_ident[4] == 1)) {
    return 0;
  }
  if (eh->e_machine != ELF_EM_386) {
    return 0;
  }
  if (eh->e_type != ELF_ET_REL && eh->e_type != ELF_ET_DYN) {
    return 0;
  }
  if (eh->e_shentsize != sizeof(Elf32_Shdr) || eh->e_shnum == 0 ||
      eh->e_shnum > NS_MODULE_MAX_SECTIONS) {
    return 0;
  }
  if (eh->e_shoff + ((uint32_t)eh->e_shnum * sizeof(Elf32_Shdr)) > image_size) {
    return 0;
  }

  shdr = (const Elf32_Shdr *)(image + eh->e_shoff);
  for (uint32_t i = 0; i < NS_MODULE_MAX_SECTIONS; i++) {
    sec_addrs[i] = 0;
  }
  obj->image_base = 0;

  for (uint32_t i = 0; i < eh->e_shnum; i++) {
    const Elf32_Shdr *sec = &shdr[i];
    uint32_t sec_mem;

    if (sec->sh_offset + sec->sh_size > image_size && sec->sh_type != SHT_NOBITS) {
      return 0;
    }

    if ((sec->sh_flags & SHF_ALLOC) == 0 || sec->sh_size == 0) {
      continue;
    }

    sec_mem = pool_alloc(sec->sh_size, sec->sh_addralign == 0 ? 4u : sec->sh_addralign);
    if (sec_mem == 0) {
      return 0;
    }
    sec_addrs[i] = sec_mem;
    if (obj->image_base == 0 || sec_mem < obj->image_base) {
      obj->image_base = sec_mem;
    }

    if (sec->sh_type == SHT_NOBITS) {
      mem_zero_local((void *)sec_mem, sec->sh_size);
    } else {
      mem_copy_local((void *)sec_mem, image + sec->sh_offset, sec->sh_size);
    }

    if (sec->sh_type == SHT_SYMTAB && symtab_idx < 0) {
      symtab_idx = (int)i;
    }
    if (sec->sh_type == SHT_DYNSYM && dynsym_idx < 0) {
      dynsym_idx = (int)i;
    }
  }

  for (uint32_t i = 0; i < eh->e_shnum; i++) {
    const Elf32_Shdr *rel_sec = &shdr[i];
    const Elf32_Shdr *sym_sec;
    const Elf32_Shdr *str_sec;
    const Elf32_Sym *syms;
    const char *strtab;
    uint32_t rel_count;

    {
      int is_rela = 0;

      if (rel_sec->sh_type == SHT_REL && rel_sec->sh_entsize == sizeof(Elf32_Rel)) {
        is_rela = 0;
      } else if (rel_sec->sh_type == SHT_RELA && rel_sec->sh_entsize == sizeof(Elf32_Rela)) {
        is_rela = 1;
      } else {
        continue;
      }

      if (rel_sec->sh_info >= eh->e_shnum || rel_sec->sh_link >= eh->e_shnum) {
        return 0;
      }
      if (sec_addrs[rel_sec->sh_info] == 0) {
        continue;
      }

      sym_sec = &shdr[rel_sec->sh_link];
      if ((sym_sec->sh_type != SHT_SYMTAB && sym_sec->sh_type != SHT_DYNSYM) ||
          sym_sec->sh_entsize != sizeof(Elf32_Sym) || sym_sec->sh_link >= eh->e_shnum) {
        return 0;
      }

      str_sec = &shdr[sym_sec->sh_link];
      if (str_sec->sh_type != SHT_STRTAB) {
        return 0;
      }

      if (sym_sec->sh_offset + sym_sec->sh_size > image_size ||
          str_sec->sh_offset + str_sec->sh_size > image_size ||
          rel_sec->sh_offset + rel_sec->sh_size > image_size) {
        return 0;
      }

      syms = (const Elf32_Sym *)(image + sym_sec->sh_offset);
      strtab = (const char *)(image + str_sec->sh_offset);
      rel_count = rel_sec->sh_size / rel_sec->sh_entsize;

      for (uint32_t r = 0; r < rel_count; r++) {
        uint32_t r_offset;
        uint32_t r_info;
        uint32_t sym_idx;
        uint32_t r_type;
        uint32_t place;
        uint32_t addend;
        uint32_t sym_addr = 0;
        int owner_slot = -1;

        if (is_rela) {
          const Elf32_Rela *rela = (const Elf32_Rela *)(image + rel_sec->sh_offset +
                                                        (r * sizeof(Elf32_Rela)));
          r_offset = rela->r_offset;
          r_info = rela->r_info;
          addend = (uint32_t)rela->r_addend;
        } else {
          const Elf32_Rel *rel = (const Elf32_Rel *)(image + rel_sec->sh_offset +
                                                     (r * sizeof(Elf32_Rel)));
          r_offset = rel->r_offset;
          r_info = rel->r_info;
        }

        sym_idx = ELF32_R_SYM(r_info);
        r_type = ELF32_R_TYPE(r_info);

        if (sym_idx >= (sym_sec->sh_size / sizeof(Elf32_Sym))) {
          return 0;
        }
        if (r_offset + 4u > shdr[rel_sec->sh_info].sh_size) {
          return 0;
        }

        place = sec_addrs[rel_sec->sh_info] + r_offset;
        if (!is_rela) {
          addend = *(uint32_t *)place;
        }

        if (syms[sym_idx].st_shndx == SHN_UNDEF) {
          const char *name;
          if (syms[sym_idx].st_name >= str_sec->sh_size) {
            return 0;
          }
          name = strtab + syms[sym_idx].st_name;

          if (!resolve_external_symbol(name, &sym_addr, &owner_slot)) {
            if (lazy_type_supported(r_type)) {
              if (!add_pending_reloc((int)(obj - g_modules), place, r_type, addend, name)) {
                return 0;
              }
              *(uint32_t *)place = 0;
              continue;
            }
            return 0;
          }

          if (!add_dependency(obj, owner_slot)) {
            return 0;
          }
        } else {
          if (!compute_symbol_addr(&syms[sym_idx], sec_addrs, eh->e_shnum, &sym_addr)) {
            return 0;
          }
        }

        if (r_type == R_386_NONE) {
          continue;
        }
        if (r_type == R_386_32) {
          *(uint32_t *)place = addend + sym_addr;
        } else if (r_type == R_386_PC32) {
          *(uint32_t *)place = addend + sym_addr - place;
        } else if (r_type == R_386_GLOB_DAT || r_type == R_386_JMP_SLOT) {
          *(uint32_t *)place = sym_addr;
        } else if (r_type == R_386_RELATIVE) {
          *(uint32_t *)place = obj->image_base + addend;
        } else {
          return 0;
        }
        applied_relocs++;
      }
    }
  }

  if (symtab_idx < 0 && dynsym_idx >= 0) {
    symtab_idx = dynsym_idx;
  }
  if (symtab_idx >= 0) {
    const Elf32_Shdr *sym_sec = &shdr[symtab_idx];
    const Elf32_Shdr *str_sec;
    const Elf32_Sym *syms;
    const char *strtab;
    uint32_t sym_count;

    if (sym_sec->sh_link >= eh->e_shnum) {
      return 0;
    }
    str_sec = &shdr[sym_sec->sh_link];
    if (str_sec->sh_type != SHT_STRTAB) {
      return 0;
    }

    syms = (const Elf32_Sym *)(image + sym_sec->sh_offset);
    strtab = (const char *)(image + str_sec->sh_offset);
    sym_count = sym_sec->sh_size / sizeof(Elf32_Sym);

    for (uint32_t s = 0; s < sym_count; s++) {
      const Elf32_Sym *sym = &syms[s];
      const char *name;
      uint32_t addr;
      uint8_t bind;

      if (sym->st_name >= str_sec->sh_size) {
        continue;
      }
      name = strtab + sym->st_name;
      bind = ELF32_ST_BIND(sym->st_info);

      if (!compute_symbol_addr(sym, sec_addrs, eh->e_shnum, &addr)) {
        continue;
      }

      if ((bind == STB_GLOBAL || bind == STB_WEAK) && name[0] != '\0') {
        (void)module_add_export(obj, name, addr);
      }
    }
  }

  obj->init_addr = module_dlsym(obj->handle, "ns_module_init");
  obj->fini_addr = module_dlsym(obj->handle, "ns_module_fini");

  if (obj->kind == NS_MODULE_KIND_KERNEL) {
    uint32_t abi_ptr = module_dlsym(obj->handle, "ns_module_abi");
    if (abi_ptr != 0 && *(uint32_t *)abi_ptr != NS_MODULE_ABI_MAGIC) {
      return 0;
    }
  }

  (void)applied_relocs;
  return 1;
}

static int load_new_module(const char *path, int kind) {
  VfsFileStat st;
  int slot;
  int size;

  if (path == 0 || path[0] == '\0') {
    return -1;
  }

  slot = find_module_slot_by_path(path, kind);
  if (slot >= 0) {
    g_modules[slot].ref_count++;
    return g_modules[slot].handle;
  }

  if (vfs_stat(path, &st) != VFS_OK || st.flags == 0 || st.size == 0 ||
      st.size > NS_MODULE_LOAD_LIMIT) {
    return -1;
  }

  size = vfs_read_file(path, g_load_buf, sizeof(g_load_buf));
  if (size <= 0 || (uint32_t)size != st.size) {
    return -1;
  }

  slot = alloc_module_slot();
  if (slot < 0) {
    return -1;
  }

  mem_zero_local(&g_modules[slot], sizeof(g_modules[slot]));
  g_modules[slot].used = 1;
  g_modules[slot].kind = kind;
  g_modules[slot].handle = 0x4000 + slot;
  g_modules[slot].ref_count = 1;
  text_copy(g_modules[slot].path, sizeof(g_modules[slot].path), path);

  if (!load_object_from_image(&g_modules[slot], g_load_buf, (uint32_t)size)) {
    mem_zero_local(&g_modules[slot], sizeof(g_modules[slot]));
    return -1;
  }

  return g_modules[slot].handle;
}

static int find_module_slot_by_handle(int handle) {
  for (int i = 0; i < NS_MODULE_MAX; i++) {
    if (g_modules[i].used && g_modules[i].handle == handle) {
      return i;
    }
  }
  return -1;
}

static int release_module_slot(int slot, int run_fini) {
  module_entry_t fini;

  if (slot < 0 || slot >= NS_MODULE_MAX || !g_modules[slot].used) {
    return 0;
  }

  if (g_modules[slot].ref_count > 0) {
    return 1;
  }
  if (module_has_dependents(slot)) {
    return 0;
  }

  if (run_fini && g_modules[slot].fini_addr != 0) {
    fini = (module_entry_t)g_modules[slot].fini_addr;
    (void)fini();
  }

  for (int i = 0; i < g_modules[slot].dep_count; i++) {
    int dep = g_modules[slot].deps[i];
    if (dep >= 0 && dep < NS_MODULE_MAX && g_modules[dep].used &&
        g_modules[dep].ref_count > 0) {
      g_modules[dep].ref_count--;
      (void)release_module_slot(dep, 0);
    }
  }

  for (int p = 0; p < NS_PENDING_RELOC_MAX; p++) {
    if (g_pending_relocs[p].used && g_pending_relocs[p].target_slot == slot) {
      g_pending_relocs[p].used = 0;
    }
  }

  mem_zero_local(&g_modules[slot], sizeof(g_modules[slot]));
  return 1;
}

void module_loader_init(void) {
  g_kernel_export_count = 0;
  g_module_pool_cursor = 0;

  mem_zero_local(g_modules, sizeof(g_modules));
  mem_zero_local(g_pending_relocs, sizeof(g_pending_relocs));
  mem_zero_local(g_dlerror, sizeof(g_dlerror));

  add_kernel_export("gprint", (uint32_t)gprint);
  add_kernel_export("set_cmd_output", (uint32_t)set_cmd_output);
  add_kernel_export("vfs_open", (uint32_t)vfs_open);
  add_kernel_export("vfs_close", (uint32_t)vfs_close);
  add_kernel_export("vfs_read", (uint32_t)vfs_read);
  add_kernel_export("vfs_write", (uint32_t)vfs_write);
  add_kernel_export("vfs_read_file", (uint32_t)vfs_read_file);
  add_kernel_export("vfs_write_file", (uint32_t)vfs_write_file);
  add_kernel_export("vfs_stat", (uint32_t)vfs_stat);
}

int module_dlopen_for_task(int task_id, const char *path) {
  int slot;
  int handle;

  if (task_id < 0 || task_id >= MAX_TASKS) {
    return -1;
  }

  clear_dlerror_for_task(task_id);
  handle = load_new_module(path, NS_MODULE_KIND_LIBRARY);
  if (handle < 0) {
    set_dlerror_for_task(task_id, "dlopen: load failed");
    return -1;
  }

  slot = find_module_slot_by_handle(handle);
  if (slot < 0) {
    set_dlerror_for_task(task_id, "dlopen: bad handle");
    return -1;
  }

  g_modules[slot].task_refs[task_id]++;
  if (try_resolve_pending_relocs() > 0) {
    set_dlerror_for_task(task_id, "dlopen: pending lazy symbols");
  }
  return handle;
}

int module_dlopen(const char *path) {
  return module_dlopen_for_task(current_task_slot(), path);
}

int module_dlclose_for_task(int task_id, int handle) {
  int slot;

  if (task_id < 0 || task_id >= MAX_TASKS) {
    return -1;
  }

  clear_dlerror_for_task(task_id);
  slot = find_module_slot_by_handle(handle);
  if (slot < 0 || !g_modules[slot].used) {
    set_dlerror_for_task(task_id, "dlclose: bad handle");
    return -1;
  }
  if (g_modules[slot].kind != NS_MODULE_KIND_LIBRARY) {
    set_dlerror_for_task(task_id, "dlclose: not library");
    return -1;
  }
  if (g_modules[slot].task_refs[task_id] <= 0) {
    set_dlerror_for_task(task_id, "dlclose: not owned by task");
    return -1;
  }

  g_modules[slot].task_refs[task_id]--;
  if (g_modules[slot].ref_count > 0) {
    g_modules[slot].ref_count--;
  }

  if (!release_module_slot(slot, 0)) {
    set_dlerror_for_task(task_id, "dlclose: dependency busy");
    return -1;
  }
  return 0;
}

int module_dlclose(int handle) {
  return module_dlclose_for_task(current_task_slot(), handle);
}

uint32_t module_dlsym(int handle, const char *symbol) {
  int slot = find_module_slot_by_handle(handle);

  if (slot < 0 || symbol == 0 || symbol[0] == '\0') {
    set_dlerror_for_task(current_task_slot(), "dlsym: bad args");
    return 0;
  }

  for (int i = 0; i < g_modules[slot].export_count; i++) {
    if (str_eq_local(g_modules[slot].exports[i].name, symbol)) {
      clear_dlerror_for_task(current_task_slot());
      return g_modules[slot].exports[i].addr;
    }
  }

  set_dlerror_for_task(current_task_slot(), "dlsym: symbol not found");
  return 0;
}

const char *module_dlerror_for_task(int task_id) {
  if (task_id < 0 || task_id >= MAX_TASKS) {
    return "dlerror: invalid task";
  }
  return g_dlerror[task_id];
}

const char *module_dlerror(void) {
  return module_dlerror_for_task(current_task_slot());
}

int module_copy_dlerror_for_task(int task_id, char *out, uint32_t out_cap) {
  uint32_t i = 0;
  const char *src = module_dlerror_for_task(task_id);

  if (out == 0 || out_cap == 0) {
    return -1;
  }
  while (src[i] && i + 1 < out_cap) {
    out[i] = src[i];
    i++;
  }
  out[i] = '\0';
  return (int)i;
}

int module_insmod(const char *path) {
  int handle = load_new_module(path, NS_MODULE_KIND_KERNEL);
  int slot;
  module_entry_t init;
  int rc;

  if (handle < 0) {
    set_dlerror_for_task(current_task_slot(), "insmod: load failed");
    return -1;
  }

  slot = find_module_slot_by_handle(handle);
  if (slot < 0) {
    set_dlerror_for_task(current_task_slot(), "insmod: bad handle");
    return -1;
  }

  g_modules[slot].kernel_refs++;

  if (!g_modules[slot].init_called) {
    if (g_modules[slot].init_addr != 0) {
      init = (module_entry_t)g_modules[slot].init_addr;
      rc = init();
      if (rc != 0) {
        if (g_modules[slot].kernel_refs > 0) {
          g_modules[slot].kernel_refs--;
        }
        if (g_modules[slot].ref_count > 0) {
          g_modules[slot].ref_count--;
        }
        (void)release_module_slot(slot, 0);
        set_dlerror_for_task(current_task_slot(), "insmod: init failed");
        return -1;
      }
    }
    g_modules[slot].init_called = 1;
  }

  if (try_resolve_pending_relocs() > 0) {
    set_dlerror_for_task(current_task_slot(), "insmod: unresolved lazy symbols");
  } else {
    clear_dlerror_for_task(current_task_slot());
  }
  return 0;
}

int module_rmmod(const char *path_or_handle) {
  uint32_t handle_value = 0;
  int slot = -1;

  if (path_or_handle == 0 || path_or_handle[0] == '\0') {
    set_dlerror_for_task(current_task_slot(), "rmmod: bad args");
    return -1;
  }

  if (parse_u32_dec_local(path_or_handle, &handle_value)) {
    slot = find_module_slot_by_handle((int)handle_value);
  }

  if (slot < 0) {
    slot = find_module_slot_by_path(path_or_handle, NS_MODULE_KIND_KERNEL);
  }

  if (slot < 0 || g_modules[slot].kind != NS_MODULE_KIND_KERNEL) {
    set_dlerror_for_task(current_task_slot(), "rmmod: module not found");
    return -1;
  }
  if (g_modules[slot].kernel_refs <= 0) {
    set_dlerror_for_task(current_task_slot(), "rmmod: no kernel ref");
    return -1;
  }
  if (module_has_dependents(slot)) {
    set_dlerror_for_task(current_task_slot(), "rmmod: dependent modules exist");
    return -1;
  }

  g_modules[slot].kernel_refs--;
  if (g_modules[slot].ref_count > 0) {
    g_modules[slot].ref_count--;
  }

  if (!release_module_slot(slot, 1)) {
    set_dlerror_for_task(current_task_slot(), "rmmod: module still busy");
    return -1;
  }

  clear_dlerror_for_task(current_task_slot());
  return 0;
}

int module_list(NsModuleInfo *out, int max_entries) {
  int n = 0;

  if (out == 0 || max_entries <= 0) {
    return 0;
  }

  for (int i = 0; i < NS_MODULE_MAX && n < max_entries; i++) {
    if (!g_modules[i].used) {
      continue;
    }
    out[n].used = 1;
    out[n].kind = g_modules[i].kind;
    out[n].handle = g_modules[i].handle;
    out[n].ref_count = g_modules[i].ref_count;
    out[n].exported_symbols = g_modules[i].export_count;
    text_copy(out[n].path, sizeof(out[n].path), g_modules[i].path);
    n++;
  }

  return n;
}
