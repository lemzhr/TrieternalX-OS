; File: src/arch/x86/boot.asm
; Target: 32-bit (x86)

; Mendefinisikan standar Multiboot untuk GRUB
MODULEALIGN equ 1 << 0
MEMINFO     equ 1 << 1
FLAGS       equ MODULEALIGN | MEMINFO
MAGIC       equ 0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

section .multiboot
    align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; Sediakan 16KB stack untuk kernel C++
section .bss
    align 16
    stack_bottom:
    resb 16384 ; 16 KB
    stack_top:

section .text
    global _start
    extern kmain ; kmain() ada di file C++

_start:
    ; Pindahkan stack pointer ke atas stack kita
    mov esp, stack_top

    ; Panggil kernel C++ kita (kmain)
    call kmain

    ; Hentikan CPU jika kernel selesai (seharusnya tidak pernah)
    cli
.hang:
    hlt
    jmp .hang