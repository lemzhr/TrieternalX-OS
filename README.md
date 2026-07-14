# TrieternalX-OS

> Sistem operasi freestanding 32-bit (x86 i686) yang ditulis dari nol menggunakan C++17 dan Assembly x86, berjalan di Protected Mode dengan GNU GRUB Multiboot.

TrieternalX-OS adalah proyek sistem operasi yang dikembangkan untuk Tugas Akhir/Skripsi, mengimplementasikan berbagai komponen OS modern dari bootloader hingga GUI Desktop interaktif dengan window manager, termasuk dukungan VBE graphics 800x600, preemptive multitasking, virtual memory, filesystem FAT16/FAT32, driver hardware, dan web-based GUI simulator.

---

## Fitur Utama

### Kernel Core
- **Multiboot Compliant** - Bootloader GNU GRUB dengan multiboot header
- **Protected Mode 32-bit** - Berjalan di mode terlindungi x86
- **C++17 Freestanding** - Kernel ditulis dalam C++ tanpa dependency libc/stdlib
- **TSS (Task State Segment)** - Transisi Ring 0 ke Ring 3 via `iret`

### Memory Management
- **PMM (Physical Memory Manager)** - Bitmap-based page frame allocator (128 MB RAM, 4KB pages)
- **VMM (Virtual Memory Manager)** - Identity mapping paging 32-bit x86, page directory & page table
- **Heap Allocator** - Best-fit dynamic allocation (`kmalloc` / `kfree`)
- **Memory Protection** - User Space vs Kernel Space, page permission (Supervisor/User)

### Process Management
- **Preemptive Multitasking** - Round-robin scheduler dengan context switching via timer interrupt
- **Task Creation** - Dynamic task allocation dengan kernel stack dan user stack terpisah
- **Ring 3 User Mode** - Transisi sebenarnya ke user mode via `iret`, proteksi akses memori kernel
- **Sleep & Yield** - Task sleep berbasis tick timer (100 Hz)
- **Task Termination** - Cleanup otomatis stack memory saat task selesai

### System Calls
- **Software Interrupt `int 0x80`** - Interface syscall user-to-kernel
- **Syscall Table** - `SYS_WRITE`, `SYS_READ_KEY`, `SYS_MALLOC`, `SYS_FREE`, `SYS_SLEEP`, `SYS_EXIT`
- **User API Library** - Wrapper functions (`api_print`, `api_malloc`, `api_free`, `api_sleep`, `api_getkey`, `api_exit`)

### Interrupt & Exception Handling
- **IDT (Interrupt Descriptor Table)** - 256 interrupt gates
- **ISR Stubs (Assembly)** - Pusha + interrupt frame untuk setiap exception
- **PIC Remapping** - 8259 PIC di-remap untuk menghindari konflik CPU exceptions
- **Page Fault Handler (INT 14)** - Decode error code, register dump, null pointer detection, kernel panic informatif

### Drivers
| Driver | Keterangan |
|--------|-----------|
| **VGA Text** | Terminal 80x25 berwarna, 16 warna |
| **VBE Graphics** | Linear framebuffer 800x600 32bpp, drawing primitives (rect, line, pixel, string) |
| **Keyboard PS/2** | Scancode map US QWERTY, interrupt-driven |
| **Mouse PS/2** | 3-byte packet decoding, cursor rendering, left/right click |
| **PIT Timer** | Programmable Interval Timer 100 Hz, tick counter |
| **ATA/IDE PIO** | Hard disk read/write sectors, disk identification |
| **PCI Bus** | Bus enumeration, vendor/device ID, class code, BAR |
| **PC Speaker** | Sound generation via PIT Channel 2, `play_sound()` / `beep()` |

### File System
- **Virtual File System (VFS)** - RAM-based filesystem dengan direktori dan file bawaan
- **FAT16/FAT32 Reader** - Parsing boot sector BPB, cluster chain traversal, file reading
- **File Operations** - `ls`, `cat`, directory listing, file size query

### GUI Desktop Environment
- **Window Manager** - Multi-window dengan z-order, focus management, close button
- **Desktop Environment** - Wallpaper, taskbar, start menu, desktop icons, system tray
- **Applications:**
  - File Explorer - navigasi direktori VFS dengan visual folder/file
  - System Monitor (htop) - CPU/RAM gauge, service table dengan toggle start/stop
  - Terminal Emulator - bash-like shell emulator (neofetch, help, ls, uname)
  - Notepad - text viewer untuk file VFS
  - Kernel Code Viewer - syntax-highlighted source code viewer
  - Settings Panel - theme selector, font configuration, taskbar size
- **Dual Render Mode** - VGA Text Mode fallback + VBE Graphics Mode
- **Mouse Integration** - Cursor rendering di framebuffer VBE

