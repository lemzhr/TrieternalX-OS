/* File: src/arch/x86/gdt.cpp */

#include "gdt.h"

// Deklarasi GDT (3 entri: Null, Kernel Code, Kernel Data)
gdt_entry_t gdt_entries[3];
gdt_ptr_t gdt_ptr;

// Fungsi C eksternal dari gdt_asm.asm
extern "C" void gdt_flush(gdt_ptr_t *gdt_ptr);

// Helper untuk mengatur satu entri GDT
void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt_entries[num].base_low = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access = access;
}

// Fungsi inisialisasi GDT utama
void init_gdt()
{
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 3) - 1;
    gdt_ptr.base = (uint32_t)&gdt_entries;

    // 1. Null Descriptor (Wajib) - Offset 0x00
    gdt_set_gate(0, 0, 0, 0, 0);

    // 2. Kernel Code Segment - Offset 0x08
    //    base=0, limit=4GB, access=0x9A (present, ring0, code, readable)
    //    gran=0xCF (4KB granularity, 32-bit)
    //    Ini adalah segmen yang dirujuk oleh IDT Anda (0x08)!
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // 3. Kernel Data Segment - Offset 0x10
    //    base=0, limit=4GB, access=0x92 (present, ring0, data, writable)
    //    gran=0xCF (4KB granularity, 32-bit)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // Muat GDT baru
    gdt_flush(&gdt_ptr);
}
