#include "usermode.h"

#include "paging.h"
#include "scheduler.h"
#include "task.h"
#include "vfs.h"
#include "module_loader.h"
#include "posix.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;

#define PT_LOAD 1

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
  uint32_t p_type;
  uint32_t p_offset;
  uint32_t p_vaddr;
  uint32_t p_paddr;
  uint32_t p_filesz;
  uint32_t p_memsz;
  uint32_t p_flags;
  uint32_t p_align;
} __attribute__((packed)) Elf32_Phdr;

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

enum {
  USER_VA_CODE = 0x40000000,
  USER_VA_STACK_TOP = 0x40002000,
  USER_PAGE_MASK = 0xFFFFF000u,
  KERNEL_TEMP_MAP_VA = 0x3FF00000,
  ET_EXEC = 2,
  ET_DYN = 3,
  SHT_NULL = 0,
  SHT_SYMTAB = 2,
  SHT_STRTAB = 3,
  SHT_RELA = 4,
  SHT_REL = 9,
  SHN_UNDEF = 0,
  R_386_NONE = 0,
  R_386_32 = 1,
  R_386_PC32 = 2,
  R_386_GLOB_DAT = 6,
  R_386_JMP_SLOT = 7,
  R_386_RELATIVE = 8,
  ELF32_R_INFO_SYM_SHIFT = 8,
  ELF32_R_INFO_TYPE_MASK = 0xFF,
};

extern void *pmm_alloc_page(void);
extern void create_task(int index, void (*func_ptr)(), uint32_t page_dir);
extern void enter_user_mode(uint32_t entry_eip, uint32_t user_stack, uint32_t page_dir);
extern uint32_t scheduler_now_ticks(void);
extern uint8_t user_program_start;
extern uint8_t user_program_end;

static uint32_t user_page_dir = 0;
static uint32_t user_entry = USER_VA_CODE;
static uint32_t user_stack_top = USER_VA_STACK_TOP - 16;

static uint8_t exec_buffer[4096];
static char last_exec_path[128];

static void user_task_entry(void);

static void mem_zero(uint8_t *dst, uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    dst[i] = 0;
  }
}

static void mem_copy(uint8_t *dst, const uint8_t *src, uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    dst[i] = src[i];
  }
}

static uint8_t *kmap_temp_page(uint32_t phys_page) {
  map_page(phys_page, KERNEL_TEMP_MAP_VA);
  return (uint8_t *)KERNEL_TEMP_MAP_VA;
}

static void kunmap_temp_page(void) { unmap_kernel_page(KERNEL_TEMP_MAP_VA); }

static void mem_zero_phys_page(uint32_t phys_page) {
  uint8_t *mapped = kmap_temp_page(phys_page & USER_PAGE_MASK);
  mem_zero(mapped, 4096);
  kunmap_temp_page();
}

static int copy_to_user_space(uint32_t pd, uint32_t user_va, const uint8_t *src,
                              uint32_t size) {
  uint32_t copied = 0;

  while (copied < size) {
    uint32_t dst_phys = resolve_user_phys(pd, user_va + copied);
    uint32_t dst_page = dst_phys & USER_PAGE_MASK;
    uint32_t page_off = dst_phys & 0xFFFu;
    uint32_t chunk;
    uint8_t *mapped;

    if (dst_phys == 0) {
      return 0;
    }

    chunk = 4096u - page_off;
    if (chunk > (size - copied)) {
      chunk = size - copied;
    }

    mapped = kmap_temp_page(dst_page);
    mem_copy(mapped + page_off, src + copied, chunk);
    kunmap_temp_page();

    copied += chunk;
  }

  return 1;
}

static int map_user_range(uint32_t pd, uint32_t start, uint32_t size) {
  uint32_t first = start & 0xFFFFF000;
  uint32_t last = (start + size - 1) & 0xFFFFF000;

  if (size == 0) {
    return 0;
  }

  for (uint32_t va = first; va <= last; va += 4096) {
    uint32_t phys = (uint32_t)pmm_alloc_page();
    if (phys == 0) {
      return 0;
    }

    if (!map_user_page(pd, phys, va)) {
      return 0;
    }

    mem_zero_phys_page(phys);
  }

  return 1;
}

static int read_user_u32(uint32_t pd, uint32_t user_va, uint32_t *value_out) {
  uint32_t phys = resolve_user_phys(pd, user_va);
  uint32_t page;
  uint32_t off;
  uint8_t *mapped;

  if (value_out == 0 || phys == 0) {
    return 0;
  }

  page = phys & USER_PAGE_MASK;
  off = phys & 0xFFFu;
  if (off > 4092u) {
    return 0;
  }

  mapped = kmap_temp_page(page);
  *value_out = *(uint32_t *)(mapped + off);
  kunmap_temp_page();
  return 1;
}

