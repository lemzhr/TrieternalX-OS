/* File: src/include/pic.h */
#ifndef PIC_H
#define PIC_H

#include "types.h"

// Fungsi untuk me-remap PIC
void PIC_remap(int offset1, int offset2);
void PIC_sendEOI(uint8_t irq);

#endif // PIC_H
