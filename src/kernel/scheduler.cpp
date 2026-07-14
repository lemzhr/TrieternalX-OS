
#include "scheduler.h"
#include "kheap.h"
#include "pic.h"
#include "timer.h"
#include "vga.h"
#include "gdt.h"
#include "vmm.h"
#include "pmm.h"

static int num_digits(uint32_t n)
{
    if (n == 0) return 1;
    int count = 0;
    while (n > 0) { count++; n /= 10; }
    return count;
}

static void write_dec(uint32_t n)
{
    if (n == 0) { VGA::terminal.putchar('0'); return; }
    char buf[32];
    int i = 0;
    while (n > 0) { buf[i++] = (n % 10) + '0'; n /= 10; }
    for (int j = i - 1; j >= 0; j--) VGA::terminal.putchar(buf[j]);
}

static Task* task_list_head = nullptr;
static Task* task_list_tail = nullptr;
static Task* current_task = nullptr;
static Task* idle_task = nullptr;
static Task* main_kernel_task = nullptr;

static uint32_t next_task_id = 1;
static bool scheduler_active = false;

static void mystrcpy(char* dest, const char* src)
{
    int i = 0;
    while (src[i] != '\0')
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static void idle_task_function()
{
    while (1)
    {
        asm volatile("hlt");
    }
}

void scheduler_init()
{
    scheduler_active = false;

    main_kernel_task = new Task();
    main_kernel_task->id = next_task_id++;
    mystrcpy(main_kernel_task->name, "kernel_main");
    main_kernel_task->is_sleeping = false;
    main_kernel_task->sleep_ticks = 0;
    main_kernel_task->is_terminated = false;
    main_kernel_task->is_user = false;
    main_kernel_task->user_stack_base = 0;
    main_kernel_task->esp = 0;
    main_kernel_task->stack_base = 0;
    main_kernel_task->next = nullptr;
    main_kernel_task->state = TASK_RUNNING;
    main_kernel_task->parent_pid = -1;
    main_kernel_task->exit_code = 0;
    main_kernel_task->entry_point = 0;
    main_kernel_task->page_directory = nullptr;
    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++)
        main_kernel_task->fds[i].flags = 0;

    task_list_head = main_kernel_task;
    task_list_tail = main_kernel_task;
    current_task = main_kernel_task;

    idle_task = scheduler_create_task(idle_task_function, "idle", false);
}

void scheduler_start()
{

    scheduler_active = true;
}