static int write_user_u32(uint32_t pd, uint32_t user_va, uint32_t value) {
  uint32_t phys = resolve_user_phys(pd, user_va);
  uint32_t page;
  uint32_t off;
  uint8_t *mapped;

  if (phys == 0) {
    return 0;
  }

  page = phys & USER_PAGE_MASK;
  off = phys & 0xFFFu;
  if (off > 4092u) {
    return 0;
  }

  mapped = kmap_temp_page(page);
  *(uint32_t *)(mapped + off) = value;
  kunmap_temp_page();
  return 1;
}

static int apply_elf_relocations(const uint8_t *image, uint32_t size,
                                 const Elf32_Ehdr *eh, uint32_t pd,
                                 uint32_t load_bias) {
  const Elf32_Shdr *shdr;

  if (eh->e_shoff == 0 || eh->e_shnum == 0 || eh->e_shentsize != sizeof(Elf32_Shdr)) {
    return 1;
  }
  if (eh->e_shoff + ((uint32_t)eh->e_shnum * sizeof(Elf32_Shdr)) > size) {
    return 0;
  }

  shdr = (const Elf32_Shdr *)(image + eh->e_shoff);

  for (uint32_t i = 0; i < eh->e_shnum; i++) {
    const Elf32_Shdr *rel_sec = &shdr[i];
    const Elf32_Shdr *target_sec;
    const Elf32_Shdr *sym_sec;
    const Elf32_Shdr *str_sec;
    const Elf32_Sym *syms;
    const char *strtab;
    uint32_t rel_count;

    if (rel_sec->sh_type == SHT_NULL || rel_sec->sh_size == 0) {
      continue;
    }
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
    if (rel_sec->sh_offset + rel_sec->sh_size > size) {
      return 0;
    }

    target_sec = &shdr[rel_sec->sh_info];
    sym_sec = &shdr[rel_sec->sh_link];
    if (sym_sec->sh_type != SHT_SYMTAB || sym_sec->sh_entsize != sizeof(Elf32_Sym)) {
      return 0;
    }
    if (sym_sec->sh_link >= eh->e_shnum) {
      return 0;
    }
    str_sec = &shdr[sym_sec->sh_link];
    if (str_sec->sh_type != SHT_STRTAB) {
      return 0;
    }
    if (sym_sec->sh_offset + sym_sec->sh_size > size ||
        str_sec->sh_offset + str_sec->sh_size > size) {
      return 0;
    }

    syms = (const Elf32_Sym *)(image + sym_sec->sh_offset);
    strtab = (const char *)(image + str_sec->sh_offset);
    rel_count = rel_sec->sh_size / rel_sec->sh_entsize;

    for (uint32_t r = 0; r < rel_count; r++) {
      uint32_t r_offset;
      uint32_t r_info;
      uint32_t sym_idx;
      uint32_t rel_type;
      uint32_t place;
      uint32_t addend;
      uint32_t sym_value;

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

      sym_idx = r_info >> ELF32_R_INFO_SYM_SHIFT;
      rel_type = r_info & ELF32_R_INFO_TYPE_MASK;

      if (target_sec->sh_addr + r_offset < target_sec->sh_addr) {
        return 0;
      }

      place = target_sec->sh_addr + load_bias + r_offset;
      if (!is_rela && !read_user_u32(pd, place, &addend)) {
        return 0;
      }

      if (sym_idx >= (sym_sec->sh_size / sizeof(Elf32_Sym))) {
        return 0;
      }

      if (syms[sym_idx].st_shndx == SHN_UNDEF) {
        uint32_t name_off = syms[sym_idx].st_name;
        uint32_t ext_addr;
        if (name_off >= str_sec->sh_size) {
          return 0;
        }
        if (!module_resolve_symbol_global(strtab + name_off, &ext_addr)) {
          return 0;
        }
        sym_value = ext_addr;
      } else {
        sym_value = syms[sym_idx].st_value + load_bias;
      }

      if (rel_type == R_386_NONE) {
        continue;
      } else if (rel_type == R_386_32) {
        if (!write_user_u32(pd, place, addend + sym_value)) {
          return 0;
        }
      } else if (rel_type == R_386_PC32) {
        if (!write_user_u32(pd, place, addend + sym_value - place)) {
          return 0;
        }
      } else if (rel_type == R_386_GLOB_DAT || rel_type == R_386_JMP_SLOT) {
        if (!write_user_u32(pd, place, sym_value)) {
          return 0;
        }
      } else if (rel_type == R_386_RELATIVE) {
        if (!write_user_u32(pd, place, load_bias + addend)) {
          return 0;
        }
      } else {
        return 0;
      }
    }
    }
  }

  return 1;
}

