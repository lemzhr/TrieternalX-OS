
#ifndef SPEAKER_H
#define SPEAKER_H

#include "types.h"

void play_sound(uint32_t frequency);
void stop_sound();
void beep(uint32_t frequency, uint32_t duration_ms);

#endif
