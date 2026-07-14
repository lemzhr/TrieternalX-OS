
#ifndef VGA_H
#define VGA_H

#include "types.h"

namespace VGA
{

    enum vga_color
    {
        COLOR_BLACK = 0,
        COLOR_BLUE = 1,
        COLOR_GREEN = 2,
        COLOR_CYAN = 3,
        COLOR_RED = 4,
        COLOR_MAGENTA = 5,
        COLOR_BROWN = 6,
        COLOR_LIGHT_GREY = 7,
        COLOR_DARK_GREY = 8,
        COLOR_LIGHT_BLUE = 9,
        COLOR_LIGHT_GREEN = 10,
        COLOR_LIGHT_CYAN = 11,
        COLOR_LIGHT_RED = 12,
        COLOR_LIGHT_MAGENTA = 13,
        COLOR_LIGHT_BROWN = 14,
        COLOR_WHITE = 15,
    };

    class Terminal
    {
    public:
        void initialize();
        void set_color(vga_color fg, vga_color bg);
        void putchar(char c);
        void write(const char *data);
        void clear();
        uint8_t get_color();

    private:
        void scroll();
        uint8_t make_color(vga_color fg, vga_color bg);
        uint16_t make_vga_entry(char c, uint8_t color);

        void update_cursor(int row, int col);

        size_t terminal_row;
        size_t terminal_column;
        uint8_t terminal_color;
        uint16_t *terminal_buffer;
    };

    extern Terminal terminal;

}

#endif
