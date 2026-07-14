
#include "speaker.h"
#include "io.h"
#include "scheduler.h"

void play_sound(uint32_t frequency)
{
    if (frequency == 0)
    {
        stop_sound();
        return;
    }

    uint32_t divisor = 1193182 / frequency;

    outb(0x43, 0xB6);
    outb(0x42, (uint8_t)(divisor & 0xFF));
    outb(0x42, (uint8_t)((divisor >> 8) & 0xFF));

    uint8_t status = inb(0x61);
    if ((status & 3) != 3)
    {
        outb(0x61, status | 3);
    }
}

void stop_sound()
{
    uint8_t status = inb(0x61) & 0xFC;
    outb(0x61, status);
}

void beep(uint32_t frequency, uint32_t duration_ms)
{
    play_sound(frequency);
    scheduler_sleep(duration_ms);
    stop_sound();
}
