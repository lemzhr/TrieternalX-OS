
#ifndef KHEAP_H
#define KHEAP_H

#include "types.h"

#define HEAP_START 0xC0000000
#define HEAP_MAX   0xD0000000

struct HeapBlock
{
    size_t size;
    bool is_free;
    HeapBlock* next;
    HeapBlock* prev;
};

void kheap_init();
void* kmalloc(size_t size);
void kfree(void* ptr);

void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;
void operator delete(void* ptr, size_t size) noexcept;
void operator delete[](void* ptr, size_t size) noexcept;

#endif