### Kernel Logger
- **Log Levels** - INFO, WARNING, ERROR, DEBUG
- **Timestamp** - Berdasarkan PIT tick counter
- **Color-coded Output** - Setiap level memiliki warna konsol berbeda

### Web GUI Simulator
- **Boot Simulator** - Animasi boot sequence dengan progress bar
- **Desktop Environment** - Glassmorphism UI dengan window drag, minimize, maximize, close
- **Live Clock** - System tray clock real-time
- **Start Menu** - Pinned apps, power actions (reboot/shutdown)
- **File Explorer** - Navigasi virtual filesystem interaktif
- **System Monitor** - CPU/RAM gauge dinamis, service management
- **Terminal Emulator** - CLI interaktif dengan simulasi perintah kernel
- **Settings Panel** - Theme wallpaper, taskbar size, font configuration
- **Theme Support** - 5 tema wallpaper (Win95 Teal, Indigo, Sunset, Forest, Midnight)

---

## Struktur Proyek

```
TrieternalX-OS/
├── Makefile                    # Build system utama
├── ARCHITECTURE.md             # Dokumentasi arsitektur OS
├── README.md                   # Dokumentasi proyek
├── prompt.txt                  # Rencana pengembangan 20 tahap
├── image.png                   # Screenshot OS
│
├── bin/
│   ├── kernel.bin              # Kernel binary (hasil linking)
│   └── TrieternalX-OS.iso      # Bootable ISO image
│
├── build/                      # Object files hasil kompilasi
│   ├── src/kernel/
│   ├── src/drivers/
│   └── src/arch/x86/
│
├── scripts/
│   └── linker.ld               # Linker script (entry: _start, base: 1MB)
│
├── sysroot/
│   └── boot/
│       ├── kernel.bin           # Kernel binary untuk GRUB
│       └── grub/
│           └── grub.cfg         # GRUB configuration
│
├── src/
│   ├── arch/x86/
│   │   ├── boot.asm            # Multiboot header + entry point _start
│   │   ├── gdt.cpp             # Global Descriptor Table (Ring 0 & Ring 3)
│   │   ├── gdt_asm.asm         # GDT flush (lgdt + far jump)
│   │   ├── idt.cpp             # Interrupt Descriptor Table
│   │   └── isrs.asm            # ISR stubs + timer/keyboard/mouse/syscall handlers
│   │
│   ├── kernel/
│   │   ├── kernel.cpp          # Entry point kmain(), inisialisasi semua subsistem
│   │   ├── shell.cpp           # Kernel shell interaktif dengan 20+ perintah
│   │   ├── gui.cpp             # Window Manager + Desktop Environment (TUI & VBE)
│   │   ├── scheduler.cpp       # Preemptive round-robin task scheduler
│   │   ├── syscall.cpp         # System call dispatcher (int 0x80)
│   │   ├── user_api.cpp        # User-space API wrapper library
│   │   ├── pmm.cpp             # Physical Memory Manager (bitmap allocator)
│   │   ├── vmm.cpp             # Virtual Memory Manager (paging x86) + Page Fault Handler
│   │   ├── kheap.cpp           # Kernel heap allocator (best-fit)
│   │   ├── fs.cpp              # Virtual File System (RAM-based)
│   │   ├── fat.cpp             # FAT16/FAT32 filesystem parser & reader
│   │   ├── services.cpp        # System services manager
│   │   ├── api.cpp             # API functions untuk GUI applications
│   │   └── logger.cpp          # Kernel logging system (INFO/WARN/ERROR/DEBUG)
│   │
│   ├── drivers/
│   │   ├── vga.cpp             # VGA Text Mode driver (80x25, 16 colors)
│   │   ├── vbe.cpp             # VBE Linear Framebuffer driver (800x600, 32bpp)
│   │   ├── keyboard.cpp        # PS/2 Keyboard driver (scancode map)
│   │   ├── mouse.cpp           # PS/2 Mouse driver (3-byte packet, cursor)
│   │   ├── timer.cpp           # PIT timer driver (100 Hz)
│   │   ├── ata.cpp             # ATA/IDE PIO disk driver
│   │   ├── pic.cpp             # 8259 PIC remapping
│   │   ├── pci.cpp             # PCI bus enumeration
│   │   └── speaker.cpp         # PC Speaker sound driver
│   │
│   └── include/                # Header files (.h)
│       ├── types.h             # Tipe data integer (uint8_t - uint64_t, size_t)
│       ├── io.h                # Port I/O (inb, outb, inw, outw, inl, outl)
│       ├── gdt.h               # GDT structures & functions
│       ├── interrupts.h        # IDT & ISR declarations
│       ├── pic.h               # PIC constants & functions
│       ├── keyboard.h          # Keyboard driver interface
│       ├── mouse.h             # Mouse driver interface
│       ├── timer.h             # Timer driver interface
│       ├── vga.h               # VGA terminal class
│       ├── vbe.h               # VBE graphics interface
│       ├── shell.h             # Shell functions
│       ├── gui.h               # GUI functions
│       ├── scheduler.h         # Task struct & scheduler functions
│       ├── syscall.h           # Syscall handler declarations
│       ├── user_api.h          # User API functions
│       ├── pmm.h               # PMM interface
│       ├── vmm.h               # VMM interface
│       ├── kheap.h             # Heap allocator interface
│       ├── fs.h                # VFS interface
│       ├── fat.h               # FAT filesystem interface
│       ├── ata.h               # ATA driver interface
│       ├── pci.h               # PCI device structures
│       ├── services.h          # System services interface
│       ├── api.h               # API functions for GUI
│       ├── logger.h            # Logger interface
│       ├── multiboot.h         # Multiboot info structure
│       └── speaker.h           # Speaker driver interface
│
└── web-gui/
    ├── index.html              # Web GUI dashboard HTML
    ├── app.js                  # Web GUI application logic
    └── style.css               # Web GUI styling (glassmorphism)
```