Task* scheduler_create_task(void (*entry_point)(), const char* name, bool is_user)
{
    asm volatile("cli");

    Task* t = new Task();
    t->id = next_task_id++;
    mystrcpy(t->name, name);
    t->is_sleeping = false;
    t->sleep_ticks = 0;
    t->is_terminated = false;
    t->is_user = is_user;
    t->next = nullptr;
    t->state = TASK_RUNNING;
    t->parent_pid = current_task ? current_task->id : -1;
    t->exit_code = 0;
    t->entry_point = (uint32_t)entry_point;
    t->page_directory = nullptr;
    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++)
        t->fds[i].flags = 0;

    t->fds[0].flags = FD_FLAG_USED | FD_FLAG_CONSOLE;
    t->fds[0].inode = 0;
    t->fds[0].offset = 0;
    t->fds[1].flags = FD_FLAG_USED | FD_FLAG_CONSOLE;
    t->fds[1].inode = 1;
    t->fds[1].offset = 0;

    if (is_user)
    {

        t->page_directory = vmm_create_user_page_dir();

        t->stack_base = (uint32_t)kmalloc(4096);
        t->user_stack_base = (uint32_t)kmalloc(4096);

        uint32_t* saved_pd = nullptr;
        if (t->page_directory)
        {
            asm volatile("mov %%cr3, %0" : "=r"(saved_pd));
            vmm_switch_page_dir(t->page_directory);
        }

        extern void vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags);
        extern uint32_t vmm_get_phys_addr(uint32_t virt);

        uint32_t phys_ustack = vmm_get_phys_addr(t->user_stack_base);
        if (phys_ustack) vmm_map_page(t->user_stack_base, phys_ustack, 7);

        uint32_t code_page = (uint32_t)entry_point & 0xFFFFF000;
        uint32_t phys_code = vmm_get_phys_addr(code_page);
        if (phys_code) vmm_map_page(code_page, phys_code, 7);

        if (saved_pd)
            vmm_switch_page_dir(nullptr);

        uint32_t* stack = (uint32_t*)(t->stack_base + 4096);

        stack--; *stack = 0x23;
        stack--; *stack = (uint32_t)(t->user_stack_base + 4096);
        stack--; *stack = 0x202;
        stack--; *stack = 0x1B;
        stack--; *stack = (uint32_t)entry_point;

        for (int i = 0; i < 8; i++)
        {
            stack--;
            *stack = 0;
        }

        t->esp = (uint32_t)stack;
    }
    else
    {
        t->user_stack_base = 0;

        t->stack_base = (uint32_t)kmalloc(4096);
        uint32_t* stack = (uint32_t*)(t->stack_base + 4096);

        stack--; *stack = (uint32_t)scheduler_exit_task;
        stack--; *stack = 0x202;
        stack--; *stack = 0x08;
        stack--; *stack = (uint32_t)entry_point;

        for (int i = 0; i < 8; i++)
        {
            stack--;
            *stack = 0;
        }

        t->esp = (uint32_t)stack;
    }

    if (task_list_tail)
    {
        task_list_tail->next = t;
        task_list_tail = t;
    }
    else
    {
        task_list_head = t;
        task_list_tail = t;
    }

    asm volatile("sti");
    return t;
}

void scheduler_yield()
{

    asm volatile("int $0x20");
}

void scheduler_sleep(uint32_t ms)
{
    if (current_task && scheduler_active)
    {

        current_task->sleep_ticks = ms / 10;
        if (current_task->sleep_ticks == 0)
        {
            current_task->sleep_ticks = 1;
        }
        current_task->is_sleeping = true;

        scheduler_yield();
    }
}

void scheduler_exit_task()
{
    asm volatile("cli");
    if (current_task)
    {
        current_task->is_terminated = true;
    }
    asm volatile("sti");

    scheduler_yield();

    while (1) {}
}

Task* scheduler_get_current_task()
{
    return current_task;
}

static Task* get_next_task()
{
    if (!current_task) return nullptr;

    Task* next = current_task->next;
    if (!next) next = task_list_head;

    while (next != current_task)
    {
        if (!next->is_sleeping && !next->is_terminated)
        {
            return next;
        }
        next = next->next;
        if (!next) next = task_list_head;
    }

    if (!current_task->is_sleeping && !current_task->is_terminated)
    {
        return current_task;
    }

    return idle_task;
}

extern "C" uint32_t timer_handler(uint32_t esp)
{

    extern void timer_increment_tick();
    timer_increment_tick();

    Task* t = task_list_head;
    while (t)
    {
        if (t->is_sleeping)
        {
            if (t->sleep_ticks > 0)
            {
                t->sleep_ticks--;
            }
            if (t->sleep_ticks == 0)
            {
                t->is_sleeping = false;
            }
        }
        t = t->next;
    }

    t = task_list_head;
    Task* prev = nullptr;
    while (t)
    {
        if (t->is_terminated && t != current_task)
        {

            if (t->stack_base)
            {
                kfree((void*)t->stack_base);
                t->stack_base = 0;
            }
            if (t->user_stack_base)
            {
                kfree((void*)t->user_stack_base);
                t->user_stack_base = 0;
            }
            if (t->page_directory)
            {
                vmm_free_user_pages(t->page_directory);
                pmm_free_frame((void*)t->page_directory);
                t->page_directory = nullptr;
            }

            if (prev)
            {
                prev->next = t->next;
            }
            else
            {
                task_list_head = t->next;
            }

            if (t == task_list_tail)
            {
                task_list_tail = prev;
            }

            Task* temp = t;
            t = t->next;
            delete temp;
            continue;
        }
        prev = t;
        t = t->next;
    }

    PIC_sendEOI(0);

    if (!scheduler_active)
    {
        return esp;
    }

    if (current_task)
    {
        current_task->esp = esp;
    }

    Task* next_task = get_next_task();
    if (next_task)
    {
        current_task = next_task;

        extern tss_entry_t tss_entry;
        tss_entry.esp0 = current_task->stack_base + 4096;

        if (current_task->page_directory)
            vmm_switch_page_dir(current_task->page_directory);
        else
            vmm_switch_page_dir(nullptr);

        return current_task->esp;
    }

    return esp;
}

