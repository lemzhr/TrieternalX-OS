/* File: src/kernel/vga.cpp */
#include "vga.h"
#include "io.h"
#include <cstdint>

// Alamat memori video
const size_t VGA_WIDTH = 80;
const size_t VGA_HEIGHT = 25;
const uintptr_t VGA_MEMORY = 0xB8000;

// Inisialisasi terminal global
VGA::Terminal VGA::terminal;

/*
 * ==========================================================================
 * Implementasi Fungsi Kursor (Fungsi Baru)
 * ==========================================================================
 */

/**
 * Memperbarui posisi kursor hardware (yang berkedip).
 * Ini dilakukan dengan mengirimkan perintah ke port I/O VGA.
 */
void VGA::Terminal::update_cursor(int row, int col)
{
    // Hitung posisi linear kursor (0..1999)
    uint16_t pos = row * VGA_WIDTH + col;

    // Kirim perintah ke port 0x3D4
    outb(0x3D4, 0x0F);                  // Perintah: set 8 bit rendah dari posisi kursor
    outb(0x3D5, (uint8_t)(pos & 0xFF)); // Kirim 8 bit rendah

    outb(0x3D4, 0x0E);                         // Perintah: set 8 bit tinggi dari posisi kursor
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF)); // Kirim 8 bit tinggi
}

/*
 * ==========================================================================
 * Fungsi Terminal
 * ==========================================================================
 */

void VGA::Terminal::initialize()
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_buffer = (uint16_t *)VGA_MEMORY;
    set_color(COLOR_WHITE, COLOR_BLACK);
    clear(); // clear() sekarang juga akan memanggil update_cursor()
}

void VGA::Terminal::clear()
{
    uint8_t clear_color = make_color(COLOR_WHITE, COLOR_BLACK);
    uint16_t clear_entry = make_vga_entry(' ', clear_color);
    for (size_t y = 0; y < VGA_HEIGHT; y++)
    {
        for (size_t x = 0; x < VGA_WIDTH; x++)
        {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = clear_entry;
        }
    }
    terminal_row = 0;
    terminal_column = 0;

    // Reset kursor hardware ke 0,0
    update_cursor(terminal_row, terminal_column); // <-- Tambahkan ini
}

void VGA::Terminal::set_color(vga_color fg, vga_color bg)
{
    terminal_color = make_color(fg, bg);
}

uint8_t VGA::Terminal::make_color(vga_color fg, vga_color bg)
{
    return fg | (bg << 4);
}

uint16_t VGA::Terminal::make_vga_entry(char c, uint8_t color)
{
    uint16_t c16 = c;
    uint16_t color16 = color;
    return c16 | (color16 << 8);
}

void VGA::Terminal::scroll()
{
    // Geser semua baris ke atas satu per satu
    for (size_t y = 1; y < VGA_HEIGHT; y++)
    {
        for (size_t x = 0; x < VGA_WIDTH; x++)
        {
            const size_t index_to = (y - 1) * VGA_WIDTH + x;
            const size_t index_from = y * VGA_WIDTH + x;
            terminal_buffer[index_to] = terminal_buffer[index_from];
        }
    }

    // Bersihkan baris terakhir
    uint16_t clear_entry = make_vga_entry(' ', terminal_color);
    for (size_t x = 0; x < VGA_WIDTH; x++)
    {
        terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = clear_entry;
    }
}

void VGA::Terminal::putchar(char c)
{
    if (c == '\n')
    {
        // Jika newline, pindah ke baris baru
        terminal_column = 0;
        terminal_row++;
    }
    else
    {
        // Jika karakter biasa, cetak
        const size_t index = terminal_row * VGA_WIDTH + terminal_column;
        terminal_buffer[index] = make_vga_entry(c, terminal_color);
        terminal_column++;
    }

    // Jika kolom melebihi batas, pindah ke baris baru
    if (terminal_column == VGA_WIDTH)
    {
        terminal_column = 0;
        terminal_row++;
    }

    // Jika baris melebihi batas, scroll layar
    if (terminal_row == VGA_HEIGHT)
    {
        scroll();
        terminal_row = VGA_HEIGHT - 1; // Kembali ke baris terakhir
        terminal_column = 0;
    }

    // Perbarui posisi kursor hardware SETELAH mencetak
    update_cursor(terminal_row, terminal_column); // <-- Tambahkan ini
}

void VGA::Terminal::write(const char *data)
{
    for (size_t i = 0; data[i] != '\0'; i++)
    {
        putchar(data[i]);
    }
}
