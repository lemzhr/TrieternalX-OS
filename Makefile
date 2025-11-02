# File: Makefile (di root proyek)

# === Variabel ===
AS = nasm
# Kita sepakat menggunakan i686-linux-gnu-
CC = i686-linux-gnu-g++
LD = i686-linux-gnu-ld

# Flags
# -I$(SRC_DIR)/include memberitahu compiler di mana mencari file .h
CFLAGS = -Wall -Wextra -std=c++17 -ffreestanding -fno-rtti -fno-exceptions -g -I$(SRC_DIR)/include
ASFLAGS = -f elf32
LDFLAGS = -T scripts/linker.ld -nostdlib -g

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
           $(BUILD_DIR)/src/arch/x86/gdt.o \
           $(BUILD_DIR)/src/arch/x86/idt.o \
           $(BUILD_DIR)/src/drivers/pic.o \
           $(BUILD_DIR)/src/drivers/keyboard.o

# Gabungkan semua file object
OBJ_FILES = $(ASM_OBJS) $(CPP_OBJS)

# Nama target
KERNEL_BIN = $(BIN_DIR)/kernel.bin
OS_ISO = $(BIN_DIR)/TrieternalX-OS.iso
GRUB_CFG = $(SYSROOT_DIR)/boot/grub/grub.cfg

# === Target Build ===

# Target default (yang dijalankan jika Anda hanya mengetik 'make')
all: $(OS_ISO)

# 1. Membuat OS ISO
$(OS_ISO): $(KERNEL_BIN) $(GRUB_CFG)
	@echo "Membuat ISO..."
	@mkdir -p $(SYSROOT_DIR)/boot/grub
	@cp $(KERNEL_BIN) $(SYSROOT_DIR)/boot/kernel.bin
	@grub-mkrescue -o $(OS_ISO) $(SYSROOT_DIR)

# 2. Link semua file .o menjadi satu kernel.bin
$(KERNEL_BIN): $(OBJ_FILES)
	@echo "Linking kernel..."
	@mkdir -p $(BIN_DIR)
	# PENTING: boot.o HARUS menjadi yang pertama di-link
	@$(LD) $(LDFLAGS) -o $(KERNEL_BIN) $(BUILD_DIR)/src/arch/x86/boot.o $(filter-out $(BUILD_DIR)/src/arch/x86/boot.o, $(OBJ_FILES))

# 3. Aturan kompilasi C++ (.cpp -> .o)
#    INI ADALAH ATURAN YANG DIPERBAIKI
$(BUILD_DIR)/src/%.o: $(SRC_DIR)/%.cpp
	@echo "Kompilasi C++: $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

# 4. Aturan assembly (.asm -> .o)
#    INI ADALAH ATURAN YANG DIPERBAIKI
$(BUILD_DIR)/src/%.o: $(SRC_DIR)/%.asm
	@echo "Assembly: $<"
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) $< -o $@

# 5. ATURAN BARU: Membuat grub.cfg secara otomatis
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

# === Target Tambahan ===
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