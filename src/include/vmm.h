
#ifndef VMM_H
#define VMM_H

#include "types.h"

void vmm_init();
void vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags);
void vmm_unmap_page(uint32_t virt);
uint32_t vmm_get_phys_addr(uint32_t virt);

uint32_t* vmm_create_user_page_dir();
void vmm_switch_page_dir(uint32_t* page_dir);
void vmm_copy_kernel_pages(uint32_t* new_pd);
void vmm_free_user_pages(uint32_t* page_dir);

#endif