int32_t process_get_pid()
{
    if (current_task)
        return (int32_t)current_task->id;
    return -1;
}

void process_exit(int32_t code)
{
    asm volatile("cli");
    if (current_task && current_task->id != 1)
    {
        current_task->exit_code = code;
        current_task->is_terminated = true;
        current_task->state = TASK_TERMINATED;
    }
    asm volatile("sti");
    scheduler_yield();
    while (1) {}
}

int32_t process_wait(int32_t pid)
{
    Task* t = task_list_head;
    while (t)
    {
        if ((int32_t)t->id == pid)
        {
            while (t->state != TASK_TERMINATED)
            {
                scheduler_yield();
            }
            return t->exit_code;
        }
        t = t->next;
    }
    return -1;
}

void process_kill(int32_t pid)
{
    asm volatile("cli");
    Task* t = task_list_head;
    while (t)
    {
        if ((int32_t)t->id == pid && t->id != 1)
        {
            t->is_terminated = true;
            t->state = TASK_TERMINATED;
            t->exit_code = -1;
            break;
        }
        t = t->next;
    }
    asm volatile("sti");
}

void process_list()
{
    VGA::terminal.set_color(VGA::COLOR_LIGHT_CYAN, VGA::COLOR_BLACK);
    VGA::terminal.write("  PID  NAME                 STATE       PARENT\n");
    VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);

    Task* t = task_list_head;
    while (t)
    {
        VGA::terminal.write("  ");
        write_dec(t->id);

        int pad = 7 - num_digits(t->id);
        for (int i = 0; i < pad; i++) VGA::terminal.putchar(' ');

        VGA::terminal.write(t->name);
        int name_len = 0;
        { const char* s = t->name; while (*s++) name_len++; }
        pad = 20 - name_len;
        for (int i = 0; i < pad; i++) VGA::terminal.putchar(' ');

        const char* state_str = "RUNNING";
        if (t->state == TASK_SLEEPING) state_str = "SLEEPING";
        else if (t->state == TASK_ZOMBIE) state_str = "ZOMBIE";
        else if (t->state == TASK_TERMINATED) state_str = "STOPPED";

        VGA::terminal.write(state_str);
        int st_len = 0;
        { const char* s = state_str; while (*s++) st_len++; }
        pad = 12 - st_len;
        for (int i = 0; i < pad; i++) VGA::terminal.putchar(' ');

        write_dec(t->parent_pid);
        VGA::terminal.putchar('\n');

        t = t->next;
    }
}

void switch_to_user_mode(uint32_t entry_point, uint32_t user_stack)
{
    extern tss_entry_t tss_entry;

    Task* current = scheduler_get_current_task();
    if (current)
    {
        tss_entry.esp0 = current->stack_base + 4096;
        current->is_user = true;
    }
    else
    {
        tss_entry.esp0 = 0;
    }

    asm volatile(
        "cli\n\t"
        "mov $0x23, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "pushl $0x23\n\t"
        "pushl %1\n\t"
        "pushf\n\t"
        "popl %%eax\n\t"
        "orl $0x200, %%eax\n\t"
        "pushl %%eax\n\t"
        "pushl $0x1B\n\t"
        "pushl %0\n\t"
        "iret\n\t"
        :
        : "r"(entry_point), "r"(user_stack)
        : "eax"
    );
}
