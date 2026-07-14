
#include "logger.h"
#include "vga.h"
#include "timer.h"

static void write_dec(uint32_t n, int width = 0)
{
    char buf[32];
    int i = 0;
    if (n == 0)
    {
        buf[i++] = '0';
    }
    else
    {
        while (n > 0)
        {
            buf[i++] = (n % 10) + '0';
            n /= 10;
        }
    }

    while (i < width)
    {
        buf[i++] = '0';
    }

    for (int j = i - 1; j >= 0; j--)
    {
        VGA::terminal.putchar(buf[j]);
    }
}

void logger_init()
{

}

void klog(LogLevel level, const char* component, const char* message)
{

    uint32_t ticks = timer_get_ticks();
    uint32_t sec = ticks / 100;
    uint32_t csec = ticks % 100;

    uint8_t old_color = VGA::terminal.get_color();

    VGA::terminal.set_color(VGA::COLOR_LIGHT_GREY, VGA::COLOR_BLACK);
    VGA::terminal.write("[");
    write_dec(sec, 5);
    VGA::terminal.write(".");
    write_dec(csec, 2);
    VGA::terminal.write("] ");

    switch (level)
    {
        case LOG_LEVEL_DEBUG:
            VGA::terminal.set_color(VGA::COLOR_LIGHT_BLUE, VGA::COLOR_BLACK);
            VGA::terminal.write("[DEBUG] ");
            break;
        case LOG_LEVEL_INFO:
            VGA::terminal.set_color(VGA::COLOR_LIGHT_GREEN, VGA::COLOR_BLACK);
            VGA::terminal.write("[INFO]  ");
            break;
        case LOG_LEVEL_WARNING:
            VGA::terminal.set_color(VGA::COLOR_LIGHT_BROWN, VGA::COLOR_BLACK);
            VGA::terminal.write("[WARN]  ");
            break;
        case LOG_LEVEL_ERROR:
            VGA::terminal.set_color(VGA::COLOR_LIGHT_RED, VGA::COLOR_BLACK);
            VGA::terminal.write("[ERROR] ");
            break;
    }

    if (component)
    {
        VGA::terminal.set_color(VGA::COLOR_LIGHT_CYAN, VGA::COLOR_BLACK);
        VGA::terminal.write("[");
        VGA::terminal.write(component);
        VGA::terminal.write("] ");
    }

    VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
    VGA::terminal.write(message);
    VGA::terminal.write("\n");

    VGA::terminal.set_color((VGA::vga_color)(old_color & 0x0F), (VGA::vga_color)((old_color >> 4) & 0x0F));
}
