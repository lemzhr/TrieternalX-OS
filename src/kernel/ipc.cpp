
#include "ipc.h"
#include "kheap.h"
#include "scheduler.h"
#include "vga.h"

static IPCMessage msg_pool[IPC_MAX_MESSAGES];
static IPCMailbox mailboxes[IPC_MAX_MAILBOXES];
static int mailbox_count = 0;

static void write_dec(uint32_t n)
{
    if (n == 0) { VGA::terminal.putchar('0'); return; }
    char buf[32];
    int i = 0;
    while (n > 0) { buf[i++] = (n % 10) + '0'; n /= 10; }
    for (int j = i - 1; j >= 0; j--) VGA::terminal.putchar(buf[j]);
}

void ipc_init()
{
    for (int i = 0; i < IPC_MAX_MESSAGES; i++)
    {
        msg_pool[i].used = false;
        msg_pool[i].next = nullptr;
    }
    for (int i = 0; i < IPC_MAX_MAILBOXES; i++)
    {
        mailboxes[i].used = false;
        mailboxes[i].head = nullptr;
        mailboxes[i].tail = nullptr;
        mailboxes[i].count = 0;
    }
    mailbox_count = 0;
}

static IPCMessage* alloc_message()
{
    for (int i = 0; i < IPC_MAX_MESSAGES; i++)
    {
        if (!msg_pool[i].used)
        {
            msg_pool[i].used = true;
            msg_pool[i].next = nullptr;
            return &msg_pool[i];
        }
    }
    return nullptr;
}

static void free_message(IPCMessage* msg)
{
    if (msg)
    {
        msg->used = false;
        msg->next = nullptr;
    }
}

int32_t ipc_send(int32_t dest_pid, const void* data, uint32_t length)
{
    if (length > IPC_MAX_MSG_SIZE)
        length = IPC_MAX_MSG_SIZE;

    IPCMessage* msg = alloc_message();
    if (!msg)
        return -1;

    msg->sender_pid = process_get_pid();
    msg->receiver_pid = dest_pid;
    msg->length = length;

    const uint8_t* src = (const uint8_t*)data;
    for (uint32_t i = 0; i < length; i++)
        msg->data[i] = src[i];

    for (int i = 0; i < IPC_MAX_MAILBOXES; i++)
    {
        if (mailboxes[i].used && mailboxes[i].owner_pid == dest_pid)
        {
            msg->next = nullptr;
            if (mailboxes[i].tail)
                mailboxes[i].tail->next = msg;
            else
                mailboxes[i].head = msg;
            mailboxes[i].tail = msg;
            mailboxes[i].count++;
            return 0;
        }
    }

    free_message(msg);
    return -1;
}

int32_t ipc_receive(int32_t* sender_pid, void* buffer, uint32_t max_length)
{
    int32_t my_pid = process_get_pid();

    for (int i = 0; i < IPC_MAX_MAILBOXES; i++)
    {
        if (mailboxes[i].used && mailboxes[i].owner_pid == my_pid)
        {
            if (mailboxes[i].head)
            {
                IPCMessage* msg = mailboxes[i].head;
                mailboxes[i].head = msg->next;
                if (!mailboxes[i].head)
                    mailboxes[i].tail = nullptr;
                mailboxes[i].count--;

                if (sender_pid)
                    *sender_pid = msg->sender_pid;

                uint32_t to_copy = msg->length;
                if (to_copy > max_length)
                    to_copy = max_length;

                uint8_t* dst = (uint8_t*)buffer;
                for (uint32_t i = 0; i < to_copy; i++)
                    dst[i] = msg->data[i];

                int32_t result = (int32_t)msg->length;
                free_message(msg);
                return result;
            }
        }
    }
    return -1;
}

