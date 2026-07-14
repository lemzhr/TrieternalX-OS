
#include "gdt.h"

gdt_entry_t gdt_entries[6];
gdt_ptr_t gdt_ptr;
tss_entry_t tss_entry;

extern "C" void gdt_flush(gdt_ptr_t *gdt_ptr);

void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt_entries[num].base_low = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access = access;
}

void init_gdt()
{
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_ptr.base = (uint32_t)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);

    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    uint32_t tss_base = (uint32_t)&tss_entry;
    uint32_t tss_limit = sizeof(tss_entry) - 1;

    for (uint32_t i = 0; i < sizeof(tss_entry); i++)
    {
        ((uint8_t*)&tss_entry)[i] = 0;
    }

    tss_entry.ss0 = 0x10;
    tss_entry.esp0 = 0;

    gdt_set_gate(5, tss_base, tss_limit, 0x89, 0x40);

    gdt_flush(&gdt_ptr);

    tss_flush();
}
