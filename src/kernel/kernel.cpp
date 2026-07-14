
#include "vga.h"
#include "gdt.h"
#include "interrupts.h"
#include "keyboard.h"
#include "pic.h"
#include "shell.h"
#include "fs.h"
#include "services.h"
#include "pmm.h"
#include "vmm.h"
#include "kheap.h"
#include "timer.h"
#include "scheduler.h"
#include "syscall.h"
#include "ata.h"
#include "fat.h"
#include "vbe.h"
#include "logger.h"
#include "mouse.h"
#include "elf.h"

void boot_log_ok(const char *message)
{
    klog_info("BOOT", message);
}

void boot_log_info(const char *message)
{
    klog_info("BOOT", message);
}

void boot_log_todo(const char *message)
{
    klog_warn("BOOT", message);
}

extern "C" void kmain(uint32_t magic, uint32_t multiboot_addr)
{

    if (magic == 0x2BADB002 && multiboot_addr != 0)
    {
        vbe_init((multiboot_info*)multiboot_addr);
    }

    VGA::terminal.initialize();

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

    boot_log_info("Bootloader menyerahkan kendali ke kernel...");
    boot_log_ok("Kernel C++ (kmain) berhasil dijalankan.");
    boot_log_ok("Driver VGA (Text Mode) berhasil dimuat.");

    init_gdt();
    init_idt();
    PIC_remap(0x20, 0x28);
    init_keyboard();
    mouse_init();

    pmm_init(128 * 1024 * 1024);
    vmm_init();
    kheap_init();

    timer_init(100);
    scheduler_init();
    syscall_init();

    ata_init();
    fat_init();

    asm volatile("sti");

    scheduler_start();

    fs_init();
    services_init();

    VGA::terminal.write("\n");
    boot_log_info("Memulai inisialisasi sistem tahap menengah:");
    boot_log_ok("Inisialisasi GDT (Global Descriptor Table)...");
    boot_log_ok("Inisialisasi IDT (Interrupts)...");
    boot_log_ok("PIC (Interrupt Controller) di-remap.");
    boot_log_ok("Memuat Keyboard Driver...");
    boot_log_ok("Inisialisasi Memory Manager (PMM, VMM, Heap)...");
    boot_log_ok("Inisialisasi PIT Timer & Scheduler...");
    boot_log_ok("Inisialisasi System Call Manager (int 0x80)...");
    boot_log_ok("Inisialisasi VBE (VESA BIOS Extensions) Graphics...");
    boot_log_ok("Inisialisasi ATA/IDE Disk Driver...");
    boot_log_ok("Memuat FAT Filesystem Reader...");
    boot_log_ok("Memuat Virtual Filesystem (RAM VFS)...");
    boot_log_ok("Memulai System Services Manager...");
    boot_log_ok("Memuat ELF Loader (Stage 1: ELF Parser)...");
    boot_log_ok("Memuat Process Manager (Stage 2: Process Control)...");
    VGA::terminal.write("\n");

    boot_log_ok("Semua proses boot selesai. Menjalankan shell kernel.");
    VGA::terminal.write("\n");
    shell_init();
    shell_prompt();

    while (1)
    {

        asm volatile("hlt");
    }
}