int32_t ipc_mailbox_create(int32_t pid)
{
    for (int i = 0; i < IPC_MAX_MAILBOXES; i++)
    {
        if (!mailboxes[i].used)
        {
            mailboxes[i].used = true;
            mailboxes[i].owner_pid = pid;
            mailboxes[i].head = nullptr;
            mailboxes[i].tail = nullptr;
            mailboxes[i].count = 0;
            mailbox_count++;
            return 0;
        }
    }
    return -1;
}

int32_t ipc_mailbox_send(int32_t dest_pid, const void* data, uint32_t length)
{
    return ipc_send(dest_pid, data, length);
}

int32_t ipc_mailbox_recv(int32_t mailbox_pid, int32_t* sender_pid, void* buffer, uint32_t max_length)
{
    for (int i = 0; i < IPC_MAX_MAILBOXES; i++)
    {
        if (mailboxes[i].used && mailboxes[i].owner_pid == mailbox_pid)
        {
            if (mailboxes[i].head)
            {
                IPCMessage* msg = mailboxes[i].head;
                mailboxes[i].head = msg->next;
                if (!mailboxes[i].head)
                    mailboxes[i].tail = nullptr;
                mailboxes[i].count--;

                if (sender_pid)
                    *sender_pid = msg->sender_pid;

                uint32_t to_copy = msg->length;
                if (to_copy > max_length)
                    to_copy = max_length;

                uint8_t* dst = (uint8_t*)buffer;
                for (uint32_t i = 0; i < to_copy; i++)
                    dst[i] = msg->data[i];

                int32_t result = (int32_t)msg->length;
                free_message(msg);
                return result;
            }
        }
    }
    return -1;
}

static void ipc_test_receiver()
{
    ipc_mailbox_create(process_get_pid());

    VGA::terminal.set_color(VGA::COLOR_LIGHT_CYAN, VGA::COLOR_BLACK);
    VGA::terminal.write("[IPC Receiver] Mailbox created. Waiting for message...\n");
    VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);

    for (int attempt = 0; attempt < 50; attempt++)
    {
        int32_t sender = 0;
        char buf[256];
        int32_t result = ipc_receive(&sender, buf, 255);
        if (result > 0)
        {
            buf[result] = '\0';
            VGA::terminal.set_color(VGA::COLOR_LIGHT_GREEN, VGA::COLOR_BLACK);
            VGA::terminal.write("[IPC Receiver] Got message from PID ");
            write_dec(sender);
            VGA::terminal.write(": ");
            VGA::terminal.write(buf);
            VGA::terminal.write("\n");
            VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
            return;
        }
        scheduler_sleep(100);
    }

    VGA::terminal.set_color(VGA::COLOR_LIGHT_RED, VGA::COLOR_BLACK);
    VGA::terminal.write("[IPC Receiver] Timed out waiting for message.\n");
    VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
}

static void ipc_test_sender()
{
    VGA::terminal.set_color(VGA::COLOR_LIGHT_BLUE, VGA::COLOR_BLACK);
    VGA::terminal.write("[IPC Sender] Sending message to PID 2 (receiver)...\n");
    VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);

    scheduler_sleep(200);

    const char* msg = "Hello from sender!";
    uint32_t len = 0;
    { const char* s = msg; while (*s++) len++; }

    int32_t result = ipc_send(2, msg, len);
    if (result == 0)
    {
        VGA::terminal.set_color(VGA::COLOR_LIGHT_GREEN, VGA::COLOR_BLACK);
        VGA::terminal.write("[IPC Sender] Message sent successfully!\n");
        VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
    }
    else
    {
        VGA::terminal.set_color(VGA::COLOR_LIGHT_RED, VGA::COLOR_BLACK);
        VGA::terminal.write("[IPC Sender] Failed to send message.\n");
        VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
    }
}

void ipc_test()
{
    VGA::terminal.write("Menguji IPC (Inter-Process Communication)...\n");
    scheduler_create_task(ipc_test_receiver, "IPC_Recv", false);
    scheduler_create_task(ipc_test_sender, "IPC_Send", false);
}
