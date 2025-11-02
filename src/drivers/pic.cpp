/* File: src/drivers/pic.cpp */

#include "pic.h"
#include "io.h" // Untuk outb()

#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI 0x20 // End-of-Interrupt

// Jeda I/O singkat
static inline void io_wait()
{
    outb(0x80, 0);
}

// Mengirim sinyal End-of-Interrupt (EOI) ke PIC
void PIC_sendEOI(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_CMD, PIC_EOI); // Kirim EOI ke PIC Slave
    outb(PIC1_CMD, PIC_EOI);     // Kirim EOI ke PIC Master
}

// Fungsi untuk me-remap PIC
void PIC_remap(int offset1, int offset2)
{
    // Simpan mask (status interrupt yang di-disable)
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    // Mulai urutan inisialisasi (ICW1)
    outb(PIC1_CMD, 0x11);
    io_wait();
    outb(PIC2_CMD, 0x11);
    io_wait();

    // Set offset vector (ICW2)
    outb(PIC1_DATA, offset1); // Master PIC: IRQ 0-7 -> offset1
    io_wait();
    outb(PIC2_DATA, offset2); // Slave PIC: IRQ 8-15 -> offset2
    io_wait();

    // Beri tahu Master PIC bahwa ada Slave di IRQ 2 (ICW3)
    outb(PIC1_DATA, 4);
    io_wait();
    // Beri tahu Slave PIC "cascade identity" (ICW3)
    outb(PIC2_DATA, 2);
    io_wait();

    // Set 8086 mode (ICW4)
    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

    // Kembalikan mask yang disimpan
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}
