
extern keyboard_handler
extern timer_handler
extern syscall_handler

global isr_stub_32
global isr_stub_33
global isr_stub_128

section .text

isr_stub_32:

    pusha

    push esp
    call timer_handler

    mov esp, eax

    popa
    iret

isr_stub_33:
    pusha
    call keyboard_handler
    popa
    iret

isr_stub_128:

    pusha

    push esp
    call syscall_handler
    add esp, 4

    popa
    iret

extern page_fault_handler
global isr_stub_14
isr_stub_14:
    pusha
    push esp
    call page_fault_handler
    add esp, 4
    popa
    add esp, 4
    iret

extern mouse_handler
global isr_stub_44
isr_stub_44:
    pusha
    call mouse_handler
    popa
    iret
