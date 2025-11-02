/* File: src/include/gdt.h */
#ifndef GDT_H
#define GDT_H

#include "types.h"
#include <stdint.h>

// Struktur untuk satu entri GDT
struct gdt_entry_t
{
    uint16_t limit_low;    // 16 bit limit
    uint16_t base_low;     // 16 bit base
    uint8_t base_middle;   // 8 bit base
    uint8_t access;        // Bendera akses (tipe, ring, dll.)
    uint8_t granularity;   // Limit (lanjutan) dan bendera granularitas
    uint8_t base_high;     // 8 bit base
} __attribute__((packed)); // Mencegah compiler mengubah padding

// Struktur untuk GDT pointer
struct gdt_ptr_t
{
    uint16_t limit; // Ukuran GDT - 1
    uint32_t base;  // Alamat dari array gdt_entry_t
} __attribute__((packed));

// Fungsi inisialisasi GDT
void init_gdt();

#endif // GDT_H