static int load_flat_image(const uint8_t *image, uint32_t size, uint32_t pd,
                           uint32_t *entry_out) {
  uint32_t phys;

  if (size == 0 || size > 4096) {
    return 0;
  }

  phys = (uint32_t)pmm_alloc_page();
  if (phys == 0) {
    return 0;
  }

  if (!map_user_page(pd, phys, USER_VA_CODE)) {
    return 0;
  }

  mem_zero_phys_page(phys);
  if (!copy_to_user_space(pd, USER_VA_CODE, image, size)) {
    return 0;
  }

  *entry_out = USER_VA_CODE;
  return 1;
}

static int load_elf32_image(const uint8_t *image, uint32_t size, uint32_t pd,
                            uint32_t *entry_out) {
  const Elf32_Ehdr *eh;
  uint32_t phdr_table_size;
  uint32_t load_bias = 0;
  uint32_t dyn_low = 0xFFFFFFFFu;
  uint32_t dyn_high = 0;

  if (size < sizeof(Elf32_Ehdr)) {
    return 0;  /* File too small for header */
  }

  eh = (const Elf32_Ehdr *)image;

  /* Validate ELF magic number (0x7F 'E' 'L' 'F') */
  if (!(eh->e_ident[0] == 0x7F && eh->e_ident[1] == 'E' && eh->e_ident[2] == 'L' &&
        eh->e_ident[3] == 'F' && eh->e_ident[4] == 1)) {
    return 0;  /* Not an ELF32 file */
  }

  /* Must be i386 executable */
  if (eh->e_machine != 3) {
    return 0;  /* Wrong machine type */
  }

  /* Must be executable, not shared object or relocatable */
  if (eh->e_type != ET_EXEC && eh->e_type != ET_DYN) {
    return 0;  /* e_type must be 2 (executable) or 3 (shared) */
  }

  /* Validate program header entry size */
  if (eh->e_phentsize != sizeof(Elf32_Phdr)) {
    return 0;  /* Program header size mismatch */
  }

  /* Validate program header table bounds */
  phdr_table_size = (uint32_t)eh->e_phnum * sizeof(Elf32_Phdr);
  if (eh->e_phoff + phdr_table_size > size) {
    return 0;  /* Program header table out of bounds */
  }

  if (eh->e_type == ET_DYN) {
    for (uint32_t i = 0; i < eh->e_phnum; i++) {
      const Elf32_Phdr *ph =
          (const Elf32_Phdr *)(image + eh->e_phoff + (i * sizeof(Elf32_Phdr)));
      uint32_t seg_end;

      if (ph->p_type != PT_LOAD || ph->p_memsz == 0) {
        continue;
      }

      if (ph->p_vaddr < dyn_low) {
        dyn_low = ph->p_vaddr;
      }
      seg_end = ph->p_vaddr + ph->p_memsz;
      if (seg_end > dyn_high) {
        dyn_high = seg_end;
      }
    }

    if (dyn_low == 0xFFFFFFFFu || dyn_high <= dyn_low) {
      return 0;
    }

    {
      uint32_t span = (dyn_high - (dyn_low & USER_PAGE_MASK));
      uint32_t aslr_slot = scheduler_now_ticks() & 0x1Fu;
      uint32_t base = USER_VA_CODE + (aslr_slot * 0x1000u);
      if (!is_user_range((const void *)base, span)) {
        base = USER_VA_CODE;
      }
      load_bias = base - (dyn_low & USER_PAGE_MASK);
    }
  }

  /* Load each PT_LOAD segment */
  for (uint32_t i = 0; i < eh->e_phnum; i++) {
    const Elf32_Phdr *ph =
        (const Elf32_Phdr *)(image + eh->e_phoff + (i * sizeof(Elf32_Phdr)));

    if (ph->p_type != PT_LOAD || ph->p_memsz == 0) {
      continue;  /* Skip non-loadable segments */
    }

    if (ph->p_align != 0 && (ph->p_align & (ph->p_align - 1)) != 0) {
      return 0;  /* Alignment must be power-of-two or zero */
    }

    if (ph->p_align > 1 && ((ph->p_offset & (ph->p_align - 1)) !=
                            (ph->p_vaddr & (ph->p_align - 1)))) {
      return 0;  /* File/virtual alignment mismatch */
    }

    if (ph->p_vaddr + ph->p_memsz < ph->p_vaddr) {
      return 0;  /* Wraparound */
    }

    /* Validate segment is in user virtual address space */
    if (!is_user_range((const void *)(ph->p_vaddr + load_bias), ph->p_memsz)) {
      return 0;  /* Segment outside user address range */
    }

    /* Validate file offsets */
    if (ph->p_offset + ph->p_filesz > size) {
      return 0;  /* Segment data out of bounds */
    }

    if (ph->p_filesz > ph->p_memsz) {
      return 0;  /* File size > memory size (invalid) */
    }

    /* Allocate and map pages for this segment */
    if (!map_user_range(pd, ph->p_vaddr + load_bias, ph->p_memsz)) {
      return 0;  /* Page allocation failed */
    }

    if (ph->p_filesz > 0) {
      if (!copy_to_user_space(pd, ph->p_vaddr + load_bias,
                              &image[ph->p_offset], ph->p_filesz)) {
        return 0;
      }
    }
  }

  if (!apply_elf_relocations(image, size, eh, pd, load_bias)) {
    return 0;
  }

  *entry_out = eh->e_entry + load_bias;

  /* Validate entry point is in user address range */
  if (!is_user_range((const void *)*entry_out, 1)) {
    return 0;  /* Entry point outside user space */
  }

  if (resolve_user_phys(pd, *entry_out) == 0) {
    return 0;  /* Entry point not mapped */
  }

  return 1;
}

