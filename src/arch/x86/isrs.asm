; File: src/arch/x86/isrs.asm
; Target: 32-bit (x86)

; Beri tahu assembler bahwa ada fungsi C++ bernama 'keyboard_handler'
; (Fungsi ini ada di file keyboard.cpp Anda)
extern keyboard_handler

; Buat 'isr_stub_33' terlihat oleh file C++ lain (khususnya idt.cpp)
global isr_stub_33

section .text
isr_stub_33:
    ; Ini adalah fungsi yang akan dipanggil oleh CPU
    ; saat interrupt keyboard (IRQ 1, yang kita petakan ke 33) terjadi.

    ; 1. Simpan semua register CPU (agar tidak rusak oleh C++)
    pusha

    ; 2. Panggil fungsi handler C++ kita
    call keyboard_handler

    ; 3. Kembalikan semua register yang tadi disimpan
    popa
    
    ; 4. Kembali dari interrupt
    iret
