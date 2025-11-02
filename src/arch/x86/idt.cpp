/* File: src/arch/x86/idt.cpp */

#include "interrupts.h"
#include "pic.h" // Kita perlu me-remap PIC
#include "vga.h" // Untuk mencetak pesan debug

// Deklarasi array IDT (256 entri)
idt_entry_t idt_entries[256];
idt_ptr_t idt_ptr;

// Memberi tahu C++ bahwa fungsi ini ada, tapi didefinisikan di
// file Assembly (isrs.asm). Ini adalah handler keyboard kita.
extern "C" void isr_stub_33();

// Fungsi eksternal untuk me-load IDT (didefinisikan di assembly,
// atau kita bisa buat inline di sini)
static inline void idt_load(idt_ptr_t *ptr)
{
    asm volatile("lidt %0" : : "m"(*ptr));
}

// Helper untuk mengatur satu entri (gate) di IDT
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    idt_entries[num].base_lo = (base & 0xFFFF);
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags = flags /* | 0x60 untuk user mode */;
}

// Fungsi inisialisasi IDT utama
void init_idt()
{
    // 1. Remap PIC agar interrupt hardware (IRQ) tidak
    //    bertabrakan dengan CPU exceptions.
    PIC_remap(0x20, 0x28); // Remap IRQ 0-7 ke Int 32-39
                           // Remap IRQ 8-15 ke Int 40-47

    // 2. Siapkan IDT pointer
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base = (uint32_t)&idt_entries;

    // 3. Atur gate untuk keyboard (IRQ 1)
    //    Setelah remap, IRQ 1 menjadi Interrupt 33 (0x21 + 1 = 33)
    //    Kita arahkan ke 'isr_stub_33' yang ada di isrs.asm
    //    Flags 0x8E: Present (1), Ring 0 (00), Interrupt Gate (1110)
    idt_set_gate(33, (uint32_t)&isr_stub_33, 0x08, 0x8E);

    // 4. Muat IDT ke CPU
    idt_load(&idt_ptr);

    VGA::terminal.set_color(VGA::COLOR_LIGHT_CYAN, VGA::COLOR_BLACK);
    VGA::terminal.write("[INFO] IDT dan PIC berhasil diinisialisasi.\n");
}
