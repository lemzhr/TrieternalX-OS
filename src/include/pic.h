
#ifndef PIC_H
#define PIC_H

#include "types.h"

void PIC_remap(int offset1, int offset2);
void PIC_sendEOI(uint8_t irq);

#endif
