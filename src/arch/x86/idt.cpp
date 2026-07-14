
#include "interrupts.h"
#include "pic.h"
#include "vga.h"
#include "logger.h"

idt_entry_t idt_entries[256];
idt_ptr_t idt_ptr;

extern "C" void isr_stub_14();
extern "C" void isr_stub_32();
extern "C" void isr_stub_33();
extern "C" void isr_stub_44();
extern "C" void isr_stub_128();

static inline void idt_load(idt_ptr_t *ptr)
{
    asm volatile("lidt %0" : : "m"(*ptr));
}

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    idt_entries[num].base_lo = (base & 0xFFFF);
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags = flags ;
}

void init_idt()
{

    PIC_remap(0x20, 0x28);

    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base = (uint32_t)&idt_entries;

    idt_set_gate(14, (uint32_t)&isr_stub_14, 0x08, 0x8E);
    idt_set_gate(32, (uint32_t)&isr_stub_32, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)&isr_stub_33, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)&isr_stub_44, 0x08, 0x8E);
    idt_set_gate(128, (uint32_t)&isr_stub_128, 0x08, 0xEE);

    idt_load(&idt_ptr);

    klog_info("BOOT", "IDT dan PIC berhasil diinisialisasi.");
}
