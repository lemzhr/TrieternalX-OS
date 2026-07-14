# File: Makefile (di root proyek)

# === Variabel ===
AS = nasm
# Menggunakan compiler native dengan flag 32-bit karena Fedora tidak menyediakan gcc-i686-linux-gnu secara default
CC = g++
LD = ld

# Flags
# -I$(SRC_DIR)/include memberitahu compiler di mana mencari file .h
CFLAGS = -m32 -Wall -Wextra -std=c++17 -ffreestanding -fno-rtti -fno-exceptions -g -I$(SRC_DIR)/include
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T scripts/linker.ld -nostdlib -g

# === File & Direktori ===
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
SYSROOT_DIR = sysroot

# === EKSPLISIT: Daftar semua file .o ===
# Ini adalah cara paling aman untuk memastikan semuanya di-link

# File Assembly
ASM_OBJS = $(BUILD_DIR)/src/arch/x86/boot.o \
           $(BUILD_DIR)/src/arch/x86/isrs.o \
           $(BUILD_DIR)/src/arch/x86/gdt_asm.o

# File C++
CPP_OBJS = $(BUILD_DIR)/src/kernel/kernel.o \
           $(BUILD_DIR)/src/kernel/vga.o \
           $(BUILD_DIR)/src/kernel/shell.o \
           $(BUILD_DIR)/src/kernel/fs.o \
           $(BUILD_DIR)/src/kernel/services.o \
           $(BUILD_DIR)/src/kernel/api.o \
           $(BUILD_DIR)/src/kernel/gui.o \
           $(BUILD_DIR)/src/kernel/pmm.o \
           $(BUILD_DIR)/src/kernel/vmm.o \
           $(BUILD_DIR)/src/kernel/kheap.o \
           $(BUILD_DIR)/src/kernel/scheduler.o \
           $(BUILD_DIR)/src/kernel/elf.o \
           $(BUILD_DIR)/src/kernel/syscall.o \
           $(BUILD_DIR)/src/kernel/user_api.o \
           $(BUILD_DIR)/src/kernel/fat.o \
           $(BUILD_DIR)/src/kernel/ipc.o \
           $(BUILD_DIR)/src/kernel/logger.o \
           $(BUILD_DIR)/src/drivers/timer.o \
           $(BUILD_DIR)/src/drivers/ata.o \
           $(BUILD_DIR)/src/drivers/vbe.o \
           $(BUILD_DIR)/src/arch/x86/gdt.o \
           $(BUILD_DIR)/src/arch/x86/idt.o \
           $(BUILD_DIR)/src/drivers/pic.o \
           $(BUILD_DIR)/src/drivers/keyboard.o \
           $(BUILD_DIR)/src/drivers/pci.o \
           $(BUILD_DIR)/src/drivers/speaker.o \
           $(BUILD_DIR)/src/drivers/mouse.o

# Gabungkan semua file object
OBJ_FILES = $(ASM_OBJS) $(CPP_OBJS)

# Nama target
KERNEL_BIN = $(BIN_DIR)/kernel.bin
OS_ISO = $(BIN_DIR)/TrieternalX-OS.iso
GRUB_CFG = $(SYSROOT_DIR)/boot/grub/grub.cfg

# === Target Build ===

# Target default (yang dijalankan jika Anda hanya mengetik 'make')
all: $(OS_ISO)

# Membuat OS ISO
$(OS_ISO): $(KERNEL_BIN) $(GRUB_CFG)
	@echo "Membuat ISO..."
	@mkdir -p $(SYSROOT_DIR)/boot/grub
	@cp $(KERNEL_BIN) $(SYSROOT_DIR)/boot/kernel.bin
	@grub2-mkrescue -o $(OS_ISO) $(SYSROOT_DIR)


$(KERNEL_BIN): $(OBJ_FILES)
	@echo "Linking kernel..."
	@mkdir -p $(BIN_DIR)
	@$(LD) $(LDFLAGS) -o $(KERNEL_BIN) $(BUILD_DIR)/src/arch/x86/boot.o $(filter-out $(BUILD_DIR)/src/arch/x86/boot.o, $(OBJ_FILES))

$(BUILD_DIR)/src/%.o: $(SRC_DIR)/%.cpp
	@echo "Kompilasi C++: $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/src/%.o: $(SRC_DIR)/%.asm
	@echo "Assembly: $<"
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) $< -o $@

$(GRUB_CFG):
	@echo "Membuat grub.cfg..."
	@mkdir -p $(dir $@)
	@(echo 'set timeout=3'; \
	  echo 'set default=0'; \
	  echo 'menuentry "TrieternalXOS" {'; \
	  echo '    multiboot /boot/kernel.bin'; \
	  echo '    boot'; \
	  echo '}'; \
	) > $@

run: $(OS_ISO)
	@echo "Menjalankan QEMU..."
	@qemu-system-i386 -cdrom $(OS_ISO) -m 512M

clean:
	@echo "Membersihkan..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@rm -f $(SYSROOT_DIR)/boot/kernel.bin
	# Hapus juga grub.cfg yang dibuat otomatis
	@rm -f $(SYSROOT_DIR)/boot/grub/grub.cfg

.PHONY: all run clean