---

## Prasyarat

### Untuk Build Kernel
- **OS**: Linux (tested on Fedora)
- **GCC/G++** dengan flag `-m32` (cross-compiler 32-bit)
- **NASM** - Netwide Assembler untuk file `.asm`
- **GNU ld** - Linker dengan target `elf_i386`
- **GRUB2** - `grub2-mkrescue` untuk membuat bootable ISO
- **QEMU** - `qemu-system-i386` untuk testing (opsional)

### Instalasi Dependency (Fedora)

```bash
# Install build tools
sudo dnf install gcc g++ nasm grub2-tools xorriso qemu-system-x86

# Pastikan gcc mendukung mode 32-bit
# Sudah tersedia native di Fedora (tanpa gcc-i686-linux-gnu)
```

### Untuk Web GUI
- Browser modern (Chrome, Firefox, Edge)
- Tidak perlu install - buka langsung `web-gui/index.html`

---

## Build & Jalankan

### Build Kernel

```bash
# Build semua (compilation + linking + ISO creation)
make

# Build hanya kernel binary
make $(BIN_DIR)/kernel.bin
```

### Jalankan di QEMU

```bash
# Build dan jalankan langsung di QEMU
make run
```

Perintah `make run` akan menjalankan:
```bash
qemu-system-i386 -cdrom bin/TrieternalX-OS.iso -m 512M
```

### Bersihkan Build

```bash
make clean
```

---

## Perintah Shell

Setelah OS boot, shell prompt `TrieternalXOS >` akan muncul. Berikut daftar perintah yang tersedia:

| Perintah | Keterangan |
|----------|-----------|
| `help` | Menampilkan daftar semua perintah |
| `gui` | Masuk ke mode GUI Desktop (TUI/VBE) |
| `clear` | Membersihkan layar |
| `about` / `sysinfo` | Informasi sistem OS |
| `ascii` | Menampilkan logo ASCII art |
| `color <nama>` | Mengubah warna teks (red, blue, green, cyan, dll) |
| `meminfo` | Status memory manager (PMM & Heap) |
| `memtest` | Pengujian alokasi heap dinamis |
| `tasktest` | Uji preemptive multitasking (2 task konkuren) |
| `syscalltest` | Uji system call via `int 0x80` |
| `usertest` | Uji Ring 3 User Mode |
| `pagefaulttest` | Uji Page Fault Handler & Kernel Panic |
| `diskinfo` | Info hard disk ATA dan partisi FAT |
| `lsdisk` | Daftar file di root directory FAT |
| `catdisk <file>` | Baca file teks dari hard disk FAT |
| `guitest` | Uji VBE 2D graphics engine |
| `pci` | Enumerasi semua perangkat bus PCI |
| `beep` | Uji PC Speaker (440Hz) |
| `mouse` | Tampilkan info real-time PS/2 Mouse |
| `reboot` | Restart komputer |
| `shutdown` | Matikan sistem |

---

## Navigasi GUI Desktop

Saat mode GUI aktif (`gui` command), gunakan keyboard untuk navigasi:

| Tombol | Aksi |
|--------|------|
| `F1` | Buka jendela Bantuan & Panduan |
| `F2` | Buka File Explorer |
| `F3` | Buka System Monitor (htop) |
| `F4` | Buka/Tutup Terminal Emulator |
| `F5` | Buka/Tutup Start Menu |
| `Space + C` | Tutup jendela fokus |
| `Esc` | Keluar GUI ke CLI Shell |

