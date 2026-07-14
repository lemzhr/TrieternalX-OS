
#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

#define SYS_WRITE      1
#define SYS_READ_KEY   2
#define SYS_MALLOC     3
#define SYS_FREE       4
#define SYS_SLEEP      5
#define SYS_EXIT       6

struct SyscallRegisters
{
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags;
};

void syscall_init();

#endif
