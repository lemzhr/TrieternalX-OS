
#include "elf.h"
#include "kheap.h"
#include "vmm.h"
#include "pmm.h"
#include "fat.h"
#include "vga.h"

bool elf_validate(const ELF32_Ehdr* hdr)
{
    if (hdr->e_ident[EI_MAG0] != 0x7f ||
        hdr->e_ident[EI_MAG1] != 'E'  ||
        hdr->e_ident[EI_MAG2] != 'L'  ||
        hdr->e_ident[EI_MAG3] != 'F')
        return false;

    if (hdr->e_ident[EI_CLASS] != ELFCLASS32)
        return false;

    if (hdr->e_ident[EI_DATA] != ELFDATA2LSB)
        return false;

    if (hdr->e_type != ET_EXEC)
        return false;

    if (hdr->e_machine != EM_386)
        return false;

    return true;
}

ELFLoadResult elf_load_from_memory(const void* data, uint32_t size)
{
    ELFLoadResult result;
    result.entry_point = 0;
    result.load_base = 0;
    result.load_size = 0;
    result.success = false;

    if (size < sizeof(ELF32_Ehdr))
        return result;

    const ELF32_Ehdr* hdr = (const ELF32_Ehdr*)data;

    if (!elf_validate(hdr))
        return result;

    uint32_t entry = hdr->e_entry;
    uint32_t ph_offset = hdr->e_phoff;
    uint32_t ph_count = hdr->e_phnum;
    uint32_t ph_entsize = hdr->e_phentsize;

    uint32_t lowest_addr = 0xFFFFFFFF;
    uint32_t highest_addr = 0;

    for (uint32_t i = 0; i < ph_count; i++)
    {
        uint32_t off = ph_offset + i * ph_entsize;
        if (off + sizeof(ELF32_Phdr) > size)
            return result;

        const ELF32_Phdr* phdr = (const ELF32_Phdr*)((const uint8_t*)data + off);

        if (phdr->p_type != PT_LOAD)
            continue;

        if (phdr->p_vaddr < lowest_addr)
            lowest_addr = phdr->p_vaddr;
        if (phdr->p_vaddr + phdr->p_memsz > highest_addr)
            highest_addr = phdr->p_vaddr + phdr->p_memsz;
    }

    if (lowest_addr >= highest_addr)
        return result;

    uint32_t total_size = highest_addr - lowest_addr;
    uint32_t base_addr = lowest_addr;

    for (uint32_t i = 0; i < ph_count; i++)
    {
        uint32_t off = ph_offset + i * ph_entsize;
        const ELF32_Phdr* phdr = (const ELF32_Phdr*)((const uint8_t*)data + off);

        if (phdr->p_type != PT_LOAD)
            continue;

        uint32_t seg_offset = phdr->p_vaddr - base_addr;
        (void)seg_offset;

        for (uint32_t page_off = 0; page_off < phdr->p_memsz; page_off += 0x1000)
        {
            uint32_t virt = (phdr->p_vaddr + page_off) & 0xFFFFF000;
            uint32_t phys = (uint32_t)pmm_alloc_frame();
            if (!phys)
                return result;

            uint8_t* page_mem = (uint8_t*)phys;
            for (uint32_t b = 0; b < 0x1000; b++)
                page_mem[b] = 0;

            uint32_t flags = 7;
            if (phdr->p_flags & PF_X)
                flags |= 4;
            vmm_map_page(virt, phys, flags);
        }

        uint8_t* dest = (uint8_t*)phdr->p_vaddr;
        uint32_t copy_filesz = phdr->p_filesz;
        if (copy_filesz > phdr->p_memsz)
            copy_filesz = phdr->p_memsz;

        if (copy_filesz > 0 && phdr->p_offset + copy_filesz <= size)
        {
            const uint8_t* src = (const uint8_t*)data + phdr->p_offset;
            for (uint32_t b = 0; b < copy_filesz; b++)
                dest[b] = src[b];
        }

        uint32_t remaining = phdr->p_memsz - phdr->p_filesz;
        if (remaining > 0)
        {
            uint8_t* bss_start = dest + phdr->p_filesz;
            for (uint32_t b = 0; b < remaining; b++)
                bss_start[b] = 0;
        }
    }

    result.entry_point = entry;
    result.load_base = base_addr;
    result.load_size = total_size;
    result.success = true;
    return result;
}

ELFLoadResult elf_load_from_disk(const char* filename)
{
    ELFLoadResult result;
    result.entry_point = 0;
    result.load_base = 0;
    result.load_size = 0;
    result.success = false;

    if (!fat_is_active())
        return result;

    uint32_t file_size = fat_get_file_size(filename);
    if (file_size == 0)
        return result;

    uint8_t* buf = (uint8_t*)kmalloc(file_size);
    if (!buf)
        return result;

    if (!fat_read_file(filename, buf, file_size))
    {
        kfree(buf);
        return result;
    }

    result = elf_load_from_memory(buf, file_size);
    kfree(buf);
    return result;
}
