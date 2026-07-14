
#include "timer.h"
#include "io.h"

static uint32_t timer_ticks = 0;

void timer_init(uint32_t frequency)
{

    uint32_t divisor = 1193180 / frequency;

    outb(0x43, 0x36);

    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

uint32_t timer_get_ticks()
{
    return timer_ticks;
}

extern "C" void timer_increment_tick()
{
    timer_ticks++;
}
