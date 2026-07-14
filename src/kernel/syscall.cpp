
#include "syscall.h"
#include "vga.h"
#include "keyboard.h"
#include "kheap.h"
#include "scheduler.h"

extern "C" void syscall_handler(SyscallRegisters* regs)
{
    uint32_t syscall_num = regs->eax;
    uint32_t arg1 = regs->ebx;

    switch (syscall_num)
    {
        case SYS_WRITE:
        {
            const char* str = (const char*)arg1;
            VGA::terminal.write(str);
            regs->eax = 0;
            break;
        }
        case SYS_READ_KEY:
        {
            regs->eax = (uint32_t)keyboard_get_key();
            break;
        }
        case SYS_MALLOC:
        {
            regs->eax = (uint32_t)kmalloc(arg1);
            break;
        }
        case SYS_FREE:
        {
            kfree((void*)arg1);
            regs->eax = 0;
            break;
        }
        case SYS_SLEEP:
        {
            scheduler_sleep(arg1);
            regs->eax = 0;
            break;
        }
        case SYS_EXIT:
        {
            scheduler_exit_task();
            regs->eax = 0;
            break;
        }
        default:
            VGA::terminal.set_color(VGA::COLOR_LIGHT_RED, VGA::COLOR_BLACK);
            VGA::terminal.write("[SYSCALL] Error: Syscall tidak dikenal!\n");
            VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
            regs->eax = -1;
            break;
    }
}

void syscall_init()
{

}
