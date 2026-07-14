
#include "vmm.h"
#include "pmm.h"
#include "vga.h"
#include "vbe.h"
#include "kheap.h"

static uint32_t kernel_page_directory[1024];
__attribute__((aligned(4096))) static uint32_t identity_page_tables[32][1024];

void vmm_init()
{
    for (uint32_t t = 0; t < 32; t++)
    {
        for (uint32_t p = 0; p < 1024; p++)
        {
            uint32_t phys = (t * 1024 + p) * PAGE_SIZE;
            identity_page_tables[t][p] = phys | 3;
        }
        kernel_page_directory[t] = ((uint32_t)&identity_page_tables[t]) | 3;
    }

    for (uint32_t t = 32; t < 1024; t++)
    {
        kernel_page_directory[t] = 0;
    }

    if (vbe_is_active())
    {
        uint32_t fb_phys = (uint32_t)vbe_get_framebuffer_addr();
        uint32_t fb_size = vbe_get_height() * vbe_get_framebuffer_pitch();

        for (uint32_t addr = fb_phys; addr < fb_phys + fb_size; addr += PAGE_SIZE)
        {
            vmm_map_page(addr, addr, 3);
        }
    }

    asm volatile("mov %0, %%cr3" : : "r"(kernel_page_directory));

    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

static uint32_t* get_current_pd()
{
    return kernel_page_directory;
}

void vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags)
{
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    if (!(kernel_page_directory[pd_idx] & 1))
    {
        void* new_pt = pmm_alloc_frame();
        uint32_t* pt = (uint32_t*)new_pt;
        for (int i = 0; i < 1024; i++)
            pt[i] = 0;
        kernel_page_directory[pd_idx] = ((uint32_t)new_pt) | flags | 1;
    }
    else
    {
        kernel_page_directory[pd_idx] |= flags;
    }

    uint32_t* pt = (uint32_t*)(kernel_page_directory[pd_idx] & 0xFFFFF000);
    pt[pt_idx] = (phys & 0xFFFFF000) | flags | 1;

    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

void vmm_unmap_page(uint32_t virt)
{
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    if (kernel_page_directory[pd_idx] & 1)
    {
        uint32_t* pt = (uint32_t*)(kernel_page_directory[pd_idx] & 0xFFFFF000);
        pt[pt_idx] = 0;
        asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
    }
}

uint32_t vmm_get_phys_addr(uint32_t virt)
{
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x3FF;

    if (!(kernel_page_directory[pd_idx] & 1))
    {
        if (virt < 128 * 1024 * 1024)
            return virt;
        return 0;
    }

    uint32_t* pt = (uint32_t*)(kernel_page_directory[pd_idx] & 0xFFFFF000);
    if (!(pt[pt_idx] & 1))
    {
        if (virt < 128 * 1024 * 1024)
            return virt;
        return 0;
    }

    return (pt[pt_idx] & 0xFFFFF000) | (virt & 0xFFF);
}

uint32_t* vmm_create_user_page_dir()
{
    uint32_t* new_pd = (uint32_t*)pmm_alloc_frame();
    if (!new_pd)
        return nullptr;

    uint32_t* pd = (uint32_t*)new_pd;
    for (int i = 0; i < 1024; i++)
        pd[i] = 0;

    vmm_copy_kernel_pages(pd);
    return pd;
}

void vmm_copy_kernel_pages(uint32_t* new_pd)
{
    for (int i = 0; i < 1024; i++)
    {
        if (i >= 768)
        {
            new_pd[i] = kernel_page_directory[i];
        }
        else if (kernel_page_directory[i] & 1)
        {
            void* new_pt_phys = pmm_alloc_frame();
            if (!new_pt_phys) continue;

            uint32_t* src_pt = (uint32_t*)(kernel_page_directory[i] & 0xFFFFF000);
            uint32_t* new_pt = (uint32_t*)new_pt_phys;

            for (int j = 0; j < 1024; j++)
                new_pt[j] = src_pt[j];

            uint32_t flags = kernel_page_directory[i] & 0xFFF;
            new_pd[i] = ((uint32_t)new_pt_phys) | flags;
        }
    }
}

void vmm_switch_page_dir(uint32_t* page_dir)
{
    if (page_dir)
        asm volatile("mov %0, %%cr3" : : "r"(page_dir));
    else
        asm volatile("mov %0, %%cr3" : : "r"(kernel_page_directory));
}

void vmm_free_user_pages(uint32_t* page_dir)
{
    if (!page_dir)
        return;

    for (int i = 0; i < 768; i++)
    {
        if (!(page_dir[i] & 1))
            continue;

        if (i >= 768)
            continue;

        uint32_t* pt = (uint32_t*)(page_dir[i] & 0xFFFFF000);

        for (int j = 0; j < 1024; j++)
        {
            if (pt[j] & 1)
            {
                uint32_t phys = pt[j] & 0xFFFFF000;
                pmm_free_frame((void*)phys);
            }
        }

        pmm_free_frame((void*)(page_dir[i] & 0xFFFFF000));
        page_dir[i] = 0;
    }
}

struct PageFaultRegisters
{
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t error_code;
    uint32_t eip, cs, eflags;
    uint32_t user_esp, user_ss;
};

static void write_hex_vmm(uint32_t n)
{
    VGA::terminal.write("0x");
    if (n == 0)
    {
        VGA::terminal.putchar('0');
        return;
    }
    char buf[32];
    int i = 0;
    const char* hexchars = "0123456789ABCDEF";
    while (n > 0)
    {
        buf[i++] = hexchars[n % 16];
        n /= 16;
    }
    for (int j = i - 1; j >= 0; j--)
        VGA::terminal.putchar(buf[j]);
}

extern "C" void page_fault_handler(PageFaultRegisters* regs)
{
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

    uint32_t ec = regs->error_code;
    bool present = ec & 1;
    bool write = ec & 2;
    bool user = ec & 4;
    bool reserved = ec & 8;
    bool id = ec & 16;

    VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_RED);
    VGA::terminal.clear();
    VGA::terminal.write("================================================================================");
    VGA::terminal.write("                                KERNEL PANIC!                                   ");
    VGA::terminal.write("================================================================================\n\n");
    VGA::terminal.write("[Page Fault Manager] PAGE FAULT occurred!\n");
    VGA::terminal.write("Faulting Address: ");
    write_hex_vmm(faulting_address);
    VGA::terminal.write("\n");

    VGA::terminal.write("Error Code Decode: ");
    if (!present) VGA::terminal.write("[Non-present page] ");
    else VGA::terminal.write("[Page-level protection violation] ");

    if (write) VGA::terminal.write("[Write Access] ");
    else VGA::terminal.write("[Read Access] ");

    if (user) VGA::terminal.write("[User Mode] ");
    else VGA::terminal.write("[Supervisor Mode] ");

    if (reserved) VGA::terminal.write("[Reserved bits overwritten] ");
    if (id) VGA::terminal.write("[Instruction Fetch] ");
    VGA::terminal.write("\n\n");

    VGA::terminal.write("CPU Register State:\n");
    VGA::terminal.write("  EIP: "); write_hex_vmm(regs->eip);
    VGA::terminal.write("  CS: "); write_hex_vmm(regs->cs);
    VGA::terminal.write("  EFLAGS: "); write_hex_vmm(regs->eflags);
    VGA::terminal.write("\n");
    VGA::terminal.write("  EAX: "); write_hex_vmm(regs->eax);
    VGA::terminal.write("  EBX: "); write_hex_vmm(regs->ebx);
    VGA::terminal.write("  ECX: "); write_hex_vmm(regs->ecx);
    VGA::terminal.write("  EDX: "); write_hex_vmm(regs->edx);
    VGA::terminal.write("\n");
    VGA::terminal.write("  ESP: "); write_hex_vmm(regs->esp);
    VGA::terminal.write("  EBP: "); write_hex_vmm(regs->ebp);
    VGA::terminal.write("  ESI: "); write_hex_vmm(regs->esi);
    VGA::terminal.write("  EDI: "); write_hex_vmm(regs->edi);
    VGA::terminal.write("\n\n");

    if (faulting_address < 0x1000)
        VGA::terminal.write("Potential NULL POINTER DEREFERENCE detected!\n");
    else
        VGA::terminal.write("Potential INVALID MEMORY ACCESS / PROTECTION FAULT detected!\n");

    VGA::terminal.write("\nSystem Halted.");

    while(1)
    {
        asm volatile("hlt");
    }
}
