
#include "pmm.h"

static uint8_t pmm_bitmap[4096];
static size_t total_frames = 32768;
static size_t used_frames = 0;

static void set_bit(size_t bit)
{
    pmm_bitmap[bit / 8] |= (1 << (bit % 8));
}

static void clear_bit(size_t bit)
{
    pmm_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static bool test_bit(size_t bit)
{
    return (pmm_bitmap[bit / 8] & (1 << (bit % 8))) != 0;
}

void pmm_init(size_t mem_size)
{
    total_frames = mem_size / PAGE_SIZE;
    used_frames = 0;

    for (size_t i = 0; i < sizeof(pmm_bitmap); i++)
    {
        pmm_bitmap[i] = 0;
    }

    size_t reserved_frames = 1024;
    for (size_t i = 0; i < reserved_frames; i++)
    {
        set_bit(i);
        used_frames++;
    }
}

void* pmm_alloc_frame()
{
    for (size_t i = 0; i < total_frames; i++)
    {
        if (!test_bit(i))
        {
            set_bit(i);
            used_frames++;
            return (void*)(i * PAGE_SIZE);
        }
    }
    return nullptr;
}

void pmm_free_frame(void* frame)
{
    size_t bit = (size_t)frame / PAGE_SIZE;
    if (test_bit(bit))
    {
        clear_bit(bit);
        used_frames--;
    }
}

size_t pmm_get_free_frames()
{
    return total_frames - used_frames;
}

size_t pmm_get_used_frames()
{
    return used_frames;
}
