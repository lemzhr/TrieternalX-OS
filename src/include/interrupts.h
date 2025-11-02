/* File: src/include/interrupts.h */
#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "types.h"

// Mendefinisikan struktur untuk satu entri IDT
struct idt_entry_t
{
    uint16_t base_lo;      // 16 bit alamat handler
    uint16_t sel;          // Segment selector
    uint8_t always0;       // Selalu nol
    uint8_t flags;         // Tipe entri dan atribut
    uint16_t base_hi;      // 16 bit alamat handler
} __attribute__((packed)); // Mencegah compiler mengubah padding

// Mendefinisikan pointer ke IDT
struct idt_ptr_t
{
    uint16_t limit;
    uint32_t base; // Alamat dari array idt_entry_t
} __attribute__((packed));

// Fungsi utama untuk inisialisasi IDT
void init_idt();

#endif // INTERRUPTS_H