static int setup_user_stack(uint32_t pd) {
  uint32_t stack_phys = (uint32_t)pmm_alloc_page();
  uint32_t guard_page_va = USER_VA_STACK_TOP - 8192;
  if (stack_phys == 0) {
    return 0;
  }

  if (!map_user_page(pd, stack_phys, USER_VA_STACK_TOP - 4096)) {
    return 0;
  }

  mem_zero_phys_page(stack_phys);

  /* Keep one page below the stack unmapped to catch stack overflow. */
  if (resolve_user_phys(pd, guard_page_va) != 0) {
    return 0;
  }
  return 1;
}

static int finalize_user_process(uint32_t pd, uint32_t entry) {
  user_page_dir = pd;
  user_entry = entry;
  user_stack_top = USER_VA_STACK_TOP - 16;

  // Replace existing user task slot with a fresh process image.
  os_tasks[2].state = TASK_STATE_TERMINATED;
  create_task(2, user_task_entry, user_page_dir);
  posix_task_init(2, os_current_task);
  if (last_exec_path[0] != '\0') {
    (void)posix_apply_exec_credentials(2, last_exec_path);
  }
  return 1;
}

static int exec_user_image(const uint8_t *image, uint32_t image_size) {
  uint32_t pd;
  uint32_t entry = USER_VA_CODE;

  if (image == 0 || image_size == 0 || image_size > sizeof(exec_buffer)) {
    return 0;
  }

  pd = create_user_page_directory();
  if (pd == 0) {
    return 0;
  }

  if (!setup_user_stack(pd)) {
    destroy_user_address_space(pd);
    return 0;
  }

  if (image[0] == 0x7F && image[1] == 'E' && image[2] == 'L' && image[3] == 'F') {
    if (!load_elf32_image(image, image_size, pd, &entry)) {
      destroy_user_address_space(pd);
      return 0;
    }
  } else {
    if (!load_flat_image(image, image_size, pd, &entry)) {
      destroy_user_address_space(pd);
      return 0;
    }
  }

  return finalize_user_process(pd, entry);
}

static void user_task_entry(void) {
  enter_user_mode(user_entry, user_stack_top, user_page_dir);
  task_terminate_current();
}

int launch_user_process_task(void) {
  uint8_t *code_start = &user_program_start;
  uint8_t *code_end = &user_program_end;
  uint32_t code_size = (uint32_t)(code_end - code_start);

  if (code_size == 0 || code_size > 4096) {
    return 0;
  }

  return exec_user_image(code_start, code_size);
}

int exec_user_program(const char *path) {
  int image_size;
  VfsFileStat stat_buf;

  if (path == 0 || path[0] == '\0') {
    return 0;
  }

  {
    int i = 0;
    while (path[i] && i < (int)sizeof(last_exec_path) - 1) {
      last_exec_path[i] = path[i];
      i++;
    }
    last_exec_path[i] = '\0';
  }

  if (vfs_stat(path, &stat_buf) != VFS_OK || stat_buf.flags == 0 ||
      stat_buf.size == 0 || stat_buf.size > sizeof(exec_buffer)) {
    return 0;
  }

  image_size = vfs_read_file(path, exec_buffer, sizeof(exec_buffer));
  if (image_size <= 0 || (uint32_t)image_size != stat_buf.size) {
    return 0;
  }

  return exec_user_image(exec_buffer, (uint32_t)image_size);
}
