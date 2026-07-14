
#ifndef VBE_H
#define VBE_H

#include "types.h"
#include "multiboot.h"

void vbe_init(multiboot_info* mbi);
bool vbe_is_active();
uint32_t vbe_get_width();
uint32_t vbe_get_height();
void* vbe_get_framebuffer_addr();
uint32_t vbe_get_framebuffer_pitch();

void vbe_putpixel(int x, int y, uint32_t color);
void vbe_clear(uint32_t color);
void vbe_draw_rect(int x, int y, int w, int h, uint32_t color);
void vbe_draw_line(int x1, int y1, int x2, int y2, uint32_t color);
void vbe_draw_char(int x, int y, char c, uint32_t color);
void vbe_draw_string(int x, int y, const char* str, uint32_t color);

void vbe_console_write(const char* str);
void vbe_console_putchar(char c);
void vbe_console_clear();
void vbe_console_set_color(uint32_t fg, uint32_t bg);

#endif
