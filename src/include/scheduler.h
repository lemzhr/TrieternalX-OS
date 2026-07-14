
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"

enum TaskState {
    TASK_RUNNING,
    TASK_SLEEPING,
    TASK_ZOMBIE,
    TASK_TERMINATED
};

#define MAX_FILE_DESCRIPTORS 16
#define FD_FLAG_USED 0x1
#define FD_FLAG_CONSOLE 0x2

struct FileDescriptor {
    uint32_t flags;
    uint32_t inode;
    uint32_t offset;
};

struct Task
{
    uint32_t id;
    uint32_t esp;
    uint32_t stack_base;
    uint32_t user_stack_base;
    uint32_t sleep_ticks;
    bool is_sleeping;
    bool is_terminated;
    bool is_user;
    char name[32];
    Task* next;

    TaskState state;
    int32_t parent_pid;
    int32_t exit_code;
    uint32_t entry_point;
    FileDescriptor fds[MAX_FILE_DESCRIPTORS];
    uint32_t* page_directory;
};

void scheduler_init();
void scheduler_start();
Task* scheduler_create_task(void (*entry_point)(), const char* name, bool is_user = false);
void scheduler_yield();
void scheduler_sleep(uint32_t ms);
void scheduler_exit_task();
Task* scheduler_get_current_task();
void switch_to_user_mode(uint32_t entry_point, uint32_t user_stack);

int32_t process_get_pid();
void process_exit(int32_t code);
int32_t process_wait(int32_t pid);
void process_kill(int32_t pid);
void process_list();

#endif
