
#include "shell.h"
#include "vga.h"
#include "io.h"
#include "gui.h"
#include "keyboard.h"
#include "pmm.h"
#include "vmm.h"
#include "kheap.h"
#include "scheduler.h"
#include "user_api.h"
#include "ata.h"
#include "fat.h"
#include "vbe.h"
#include "pci.h"
#include "speaker.h"
#include "mouse.h"
#include "elf.h"
#include "fs.h"
#include "ipc.h"

static char shell_buffer[256];
static size_t shell_buffer_index = 0;

static int mystrcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

static int mystrncmp(const char *s1, const char *s2, size_t n)
{
    while (n && *s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
        n--;
    }
    if (n == 0)
    {
        return 0;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

static int my_atoi(const char* s)
{
    int result = 0;
    while (*s >= '0' && *s <= '9')
    {
        result = result * 10 + (*s - '0');
        s++;
    }
    return result;
}

void shell_init()
{
    shell_buffer_index = 0;
    for (int i = 0; i < 256; i++)
    {
        shell_buffer[i] = 0;
    }
}

void shell_prompt()
{
    VGA::terminal.set_color(VGA::COLOR_LIGHT_CYAN, VGA::COLOR_BLACK);
    VGA::terminal.write("TrieternalXOS > ");
    VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
}

static void print_logo()
{
    VGA::terminal.set_color(VGA::COLOR_LIGHT_GREEN, VGA::COLOR_BLACK);
    VGA::terminal.write("================================================================================\n");
    VGA::terminal.write(" _        _      _                  _          \n");
    VGA::terminal.write("| |_ _ __(_) ___| |_ ___ _ __ _ __   __ _| |_   __\n");
    VGA::terminal.write("| __| '__| |/ _ \\ __/ _ \\ '__| '_ \\ / _` | \\ \\ / /\n");
    VGA::terminal.write("| |_| |  | |  __/ ||  __/ |  | | | | (_| | |>  < \n");
    VGA::terminal.write(" \\__|_|  |_|\\___|\\__\\___|_|  |_| |_|\\__,_|_/_/\\_\\\n");
    VGA::terminal.write("                                                 \n");
    VGA::terminal.write("================================================================================\n");
    VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
}

static void write_dec(uint32_t n)
{
    if (n == 0)
    {
        VGA::terminal.putchar('0');
        return;
    }
    char buf[32];
    int i = 0;
    while (n > 0)
    {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    for (int j = i - 1; j >= 0; j--)
    {
        VGA::terminal.putchar(buf[j]);
    }
}

static void write_hex(uint32_t n)
{
    VGA::terminal.write("0x");
    if (n == 0)
    {
        VGA::terminal.putchar('0');
        return;
    }
    char buf[32];
    int i = 0;
    const char* hexchars = "0123456789ABCDEF";
    while (n > 0)
    {
        buf[i++] = hexchars[n % 16];
        n /= 16;
    }
    for (int j = i - 1; j >= 0; j--)
    {
        VGA::terminal.putchar(buf[j]);
    }
}

static void task_a_func()
{
    for (int i = 0; i < 5; i++)
    {
        VGA::terminal.set_color(VGA::COLOR_LIGHT_BLUE, VGA::COLOR_BLACK);
        VGA::terminal.write("[Task A] Iterasi ");
        write_dec(i);
        VGA::terminal.write("\n");
        VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
        scheduler_sleep(1000);
    }
    VGA::terminal.set_color(VGA::COLOR_LIGHT_BLUE, VGA::COLOR_BLACK);
    VGA::terminal.write("[Task A] Selesai.\n");
    VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
}

static void task_b_func()
{
    for (int i = 0; i < 3; i++)
    {
        VGA::terminal.set_color(VGA::COLOR_LIGHT_GREEN, VGA::COLOR_BLACK);
        VGA::terminal.write("[Task B] Iterasi ");
        write_dec(i);
        VGA::terminal.write("\n");
        VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
        scheduler_sleep(1500);
    }
    VGA::terminal.set_color(VGA::COLOR_LIGHT_GREEN, VGA::COLOR_BLACK);
    VGA::terminal.write("[Task B] Selesai.\n");
    VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
}

static void syscall_test_task()
{
    api_print("[Syscall Task] Memulai tugas pengujian Syscall via int 0x80...\n");

    api_print("[Syscall Task] Melakukan alokasi dinamis via SYS_MALLOC...\n");
    char* buffer = (char*)api_malloc(64);
    if (buffer)
    {
        api_print("[Syscall Task] Alokasi berhasil. Menulis data...\n");
        buffer[0] = 'H'; buffer[1] = 'e'; buffer[2] = 'l'; buffer[3] = 'l'; buffer[4] = 'o'; buffer[5] = '\0';
        api_print("[Syscall Task] Isi buffer: ");
        api_print(buffer);
        api_print("\n");

        api_print("[Syscall Task] Membebaskan buffer via SYS_FREE...\n");
        api_free(buffer);
    }

    api_print("[Syscall Task] Menunggu 2 detik via SYS_SLEEP...\n");
    api_sleep(2000);

    api_print("[Syscall Task] Tekan tombol apa saja untuk melanjutkan...\n");
    char key = api_getkey();

    api_print("[Syscall Task] Tombol ditekan: ");
    char key_str[2] = {key, '\0'};
    api_print(key_str);
    api_print("\n");

    api_print("[Syscall Task] Keluar dari tugas via SYS_EXIT...\n");
    api_exit();
}

static void user_mode_test_task()
{

    char msg[32];
    msg[0] = '['; msg[1] = 'U'; msg[2] = 's'; msg[3] = 'e'; msg[4] = 'r'; msg[5] = ' '; msg[6] = 'M'; msg[7] = 'o';
    msg[8] = 'd'; msg[9] = 'e'; msg[10] = ']'; msg[11] = ' '; msg[12] = 'H'; msg[13] = 'e'; msg[14] = 'l'; msg[15] = 'l';
    msg[16] = 'o'; msg[17] = ' '; msg[18] = 'w'; msg[19] = 'o'; msg[20] = 'r'; msg[21] = 'l'; msg[22] = 'd'; msg[23] = '!';
    msg[24] = '\n'; msg[25] = '\0';

    asm volatile("int $0x80" : : "a"(1), "b"(msg));

    char msg2[32];
    msg2[0] = '['; msg2[1] = 'U'; msg2[2] = 's'; msg2[3] = 'e'; msg2[4] = 'r'; msg2[5] = ' '; msg2[6] = 'M'; msg2[7] = 'o';
    msg2[8] = 'd'; msg2[9] = 'e'; msg2[10] = ']'; msg2[11] = ' '; msg2[12] = 'S'; msg2[13] = 'l'; msg2[14] = 'e'; msg2[15] = 'e';
    msg2[16] = 'p'; msg2[17] = 'i'; msg2[18] = 'n'; msg2[19] = 'g'; msg2[20] = ' '; msg2[21] = '2'; msg2[22] = 's'; msg2[23] = '.';
    msg2[24] = '\n'; msg2[25] = '\0';
    asm volatile("int $0x80" : : "a"(1), "b"(msg2));

    asm volatile("int $0x80" : : "a"(5), "b"(2000));

    char msg3[32];
    msg3[0] = '['; msg3[1] = 'U'; msg3[2] = 's'; msg3[3] = 'e'; msg3[4] = 'r'; msg3[5] = ' '; msg3[6] = 'M'; msg3[7] = 'o';
    msg3[8] = 'd'; msg3[9] = 'e'; msg3[10] = ']'; msg3[11] = ' '; msg3[12] = 'E'; msg3[13] = 'x'; msg3[14] = 'i'; msg3[15] = 't';
    msg3[16] = 'i'; msg3[17] = 'n'; msg3[18] = 'g'; msg3[19] = ' '; msg3[20] = 'n'; msg3[21] = 'o'; msg3[22] = 'w'; msg3[23] = '.';
    msg3[24] = '\n'; msg3[25] = '\0';
    asm volatile("int $0x80" : : "a"(1), "b"(msg3));

    asm volatile("int $0x80" : : "a"(6), "b"(0));
}

static void pagefault_test_task()
{
    VGA::terminal.write("Memicu Page Fault dengan melakukan dereferensi pointer NULL...\n");
    scheduler_sleep(1000);

    volatile uint32_t* ptr = (volatile uint32_t*)0;
    *ptr = 0xdeadbeef;
}

static void shell_execute(const char *cmd)
{
    if (mystrcmp(cmd, "") == 0)
    {
        return;
    }
    else if (mystrcmp(cmd, "help") == 0)
    {
        VGA::terminal.write("Daftar Perintah TrieternalX-OS:\n");
        VGA::terminal.write("  help            - Menampilkan menu bantuan ini\n");
        VGA::terminal.write("  gui             - Masuk ke mode GUI Desktop (TUI)\n");
        VGA::terminal.write("  clear           - Membersihkan layar monitor\n");
        VGA::terminal.write("  about / sysinfo - Menampilkan informasi sistem OS\n");
        VGA::terminal.write("  ascii           - Menampilkan logo ASCII Art TrieternalX-OS\n");
        VGA::terminal.write("  color <nama>    - Mengubah warna teks (contoh: color red)\n");
        VGA::terminal.write("  meminfo         - Menampilkan informasi memori PMM & Heap\n");
        VGA::terminal.write("  memtest         - Melakukan pengujian alokasi heap dinamis\n");
        VGA::terminal.write("  tasktest        - Menguji preemptive multitasking dengan 2 task konkuren\n");
        VGA::terminal.write("  syscalltest     - Menguji interupsi software int 0x80 untuk system call\n");
        VGA::terminal.write("  usertest        - Menguji Ring 3 User Mode (Stage 1)\n");
        VGA::terminal.write("  pagefaulttest   - Menguji Page Fault Handler & Kernel Panic (Stage 5)\n");
        VGA::terminal.write("  diskinfo        - Menampilkan informasi hard disk ATA dan partisi FAT\n");
        VGA::terminal.write("  lsdisk          - Menampilkan daftar file di root directory partisi FAT\n");
        VGA::terminal.write("  catdisk <file>  - Membaca file teks dari hard disk partisi FAT\n");
        VGA::terminal.write("  run <elf>       - Memuat dan menjalankan program ELF dari disk\n");
        VGA::terminal.write("  exec <elf>      - Memuat ELF ke user mode dan eksekusi langsung\n");
        VGA::terminal.write("  ps              - Menampilkan daftar semua proses aktif\n");
        VGA::terminal.write("  kill <pid>      - Menghentikan proses berdasarkan PID\n");
        VGA::terminal.write("  ls [path]       - Menampilkan isi direktori VFS\n");
        VGA::terminal.write("  mkdir <path>    - Membuat direktori baru di VFS\n");
        VGA::terminal.write("  touch <path>    - Membuat file baru di VFS\n");
        VGA::terminal.write("  cat <path>      - Membaca isi file dari VFS\n");
        VGA::terminal.write("  write <p> <txt> - Menulis konten ke file VFS\n");
        VGA::terminal.write("  echo <text>     - Menampilkan teks ke layar\n");
        VGA::terminal.write("  guitest         - Menguji VBE 2D graphics engine (mode grafis Ring 0)\n");
        VGA::terminal.write("  ipctest         - Menguji IPC (Inter-Process Communication)\n");
        VGA::terminal.write("  pci             - Enumerasi semua perangkat bus PCI (Stage 12)\n");
        VGA::terminal.write("  beep            - Menguji PC Speaker play_sound() (Stage 16)\n");
        VGA::terminal.write("  mouse           - Menampilkan informasi real-time PS/2 Mouse (Stage 7)\n");
        VGA::terminal.write("  reboot          - Menghidupkan ulang (restart) komputer\n");
        VGA::terminal.write("  shutdown        - Mematikan (shutdown) emulator/komputer\n");
    }
    else if (mystrcmp(cmd, "clear") == 0)
    {
        VGA::terminal.clear();
    }
    else if (mystrcmp(cmd, "gui") == 0)
    {
        gui_start();
    }
    else if (mystrcmp(cmd, "about") == 0 || mystrcmp(cmd, "sysinfo") == 0)
    {
        VGA::terminal.set_color(VGA::COLOR_LIGHT_GREEN, VGA::COLOR_BLACK);
        VGA::terminal.write("--- INFORMASI SISTEM TRIETERNALX-OS ---\n");
        VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
        VGA::terminal.write("Nama OS     : TrieternalX-OS (32-bit)\n");
        VGA::terminal.write("Arsitektur  : x86 (i686 freestanding)\n");
        VGA::terminal.write("Compiler    : GNU GCC (g++ -m32)\n");
        VGA::terminal.write("Bootloader  : GNU GRUB multiboot\n");
        VGA::terminal.write("Fitur       : GDT, IDT, PIC, Keyboard, VGA Text Driver, PMM, VMM, Scheduler\n");
        VGA::terminal.write("Status      : Berjalan lancar di mode terlindungi (Protected Mode)\n");
    }
    else if (mystrcmp(cmd, "ascii") == 0)
    {
        print_logo();
    }
    else if (mystrncmp(cmd, "color ", 6) == 0)
    {
        const char *color_name = cmd + 6;
        if (mystrcmp(color_name, "black") == 0) VGA::terminal.set_color(VGA::COLOR_BLACK, VGA::COLOR_BLACK);
        else if (mystrcmp(color_name, "blue") == 0) VGA::terminal.set_color(VGA::COLOR_BLUE, VGA::COLOR_BLACK);
        else if (mystrcmp(color_name, "green") == 0) VGA::terminal.set_color(VGA::COLOR_GREEN, VGA::COLOR_BLACK);
        else if (mystrcmp(color_name, "cyan") == 0) VGA::terminal.set_color(VGA::COLOR_CYAN, VGA::COLOR_BLACK);
        else if (mystrcmp(color_name, "red") == 0) VGA::terminal.set_color(VGA::COLOR_RED, VGA::COLOR_BLACK);
        else if (mystrcmp(color_name, "magenta") == 0) VGA::terminal.set_color(VGA::COLOR_MAGENTA, VGA::COLOR_BLACK);
        else if (mystrcmp(color_name, "brown") == 0) VGA::terminal.set_color(VGA::COLOR_BROWN, VGA::COLOR_BLACK);
        else if (mystrcmp(color_name, "grey") == 0) VGA::terminal.set_color(VGA::COLOR_LIGHT_GREY, VGA::COLOR_BLACK);
        else if (mystrcmp(color_name, "white") == 0) VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
        else
        {
            VGA::terminal.write("Warna tidak dikenal. Pilihan: black, blue, green, cyan, red, magenta, brown, grey, white\n");
        }
    }
    else if (mystrcmp(cmd, "meminfo") == 0)
    {
        VGA::terminal.write("--- STATUS MEMORY MANAGER ---\n");
        VGA::terminal.write("Total Memory : ");
        write_dec(128);
        VGA::terminal.write(" MB (");
        write_dec(128 * 1024 * 1024 / 4096);
        VGA::terminal.write(" Page Frames)\n");

        size_t used_frames = pmm_get_used_frames();
        size_t free_frames = pmm_get_free_frames();

        VGA::terminal.write("Used Frames  : ");
        write_dec(used_frames);
        VGA::terminal.write(" (");
        write_dec(used_frames * 4);
        VGA::terminal.write(" KB)\n");

        VGA::terminal.write("Free Frames  : ");
        write_dec(free_frames);
        VGA::terminal.write(" (");
        write_dec(free_frames * 4);
        VGA::terminal.write(" KB)\n");

        VGA::terminal.write("Heap Start   : ");
        write_hex(HEAP_START);
        VGA::terminal.write("\n");
    }
    else if (mystrcmp(cmd, "memtest") == 0)
    {
        VGA::terminal.write("Memulai pengujian alokasi heap dinamis...\n");

        VGA::terminal.write("Mencoba mengalokasikan array 100 integer...\n");
        int* arr = new int[100];
        if (arr == nullptr)
        {
            VGA::terminal.set_color(VGA::COLOR_LIGHT_RED, VGA::COLOR_BLACK);
            VGA::terminal.write("Alokasi GAGAL!\n");
            VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
            return;
        }

        VGA::terminal.set_color(VGA::COLOR_LIGHT_GREEN, VGA::COLOR_BLACK);
        VGA::terminal.write("Alokasi BERHASIL. Alamat pointer: ");
        write_hex((uint32_t)arr);
        VGA::terminal.write("\n");
        VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);

        VGA::terminal.write("Mengisi data ke array...\n");
        for (int i = 0; i < 100; i++)
        {
            arr[i] = i * 7;
        }

        VGA::terminal.write("Memverifikasi data...\n");
        bool test_passed = true;
        for (int i = 0; i < 100; i++)
        {
            if (arr[i] != i * 7)
            {
                test_passed = false;
                break;
            }
        }

        if (test_passed)
        {
            VGA::terminal.set_color(VGA::COLOR_LIGHT_GREEN, VGA::COLOR_BLACK);
            VGA::terminal.write("Verifikasi data BERHASIL!\n");
        }
        else
        {
            VGA::terminal.set_color(VGA::COLOR_LIGHT_RED, VGA::COLOR_BLACK);
            VGA::terminal.write("Verifikasi data GAGAL!\n");
        }
        VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);

        VGA::terminal.write("Membebaskan memori array...\n");
        delete[] arr;
        VGA::terminal.set_color(VGA::COLOR_LIGHT_GREEN, VGA::COLOR_BLACK);
        VGA::terminal.write("Memori dibebaskan. Pengujian selesai.\n");
        VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
    }
    else if (mystrcmp(cmd, "tasktest") == 0)
    {
        VGA::terminal.write("Membuat Task A (1.0s) dan Task B (1.5s) secara konkuren...\n");
        scheduler_create_task(task_a_func, "Task_A");
        scheduler_create_task(task_b_func, "Task_B");
    }
    else if (mystrcmp(cmd, "syscalltest") == 0)
    {
        VGA::terminal.write("Membuat tugas uji syscall yang menggunakan interupsi int 0x80...\n");
        scheduler_create_task(syscall_test_task, "Syscall_Test");
    }
    else if (mystrcmp(cmd, "usertest") == 0)
    {
        VGA::terminal.write("Membuat tugas Ring 3 User Mode sesungguhnya...\n");
        scheduler_create_task(user_mode_test_task, "User_Mode_Test", true);
    }
    else if (mystrcmp(cmd, "pagefaulttest") == 0)
    {
        VGA::terminal.write("Membuat tugas untuk menguji deteksi Page Fault...\n");
        scheduler_create_task(pagefault_test_task, "PageFault_Test");
    }
    else if (mystrcmp(cmd, "diskinfo") == 0)
    {
        VGA::terminal.write("--- ATA DISK & FILESYSTEM INFO ---\n");
        if (ata_disk_exists())
        {
            VGA::terminal.write("Status Disk  : TERDETEKSI\n");
            VGA::terminal.write("Model Disk   : ");
            VGA::terminal.write(ata_get_model());
            VGA::terminal.write("\n");
            VGA::terminal.write("Status FAT   : ");
            if (fat_is_active())
            {
                VGA::terminal.write("AKTIF (FAT16/FAT32)\n");
            }
            else
            {
                VGA::terminal.write("TIDAK AKTIF / BELUM DI-FORMAT\n");
            }
        }
        else
        {
            VGA::terminal.write("Status Disk  : TIDAK TERDETEKSI (QEMU dijalankan tanpa hard disk)\n");
        }
    }
    else if (mystrcmp(cmd, "lsdisk") == 0)
    {
        if (fat_is_active())
        {
            fat_list_root();
        }
        else
        {
            VGA::terminal.write("FAT Filesystem tidak aktif / disk tidak terdeteksi.\n");
        }
    }
    else if (mystrncmp(cmd, "catdisk ", 8) == 0)
    {
        const char* filename = cmd + 8;
        if (fat_is_active())
        {
            uint32_t size = fat_get_file_size(filename);
            if (size == 0)
            {
                VGA::terminal.write("Berkas tidak ditemukan atau kosong.\n");
            }
            else
            {
                uint8_t* buf = new uint8_t[size + 1];
                if (buf)
                {
                    if (fat_read_file(filename, buf, size))
                    {
                        buf[size] = '\0';
                        VGA::terminal.write((char*)buf);
                        VGA::terminal.write("\n");
                    }
                    else
                    {
                        VGA::terminal.write("Gagal membaca berkas.\n");
                    }
                    delete[] buf;
                }
            }
        }
        else
        {
            VGA::terminal.write("FAT Filesystem tidak aktif / disk tidak terdeteksi.\n");
        }
    }
    else if (mystrcmp(cmd, "guitest") == 0)
    {
        if (!vbe_is_active())
        {
            VGA::terminal.write("VBE mode grafis tidak aktif.\n");
            return;
        }

        vbe_clear(0x0f172a);

        vbe_draw_rect(50, 50, 700, 500, 0x1e293b);
        vbe_draw_rect(50, 50, 700, 30, 0x3b82f6);
        vbe_draw_string(60, 58, "TrieternalX-OS VBE 2D Graphics Engine Demo", 0xFFFFFF);

        vbe_draw_rect(100, 120, 150, 100, 0xef4444);
        vbe_draw_rect(280, 120, 150, 100, 0x10b981);
        vbe_draw_rect(460, 120, 150, 100, 0xf59e0b);
        vbe_draw_rect(640, 120, 80, 100, 0x8b5cf6);

        vbe_draw_line(100, 270, 700, 270, 0x94a3b8);
        vbe_draw_line(100, 290, 700, 350, 0xec4899);
        vbe_draw_line(100, 350, 700, 290, 0x06b6d4);

        vbe_draw_string(100, 400, "Resolusi Terdeteksi: 800 x 600 - 32 Bit Per Pixel (ARGB)", 0x10b981);
        vbe_draw_string(100, 430, "Pustaka grafis freestanding ini di-render secara modular di Ring 0.", 0xFFFFFF);
        vbe_draw_string(100, 460, "Tekan tombol apa saja di emulator untuk kembali ke mode teks...", 0xeab308);

        keyboard_get_key();

        vbe_console_clear();
    }
    else if (mystrcmp(cmd, "pci") == 0)
    {
        pci_print_devices();
    }
    else if (mystrcmp(cmd, "beep") == 0)
    {
        VGA::terminal.write("Membunyikan speaker PC (440Hz)... \n");
        beep(440, 500);
        VGA::terminal.write("Beep selesai.\n");
    }
    else if (mystrcmp(cmd, "mouse") == 0)
    {
        VGA::terminal.write("Informasi Mouse PS/2 (Tekan tombol keyboard apa saja untuk keluar):\n");
        while (!keyboard_has_key())
        {
            VGA::terminal.write("X: ");
            write_dec(mouse_get_x());
            VGA::terminal.write(" Y: ");
            write_dec(mouse_get_y());
            VGA::terminal.write(" L: ");
            VGA::terminal.write(mouse_is_left_clicked() ? "1" : "0");
            VGA::terminal.write(" R: ");
            VGA::terminal.write(mouse_is_right_clicked() ? "1" : "0");
            VGA::terminal.write("   \r");
            scheduler_sleep(50);
        }
        keyboard_get_key();
        VGA::terminal.write("\n");
    }
    else if (mystrcmp(cmd, "reboot") == 0)
    {
        VGA::terminal.write("Sedang melakukan reboot...");
        outb(0x64, 0xFE);
    }
    else if (mystrcmp(cmd, "shutdown") == 0)
    {
        VGA::terminal.write("Sedang mematikan sistem...");
        outw(0x604, 0x2000);
        outw(0xB004, 0x2000);
        outw(0x4004, 0x3400);
    }
    else if (mystrcmp(cmd, "ps") == 0)
    {
        VGA::terminal.write("--- DAFTAR PROSES ---\n");
        process_list();
    }
    else if (mystrncmp(cmd, "kill ", 5) == 0)
    {
        int pid = my_atoi(cmd + 5);
        if (pid <= 0)
        {
            VGA::terminal.write("PID tidak valid.\n");
        }
        else
        {
            process_kill((int32_t)pid);
            VGA::terminal.write("Proses dengan PID ");
            write_dec(pid);
            VGA::terminal.write(" telah dihentikan.\n");
        }
    }
    else if (mystrncmp(cmd, "run ", 4) == 0)
    {
        const char* filename = cmd + 4;
        if (!fat_is_active())
        {
            VGA::terminal.write("FAT Filesystem tidak aktif.\n");
        }
        else
        {
            VGA::terminal.write("Memuat ELF dari disk: ");
            VGA::terminal.write(filename);
            VGA::terminal.write("...\n");

            ELFLoadResult result = elf_load_from_disk(filename);
            if (result.success)
            {
                VGA::terminal.set_color(VGA::COLOR_LIGHT_GREEN, VGA::COLOR_BLACK);
                VGA::terminal.write("ELF berhasil dimuat. Entry: 0x");
                write_hex(result.entry_point);
                VGA::terminal.write("\n");
                VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);

                scheduler_create_task((void(*)())result.entry_point, filename, true);
                VGA::terminal.write("Proses dibuat. Gunakan 'ps' untuk melihat status.\n");
            }
            else
            {
                VGA::terminal.set_color(VGA::COLOR_LIGHT_RED, VGA::COLOR_BLACK);
                VGA::terminal.write("Gagal memuat ELF. Pastikan file valid dan berformat ELF32.\n");
                VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
            }
        }
    }
    else if (mystrncmp(cmd, "exec ", 5) == 0)
    {
        const char* filename = cmd + 5;
        if (!fat_is_active())
        {
            VGA::terminal.write("FAT Filesystem tidak aktif.\n");
        }
        else
        {
            ELFLoadResult result = elf_load_from_disk(filename);
            if (result.success)
            {
                VGA::terminal.write("ELF dimuat. Entry: 0x");
                write_hex(result.entry_point);
                VGA::terminal.write("\n");

                Task* t = scheduler_create_task((void(*)())result.entry_point, filename, true);
                if (t)
                {
                    VGA::terminal.write("Proses PID ");
                    write_dec(t->id);
                    VGA::terminal.write(" berjalan.\n");
                }
            }
            else
            {
                VGA::terminal.set_color(VGA::COLOR_LIGHT_RED, VGA::COLOR_BLACK);
                VGA::terminal.write("Gagal memuat ELF.\n");
                VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
            }
        }
    }
    else if (mystrcmp(cmd, "ls") == 0)
    {
        vfs_list("/");
    }
    else if (mystrncmp(cmd, "ls ", 3) == 0)
    {
        vfs_list(cmd + 3);
    }
    else if (mystrncmp(cmd, "mkdir ", 6) == 0)
    {
        const char* path = cmd + 6;
        if (vfs_mkdir(path) >= 0)
        {
            VGA::terminal.set_color(VGA::COLOR_LIGHT_GREEN, VGA::COLOR_BLACK);
            VGA::terminal.write("Direktori dibuat: ");
            VGA::terminal.write(path);
            VGA::terminal.write("\n");
            VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
        }
        else
        {
            VGA::terminal.set_color(VGA::COLOR_LIGHT_RED, VGA::COLOR_BLACK);
            VGA::terminal.write("Gagal membuat direktori.\n");
            VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
        }
    }
    else if (mystrncmp(cmd, "touch ", 6) == 0)
    {
        const char* path = cmd + 6;
        if (vfs_create(path) >= 0)
        {
            VGA::terminal.set_color(VGA::COLOR_LIGHT_GREEN, VGA::COLOR_BLACK);
            VGA::terminal.write("File dibuat: ");
            VGA::terminal.write(path);
            VGA::terminal.write("\n");
            VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
        }
        else
        {
            VGA::terminal.set_color(VGA::COLOR_LIGHT_RED, VGA::COLOR_BLACK);
            VGA::terminal.write("Gagal membuat file.\n");
            VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
        }
    }
    else if (mystrncmp(cmd, "cat ", 4) == 0)
    {
        const char* path = cmd + 4;
        int32_t fd = vfs_open(path, 0);
        if (fd >= 0)
        {
            char buf[1024];
            int32_t bytes = vfs_read(fd, buf, 1023);
            if (bytes > 0)
            {
                buf[bytes] = '\0';
                VGA::terminal.write(buf);
                VGA::terminal.putchar('\n');
            }
            else
            {
                VGA::terminal.write("(file kosong)\n");
            }
            vfs_close(fd);
        }
        else if (fd == -2)
        {
            VGA::terminal.write("It's a directory.\n");
        }
        else
        {
            VGA::terminal.write("File not found: ");
            VGA::terminal.write(path);
            VGA::terminal.write("\n");
        }
    }
    else if (mystrncmp(cmd, "echo ", 5) == 0)
    {
        const char* text = cmd + 5;
        VGA::terminal.write(text);
        VGA::terminal.putchar('\n');
    }
    else if (mystrncmp(cmd, "write ", 6) == 0)
    {

        const char* rest = cmd + 6;
        char path_buf[128];
        int i = 0;
        while (rest[i] && rest[i] != ' ' && i < 127)
        {
            path_buf[i] = rest[i];
            i++;
        }
        path_buf[i] = '\0';

        if (rest[i] == ' ')
        {
            const char* content = rest + i + 1;
            int32_t fd = vfs_open(path_buf, 2);
            if (fd >= 0)
            {
                uint32_t clen = 0;
                { const char* s = content; while (*s++) clen++; }
                vfs_write(fd, content, clen);
                VGA::terminal.set_color(VGA::COLOR_LIGHT_GREEN, VGA::COLOR_BLACK);
                VGA::terminal.write("Tertulis ke ");
                VGA::terminal.write(path_buf);
                VGA::terminal.write(" (");
                write_dec(clen);
                VGA::terminal.write(" bytes)\n");
                VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);
                vfs_close(fd);
            }
            else
            {
                VGA::terminal.write("File not found: ");
                VGA::terminal.write(path_buf);
                VGA::terminal.write("\n");
            }
        }
        else
        {
            VGA::terminal.write("Usage: write <path> <content>\n");
        }
    }
    else if (mystrcmp(cmd, "ipctest") == 0)
    {
        ipc_test();
    }
    else
    {
        VGA::terminal.write("Perintah '");
        VGA::terminal.write(cmd);
        VGA::terminal.write("' tidak dikenal. Ketik 'help' untuk daftar perintah.\n");
    }
}

void shell_input(char c)
{
    if (c == '\n')
    {
        VGA::terminal.putchar('\n');
        shell_buffer[shell_buffer_index] = '\0';
        shell_execute(shell_buffer);
        shell_buffer_index = 0;
        shell_prompt();
    }
    else if (c == '\b')
    {
        if (shell_buffer_index > 0)
        {
            shell_buffer_index--;
            shell_buffer[shell_buffer_index] = '\0';
            VGA::terminal.putchar('\b');
        }
    }
    else
    {

        if (shell_buffer_index < sizeof(shell_buffer) - 1)
        {
            shell_buffer[shell_buffer_index++] = c;
            VGA::terminal.putchar(c);
        }
    }
}
