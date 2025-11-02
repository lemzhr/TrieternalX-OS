; File: src/arch/x86/gdt_asm.asm
; Target: 32-bit (x86)

; Membuat 'gdt_flush' bisa dipanggil dari C++
global gdt_flush 

section .text
gdt_flush:
    ; Ambil GDT pointer dari argumen pertama di stack
    lgdt [esp+4] 

    ; Muat ulang semua segment selector
    ; 0x10 adalah offset untuk Kernel Data segment (entri ke-3)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; 0x08 adalah offset for Kernel Code segment (entri ke-2)
    ; Kita lakukan "far jump" untuk me-reload CS
    jmp 0x08:.flush
.flush:
    ret