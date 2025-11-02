/* File: src/drivers/keyboard.cpp */

#include "keyboard.h"
#include "io.h"  // Untuk inb() dan outb()
#include "pic.h" // Untuk PIC_sendEOI()
#include "vga.h" // Untuk VGA::terminal.putchar()
#include "types.h"

// Scancode map (US QWERTY, hanya lowercase, angka, dan beberapa simbol)
// Ini adalah versi yang sangat disederhanakan.
// 0x00 adalah 'tidak terdefinisi'
const char scancode_map[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Ini adalah C++ Handler yang akan dipanggil oleh Assembly Stub
extern "C" void keyboard_handler()
{
    uint8_t scancode = inb(0x60); // Baca scancode dari port keyboard

    // Scancode > 0x80 adalah "key release" (tombol dilepas)
    // Untuk shell sederhana, kita abaikan saja
    if (scancode < 0x80)
    {
        char c = scancode_map[scancode];
        if (c != 0)
        {
            // Cetak karakter ke layar!
            VGA::terminal.putchar(c);
        }
    }

    // PENTING: Kirim sinyal End-of-Interrupt (EOI) ke PIC
    // agar keyboard bisa mengirim interrupt lagi.
    PIC_sendEOI(1); // 1 adalah nomor IRQ untuk keyboard
}

// Fungsi inisialisasi (hanya untuk formalitas)
void init_keyboard()
{
    // Di sini kita bisa mengaktifkan IRQ 1 di PIC
    // Tapi untuk sekarang, kita biarkan default (sudah aktif)
    // (Handler-nya sudah didaftarkan di idt.cpp)
}
