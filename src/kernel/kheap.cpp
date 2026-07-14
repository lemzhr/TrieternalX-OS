
#include "kheap.h"
#include "pmm.h"
#include "vmm.h"

static HeapBlock* heap_start_block = nullptr;
static uint32_t heap_current_end = HEAP_START;

void kheap_init()
{
    heap_start_block = nullptr;
    heap_current_end = HEAP_START;
}

static bool heap_grow(size_t size)
{

    size_t total_needed = size + sizeof(HeapBlock);

    size_t pages_needed = (total_needed + PAGE_SIZE - 1) / PAGE_SIZE;

    for (size_t i = 0; i < pages_needed; i++)
    {
        if (heap_current_end >= HEAP_MAX)
        {
            return false;
        }

        void* phys = pmm_alloc_frame();
        if (!phys)
        {
            return false;
        }

        vmm_map_page(heap_current_end, (uint32_t)phys, 3);
        heap_current_end += PAGE_SIZE;
    }

    return true;
}

void* kmalloc(size_t size)
{
    if (size == 0) return nullptr;

    size = (size + 3) & ~3;

    HeapBlock* current = heap_start_block;
    HeapBlock* best_fit = nullptr;

    while (current)
    {
        if (current->is_free && current->size >= size)
        {
            if (!best_fit || current->size < best_fit->size)
            {
                best_fit = current;
            }
        }
        current = current->next;
    }

    if (best_fit)
    {

        if (best_fit->size >= size + sizeof(HeapBlock) + 4)
        {
            HeapBlock* new_block = (HeapBlock*)((uint32_t)best_fit + sizeof(HeapBlock) + size);
            new_block->size = best_fit->size - size - sizeof(HeapBlock);
            new_block->is_free = true;
            new_block->next = best_fit->next;
            new_block->prev = best_fit;

            if (best_fit->next)
            {
                best_fit->next->prev = new_block;
            }
            best_fit->next = new_block;
            best_fit->size = size;
        }
        best_fit->is_free = false;
        return (void*)((uint32_t)best_fit + sizeof(HeapBlock));
    }

    uint32_t old_end = heap_current_end;
    if (!heap_grow(size))
    {
        return nullptr;
    }

    HeapBlock* new_block = (HeapBlock*)old_end;
    new_block->size = heap_current_end - old_end - sizeof(HeapBlock);
    new_block->is_free = true;
    new_block->next = nullptr;
    new_block->prev = nullptr;

    if (!heap_start_block)
    {
        heap_start_block = new_block;
    }
    else
    {
        HeapBlock* last = heap_start_block;
        while (last->next)
        {
            last = last->next;
        }
        last->next = new_block;
        new_block->prev = last;
    }

    return kmalloc(size);
}

void kfree(void* ptr)
{
    if (!ptr) return;

    HeapBlock* block = (HeapBlock*)((uint32_t)ptr - sizeof(HeapBlock));
    block->is_free = true;

    if (block->next && block->next->is_free)
    {
        block->size += sizeof(HeapBlock) + block->next->size;
        block->next = block->next->next;
        if (block->next)
        {
            block->next->prev = block;
        }
    }

    if (block->prev && block->prev->is_free)
    {
        block->prev->size += sizeof(HeapBlock) + block->size;
        block->prev->next = block->next;
        if (block->next)
        {
            block->next->prev = block->prev;
        }
    }
}

void* operator new(size_t size) { return kmalloc(size); }
void* operator new[](size_t size) { return kmalloc(size); }
void operator delete(void* ptr) noexcept { kfree(ptr); }
void operator delete[](void* ptr) noexcept { kfree(ptr); }
void operator delete(void* ptr, size_t) noexcept { kfree(ptr); }
void operator delete[](void* ptr, size_t) noexcept { kfree(ptr); }

extern "C" void* memset(void* dest, int ch, size_t count)
{
    uint8_t* d = (uint8_t*)dest;
    for (size_t i = 0; i < count; i++)
        d[i] = (uint8_t)ch;
    return dest;
}

extern "C" void* memcpy(void* dest, const void* src, size_t count)
{
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < count; i++)
        d[i] = s[i];
    return dest;
}
