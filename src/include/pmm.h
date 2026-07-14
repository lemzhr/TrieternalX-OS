
#ifndef PMM_H
#define PMM_H

#include "types.h"

#define PAGE_SIZE 4096

void pmm_init(size_t mem_size);
void* pmm_alloc_frame();
void pmm_free_frame(void* frame);
size_t pmm_get_free_frames();
size_t pmm_get_used_frames();

#endif
