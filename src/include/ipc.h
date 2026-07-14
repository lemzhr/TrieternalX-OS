
#ifndef IPC_H
#define IPC_H

#include "types.h"

#define IPC_MAX_MESSAGES  64
#define IPC_MAX_MSG_SIZE  256
#define IPC_MAX_MAILBOXES 16

struct IPCMessage {
    int32_t sender_pid;
    int32_t receiver_pid;
    uint32_t length;
    uint8_t data[IPC_MAX_MSG_SIZE];
    bool used;
    IPCMessage* next;
};

struct IPCMailbox {
    int32_t owner_pid;
    IPCMessage* head;
    IPCMessage* tail;
    uint32_t count;
    bool used;
};

void ipc_init();
int32_t ipc_send(int32_t dest_pid, const void* data, uint32_t length);
int32_t ipc_receive(int32_t* sender_pid, void* buffer, uint32_t max_length);
int32_t ipc_mailbox_create(int32_t pid);
int32_t ipc_mailbox_send(int32_t dest_pid, const void* data, uint32_t length);
int32_t ipc_mailbox_recv(int32_t mailbox_pid, int32_t* sender_pid, void* buffer, uint32_t max_length);
void ipc_test();

#endif
