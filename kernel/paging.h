#ifndef PAGING_H
#define PAGING_H

typedef unsigned int uint32_t;

extern uint32_t page_directory[1024];

void init_paging(void);
void map_page(uint32_t phys_addr, uint32_t virt_addr);
void map_page_flags(uint32_t phys_addr, uint32_t virt_addr, uint32_t flags);
void unmap_kernel_page(uint32_t virt_addr);
void map_mmio_region(uint32_t phys, uint32_t size);

uint32_t create_user_page_directory(void);
int map_user_page(uint32_t pd_phys, uint32_t phys_addr, uint32_t virt_addr);
int is_user_range(const void *ptr, uint32_t size);
int is_user_range_accessible(uint32_t pd_phys, const void *ptr, uint32_t size);
uint32_t resolve_user_phys(uint32_t pd_phys, uint32_t virt_addr);
uint32_t clone_user_address_space(uint32_t src_pd_phys);
void destroy_user_address_space(uint32_t pd_phys);

#endif
