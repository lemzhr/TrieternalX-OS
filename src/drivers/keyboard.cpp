
#include "keyboard.h"
#include "io.h"
#include "pic.h"
#include "vga.h"
#include "types.h"
#include "shell.h"
#include "gui.h"

const char scancode_map[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#include "scheduler.h"

static volatile char last_char = 0;
static volatile bool has_char = false;

extern "C" void keyboard_handler()
{
    uint8_t scancode = inb(0x60);

    if (scancode < 0x80)
    {
        char c = scancode_map[scancode];
        if (c != 0)
        {
            last_char = c;
            has_char = true;
        }

        if (is_in_gui_mode())
        {

            gui_handle_key(scancode, c);
        }
        else if (c != 0)
        {

            shell_input(c);
        }
    }

    PIC_sendEOI(1);
}

void init_keyboard()
{
    last_char = 0;
    has_char = false;
}

char keyboard_get_key()
{
    while (!has_char)
    {

        scheduler_yield();
    }
    char c = last_char;
    has_char = false;
    return c;
}

bool keyboard_has_key()
{
    return has_char;
}