Di mode File Explorer:
- `Panah Atas/Bawah` - Navigasi file
- `Enter` - Buka file/folder
- `Backspace` - Kembali ke parent folder

---

## Arsitektur Sistem

```
+-----------------------------------+
|  Bootloader [GNU GRUB Multiboot]  |
+-----------------------------------+
                |
                v
+-----------------------------------+
|  HAL (GDT, IDT, PIC, TSS)        |
+-----------------------------------+
                |
                v
+-----------------------------------+
|  Kernel Core (kmain / C++17)      |
+-----------------------------------+
                |
    +-----------+-----------+
    |                       |
    v                       v
+------------+      +------------+
| PMM, VMM,  |      | ISR, Timer,|
| Heap       |      | Keyboard   |
+------------+      +------------+
    |                       |
    v                       v
+------------+      +------------+
| Scheduler  |      | VGA, VBE,  |
| Round-Robin|      | ATA, PCI   |
+------------+      +------------+
    |                       |
    v                       v
+------------+      +------------+
| VFS, FAT   |      | GUI Engine |
|            |      | (Window Mgr)|
+------------+      +------------+
    |                       |
    v                       v
+------------+      +------------+
| User API   |      | Shell,     |
| (int 0x80) |      | Desktop App|
+------------+      +------------+
```

---

## Teknologi

- **Bahasa**: C++17 (freestanding), x86 Assembly (NASM)
- **Compiler**: GNU GCC/G++ (`-m32 -ffreestanding -fno-rtti -fno-exceptions`)
- **Linker**: GNU ld (`-m elf_i386 -nostdlib`)
- **Bootloader**: GNU GRUB (Multiboot compliant)
- **Target**: x86 i686 (32-bit Protected Mode)
- **Graphics**: VGA Text Mode (80x25) + VBE Linear Framebuffer (800x600, 32bpp)
- **Emulator**: QEMU (`qemu-system-i386`)
- **Web GUI**: HTML5, CSS3 (Glassmorphism), Vanilla JavaScript

---

## Rencana Pengembangan (20 Tahap)

| Tahap | Status | Deskripsi |
|-------|--------|-----------|
| 1. Ring 3 User Mode | Selesai | GDT Ring 3, TSS, `switch_to_user_mode()`, `iret`, user/kernel stack |
| 2. ELF Loader | Dalam Rencana | ELF32 parsing, program header, virtual memory mapping |
| 3. Process Manager | Dalam Rencana | PID, PCB, parent/child, exit code, wait, kill, exec |
| 4. Memory Protection | Dalam Rencana | User/Kernel space, page permissions, per-process page directory |
| 5. Page Fault Manager | Selesai | INT 14, error code decode, register dump, null pointer detection |
| 6. Kernel Logger | Selesai | INFO/WARNING/ERROR/DEBUG, timestamp PIT |
| 7. Mouse Driver | Selesai | PS/2 IRQ12, cursor, left/right click, scroll |
| 8. Window Manager | Selesai | Window, drag, resize, close, minimize, maximize, focus, z-order |
| 9. Desktop Environment | Selesai | Wallpaper, taskbar, start menu, desktop icons, clock |
| 10. VFS Lengkap | Dalam Rencana | Mount, unmount, file descriptor, mkdir, rename, delete, copy |
| 11. FAT32 Write | Selesai (Read) | FAT16/FAT32 read support, cluster chain traversal |
| 12. PCI Bus | Selesai | Bus enumeration, vendor/device ID, class code, BAR |
| 13. USB | Dalam Rencana | UHCI/OHCI/EHCI foundation |
| 14. Network | Dalam Rencana | RTL8139, Ethernet, ARP, IPv4, ICMP, UDP, TCP |
| 15. Socket API | Dalam Rencana | BSD Socket API (socket, bind, listen, accept, connect) |
| 16. Audio | Selesai (Basic) | PC Speaker `play_sound()` / `beep()` |
| 17. Security | Dalam Rencana | User, group, permission, login, password |
| 18. Dynamic Module | Dalam Rencana | Load/unload module, driver registration, runtime linking |
| 19. SMP | Dalam Rencana | AP Startup, Local APIC, IO APIC, spinlock |
| 20. Optimisasi & Rilis | Dalam Rencana | Benchmark, boot optimization, documentation, release |

---

## Screenshot

![TrieternalX-OS Desktop](image.png)

---

## Lisensi

Hak Cipta &copy; 2026 Ariel Aprielyullah. All rights reserved.

Proyek ini dikembangkan sebagai bagian dari Tugas Akhir/Skripsi.
