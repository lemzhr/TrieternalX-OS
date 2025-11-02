/* File: src/include/io.h */
#ifndef IO_H
#define IO_H

#include "types.h" // Untuk uint16_t dan uint8_t

/**
 * Mengirim satu byte data ke port I/O.
 * @param port Port I/O (16-bit)
 * @param val Data (8-bit) yang akan dikirim
 */
static inline void outb(uint16_t port, uint8_t val)
{
    // "outb %0, %1" : Operasi assembly
    // : : "a"(val), "Nd"(port) : Menentukan input (val ke register 'a', port ke register 'N' atau 'd')
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/**
 * Membaca satu byte data dari port I/O.
 * @param port Port I/O (16-bit)
 * @return Data (8-bit) yang dibaca
 */
static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    // "=a"(ret) : Output (hasilnya disimpan di 'ret')
    // "Nd"(port): Input (port yang akan dibaca)
    asm volatile("inb %1, %0"
                 : "=a"(ret)
                 : "Nd"(port));
    return ret;
}

#endif // IO_H