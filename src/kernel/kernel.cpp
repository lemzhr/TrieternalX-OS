/* File: src/kernel/kernel.cpp */

#include "vga.h"
#include "gdt.h"        // <-- Memastikan GDT di-include
#include "interrupts.h" // <-- Memastikan IDT di-include
#include "keyboard.h"   // <-- Memastikan Keyboard di-include
#include "pic.h"        // <-- Memastikan PIC di-include

/*
 * ==========================================================================
 * Helper Functions untuk Boot Log (Gaya Linux)
 * ==========================================================================
 */

// Fungsi internal untuk mencetak status
void print_status(const char *status, VGA::vga_color color)
{
    VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
    VGA::terminal.write("[");
    VGA::terminal.set_color(color, VGA::COLOR_BLACK);
    VGA::terminal.write(status);
    VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
    VGA::terminal.write("] ");
}

// Mencetak pesan [ OK ] (Hijau)
void boot_log_ok(const char *message)
{
    print_status(" OK ", VGA::COLOR_LIGHT_GREEN);
    VGA::terminal.set_color(VGA::COLOR_LIGHT_GREY, VGA::COLOR_BLACK);
    VGA::terminal.write(message);
    VGA::terminal.write("\n");
}

// Mencetak pesan [INFO] (Cyan)
void boot_log_info(const char *message)
{
    print_status("INFO", VGA::COLOR_LIGHT_CYAN);
    VGA::terminal.set_color(VGA::COLOR_LIGHT_GREY, VGA::COLOR_BLACK);
    VGA::terminal.write(message);
    VGA::terminal.write("\n");
}

// Mencetak pesan [TODO] (Kuning/Coklat) untuk fitur skripsi Anda
void boot_log_todo(const char *message)
{
    print_status("TODO", VGA::COLOR_LIGHT_BROWN);
    VGA::terminal.set_color(VGA::COLOR_LIGHT_GREY, VGA::COLOR_BLACK);
    VGA::terminal.write(message);
    VGA::terminal.write("\n");
}

/*
 * ==========================================================================
 * Titik Masuk Kernel Utama (kmain)
 * ==========================================================================
 */
extern "C" void kmain()
{
    // 1. Inisialisasi terminal
    VGA::terminal.initialize();

    // 2. Gambar Logo ASCII Art
    VGA::terminal.set_color(VGA::COLOR_LIGHT_GREEN, VGA::COLOR_BLACK);
    VGA::terminal.write("\n");
    VGA::terminal.write("================================================================================\n");
    VGA::terminal.write(" _        _      _                  _          \n");
    VGA::terminal.write("| |_ _ __(_) ___| |_ ___ _ __ _ __   __ _| |_   __\n");
    VGA::terminal.write("| __| '__| |/ _ \\ __/ _ \\ '__| '_ \\ / _` | \\ \\ / /\n");
    VGA::terminal.write("| |_| |  | |  __/ ||  __/ |  | | | | (_| | |>  < \n");
    VGA::terminal.write(" \\__|_|  |_|\\___|\\__\\___|_|  |_| |_|\\__,_|_/_/\\_\\\n");
    VGA::terminal.write("                                                 \n");
    VGA::terminal.write("================================================================================\n\n");

    // 3. Tampilkan log boot "seperti Linux"
    boot_log_info("Bootloader menyerahkan kendali ke kernel...");
    boot_log_ok("Kernel C++ (kmain) berhasil dijalankan.");
    boot_log_ok("Driver VGA (Text Mode) berhasil dimuat.");

    // 4. INISIALISASI GDT & INTERRUPT (INI YANG PENTING!)
    //    Urutan ini sangat penting: GDT -> IDT -> PIC -> Keyboard
    init_gdt();
    init_idt();
    PIC_remap(0x20, 0x28); // Remap PIC agar tidak konflik
    init_keyboard();

    // 5. AKTIFKAN INTERRUPT SECARA GLOBAL (Sangat Penting!)
    //    "sti" = Set Interrupt Flag. Ini "menghidupkan" hardware.
    asm volatile("sti");

    // 6. Perbarui daftar status
    VGA::terminal.write("\n");
    boot_log_info("Memulai inisialisasi sistem tahap menengah:");
    boot_log_ok("Inisialisasi GDT (Global Descriptor Table)...");
    boot_log_ok("Inisialisasi IDT (Interrupts)...");
    boot_log_ok("PIC (Interrupt Controller) di-remap.");
    boot_log_ok("Memuat Keyboard Driver...");
    boot_log_todo("Implementasi Paging (Manajemen Memori)...");
    boot_log_todo("Memulai Scheduler (Multitasking)...");
    VGA::terminal.write("\n");

    boot_log_ok("Semua proses boot selesai. Menjalankan shell kernel.");
    VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
    VGA::terminal.write("\nTrieternalXOS > "); // Prompt shell sederhana

    // Kernel tidak boleh berhenti
    // Ini sekarang menjadi "idle loop"
    while (1)
    {
        // Hentikan CPU sampai interrupt berikutnya (ketikan) datang
        asm volatile("hlt");
    }
}
