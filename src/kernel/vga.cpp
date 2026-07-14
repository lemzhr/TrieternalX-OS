
#include "vga.h"
#include "io.h"
#include "vbe.h"
#include <cstdint>

const size_t VGA_WIDTH = 80;
const size_t VGA_HEIGHT = 25;
const uintptr_t VGA_MEMORY = 0xB8000;

VGA::Terminal VGA::terminal;

static uint32_t vga_to_rgb(VGA::vga_color c)
{
    switch (c)
    {
        case VGA::COLOR_BLACK:         return 0x000000;
        case VGA::COLOR_BLUE:          return 0x0000AA;
        case VGA::COLOR_GREEN:         return 0x00AA00;
        case VGA::COLOR_CYAN:          return 0x00AAAA;
        case VGA::COLOR_RED:           return 0xAA0000;
        case VGA::COLOR_MAGENTA:       return 0xAA00AA;
        case VGA::COLOR_BROWN:         return 0xAA5500;
        case VGA::COLOR_LIGHT_GREY:    return 0xAAAAAA;
        case VGA::COLOR_DARK_GREY:     return 0x555555;
        case VGA::COLOR_LIGHT_BLUE:    return 0x5555FF;
        case VGA::COLOR_LIGHT_GREEN:   return 0x55FF55;
        case VGA::COLOR_LIGHT_CYAN:    return 0x55FFFF;
        case VGA::COLOR_LIGHT_RED:     return 0xFF5555;
        case VGA::COLOR_LIGHT_MAGENTA: return 0xFF55FF;
        case VGA::COLOR_LIGHT_BROWN:   return 0xFFFF55;
        case VGA::COLOR_WHITE:         return 0xFFFFFF;
        default:                       return 0xFFFFFF;
    }
}

void VGA::Terminal::update_cursor(int row, int col)
{
    if (vbe_is_active()) return;

    uint16_t pos = row * VGA_WIDTH + col;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void VGA::Terminal::initialize()
{
    if (vbe_is_active())
    {
        vbe_console_clear();
        return;
    }

    terminal_row = 0;
    terminal_column = 0;
    terminal_buffer = (uint16_t *)VGA_MEMORY;
    set_color(COLOR_WHITE, COLOR_BLACK);
    clear();
}

void VGA::Terminal::clear()
{
    if (vbe_is_active())
    {
        vbe_console_clear();
        return;
    }

    uint8_t clear_color = make_color(COLOR_WHITE, COLOR_BLACK);
    uint16_t clear_entry = make_vga_entry(' ', clear_color);
    for (size_t y = 0; y < VGA_HEIGHT; y++)
    {
        for (size_t x = 0; x < VGA_WIDTH; x++)
        {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = clear_entry;
        }
    }
    terminal_row = 0;
    terminal_column = 0;

    update_cursor(terminal_row, terminal_column);
}

void VGA::Terminal::set_color(vga_color fg, vga_color bg)
{
    if (vbe_is_active())
    {
        vbe_console_set_color(vga_to_rgb(fg), vga_to_rgb(bg));
        return;
    }
    terminal_color = make_color(fg, bg);
}

uint8_t VGA::Terminal::make_color(vga_color fg, vga_color bg)
{
    return fg | (bg << 4);
}

uint16_t VGA::Terminal::make_vga_entry(char c, uint8_t color)
{
    uint16_t c16 = c;
    uint16_t color16 = color;
    return c16 | (color16 << 8);
}

void VGA::Terminal::scroll()
{
    if (vbe_is_active()) return;

    for (size_t y = 1; y < VGA_HEIGHT; y++)
    {
        for (size_t x = 0; x < VGA_WIDTH; x++)
        {
            const size_t index_to = (y - 1) * VGA_WIDTH + x;
            const size_t index_from = y * VGA_WIDTH + x;
            terminal_buffer[index_to] = terminal_buffer[index_from];
        }
    }

    uint16_t clear_entry = make_vga_entry(' ', terminal_color);
    for (size_t x = 0; x < VGA_WIDTH; x++)
    {
        terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = clear_entry;
    }
}

void VGA::Terminal::putchar(char c)
{
    if (vbe_is_active())
    {
        vbe_console_putchar(c);
        return;
    }

    if (c == '\n')
    {
        terminal_column = 0;
        terminal_row++;
    }
    else if (c == '\b')
    {
        if (terminal_column > 0)
        {
            terminal_column--;
            const size_t index = terminal_row * VGA_WIDTH + terminal_column;
            terminal_buffer[index] = make_vga_entry(' ', terminal_color);
        }
    }
    else
    {
        const size_t index = terminal_row * VGA_WIDTH + terminal_column;
        terminal_buffer[index] = make_vga_entry(c, terminal_color);
        terminal_column++;
    }

    if (c != '\b' && terminal_column == VGA_WIDTH)
    {
        terminal_column = 0;
        terminal_row++;
    }

    if (terminal_row == VGA_HEIGHT)
    {
        scroll();
        terminal_row = VGA_HEIGHT - 1;
        terminal_column = 0;
    }

    update_cursor(terminal_row, terminal_column);
}

void VGA::Terminal::write(const char *data)
{
    for (size_t i = 0; data[i] != '\0'; i++)
    {
        putchar(data[i]);
    }
}

uint8_t VGA::Terminal::get_color()
{
    return terminal_color;
}
