
#include "mouse.h"
#include "io.h"
#include "pic.h"
#include "logger.h"
#include "vbe.h"
#include "gui.h"

static int8_t mouse_cycle = 0;
static uint8_t mouse_byte[3];
static int32_t mouse_x = 400;
static int32_t mouse_y = 300;
static bool left_clicked = false;
static bool right_clicked = false;

static uint32_t saved_cursor_pixels[100];
static int last_drawn_x = -1;
static int last_drawn_y = -1;

void draw_mouse_cursor_vbe(int x, int y)
{
    if (!vbe_is_active()) return;

    if (last_drawn_x != -1)
    {
        for (int i = 0; i < 10; i++)
        {
            for (int j = 0; j < 10; j++)
            {
                int px = last_drawn_x + j;
                int py = last_drawn_y + i;
                if (px < 800 && py < 600)
                {
                    vbe_putpixel(px, py, saved_cursor_pixels[i * 10 + j]);
                }
            }
        }
    }

    uint32_t* fb = (uint32_t*)vbe_get_framebuffer_addr();
    uint32_t pitch = vbe_get_framebuffer_pitch() / 4;
    if (fb)
    {
        for (int i = 0; i < 10; i++)
        {
            for (int j = 0; j < 10; j++)
            {
                int px = x + j;
                int py = y + i;
                if (px < 800 && py < 600)
                {
                    saved_cursor_pixels[i * 10 + j] = fb[py * pitch + px];
                }
            }
        }
    }

    uint32_t color = 0xFFFFFF;
    uint32_t border = 0x000000;

    static const uint8_t cursor_map[10][10] = {
        {2,0,0,0,0,0,0,0,0,0},
        {2,2,0,0,0,0,0,0,0,0},
        {2,1,2,0,0,0,0,0,0,0},
        {2,1,1,2,0,0,0,0,0,0},
        {2,1,1,1,2,0,0,0,0,0},
        {2,1,1,1,1,2,0,0,0,0},
        {2,1,1,1,1,1,2,0,0,0},
        {2,1,1,2,2,2,2,2,0,0},
        {2,1,2,0,2,1,2,0,0,0},
        {2,2,0,0,0,2,2,0,0,0}
    };

    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            int px = x + j;
            int py = y + i;
            if (px < 800 && py < 600)
            {
                if (cursor_map[i][j] == 1)
                {
                    vbe_putpixel(px, py, color);
                }
                else if (cursor_map[i][j] == 2)
                {
                    vbe_putpixel(px, py, border);
                }
            }
        }
    }

    last_drawn_x = x;
    last_drawn_y = y;
}

void reset_mouse_cursor_vbe()
{
    last_drawn_x = -1;
    last_drawn_y = -1;
}

static void mouse_wait(uint8_t a_type)
{
    uint32_t timeout = 100000;
    if (a_type == 0)
    {
        while (timeout--)
        {
            if ((inb(0x64) & 1) == 1)
                return;
        }
    }
    else
    {
        while (timeout--)
        {
            if ((inb(0x64) & 2) == 0)
                return;
        }
    }
}

static void mouse_write(uint8_t a_write)
{
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, a_write);
}

static uint8_t mouse_read()
{
    mouse_wait(0);
    return inb(0x60);
}

void mouse_init()
{
    klog_info("MOUSE", "Initializing PS/2 Mouse Driver...");

    uint8_t status;

    mouse_wait(1);
    outb(0x64, 0xA8);

    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    status = inb(0x60) | 2;

    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);

    mouse_write(0xF6);
    mouse_read();

    mouse_write(0xF4);
    mouse_read();

    mouse_cycle = 0;
    mouse_x = 400;
    mouse_y = 300;

    last_drawn_x = -1;
    last_drawn_y = -1;

    klog_info("MOUSE", "PS/2 Mouse Driver initialized successfully.");
}

extern "C" void mouse_handler()
{
    uint8_t status = inb(0x64);
    if (!(status & 1))
    {
        PIC_sendEOI(12);
        return;
    }
    if (!(status & 0x20))
    {
        PIC_sendEOI(12);
        return;
    }

    uint8_t data = inb(0x60);
    switch (mouse_cycle)
    {
        case 0:
            mouse_byte[0] = data;
            if (data & 0x08)
                mouse_cycle = 1;
            break;
        case 1:
            mouse_byte[1] = data;
            mouse_cycle = 2;
            break;
        case 2:
            mouse_byte[2] = data;
            mouse_cycle = 0;

            left_clicked = (mouse_byte[0] & 0x01) != 0;
            right_clicked = (mouse_byte[0] & 0x02) != 0;

            int32_t x_rel = (int32_t)mouse_byte[1];
            int32_t y_rel = (int32_t)mouse_byte[2];

            if (mouse_byte[0] & 0x10) x_rel |= 0xFFFFFF00;
            if (mouse_byte[0] & 0x20) y_rel |= 0xFFFFFF00;

            mouse_x += x_rel;
            mouse_y -= y_rel;

            if (mouse_x < 0) mouse_x = 0;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_x > 799) mouse_x = 799;
            if (mouse_y > 599) mouse_y = 599;

            if (is_in_gui_mode() && vbe_is_active())
            {
                draw_mouse_cursor_vbe(mouse_x, mouse_y);
            }
            break;
    }

    PIC_sendEOI(12);
}

int32_t mouse_get_x() { return mouse_x; }
int32_t mouse_get_y() { return mouse_y; }
bool mouse_is_left_clicked() { return left_clicked; }
bool mouse_is_right_clicked() { return right_clicked; }
