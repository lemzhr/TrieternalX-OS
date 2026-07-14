
#include "user_api.h"
#include "syscall.h"

static inline int syscall_trigger(uint32_t syscall_num, uint32_t arg1)
{
    int ret;
    asm volatile("int $0x80"
                 : "=a"(ret)
                 : "a"(syscall_num), "b"(arg1));
    return ret;
}

void api_print(const char* str)
{
    syscall_trigger(SYS_WRITE, (uint32_t)str);
}

char api_getkey()
{
    return (char)syscall_trigger(SYS_READ_KEY, 0);
}

void* api_malloc(size_t size)
{
    return (void*)syscall_trigger(SYS_MALLOC, size);
}

void api_free(void* ptr)
{
    syscall_trigger(SYS_FREE, (uint32_t)ptr);
}

void api_sleep(uint32_t ms)
{
    syscall_trigger(SYS_SLEEP, ms);
}

void api_exit()
{
    syscall_trigger(SYS_EXIT, 0);
}
