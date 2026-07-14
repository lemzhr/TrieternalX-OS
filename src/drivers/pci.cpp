
#include "pci.h"
#include "io.h"
#include "vga.h"
#include "logger.h"

#define MAX_PCI_DEVICES 32

static PCIDevice pci_devices[MAX_PCI_DEVICES];
static int pci_device_count = 0;

static uint32_t pci_read_config_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t address = (uint32_t)((uint32_t)0x80000000 |
                       ((uint32_t)bus << 16) |
                       ((uint32_t)slot << 11) |
                       ((uint32_t)func << 8) |
                       (offset & 0xFC));
    outl(0xCF8, address);
    return inl(0xCFC);
}

void pci_init()
{
    pci_device_count = 0;
    klog_info("PCI", "Scanning PCI Bus...");

    for (uint32_t bus = 0; bus < 256; bus++)
    {
        for (uint8_t slot = 0; slot < 32; slot++)
        {
            for (uint8_t func = 0; func < 8; func++)
            {
                uint32_t val = pci_read_config_dword(bus, slot, func, 0);
                uint16_t vendor_id = val & 0xFFFF;
                uint16_t device_id = val >> 16;

                if (vendor_id == 0xFFFF || vendor_id == 0x0000)
                {
                    continue;
                }

                if (pci_device_count >= MAX_PCI_DEVICES)
                {
                    break;
                }

                uint32_t val8 = pci_read_config_dword(bus, slot, func, 8);
                uint8_t class_code = (val8 >> 24) & 0xFF;
                uint8_t subclass = (val8 >> 16) & 0xFF;

                PCIDevice& dev = pci_devices[pci_device_count];
                dev.bus = bus;
                dev.slot = slot;
                dev.func = func;
                dev.vendor_id = vendor_id;
                dev.device_id = device_id;
                dev.class_code = class_code;
                dev.subclass = subclass;

                for (int b = 0; b < 6; b++)
                {
                    dev.bar[b] = pci_read_config_dword(bus, slot, func, 0x10 + b * 4);
                }

                pci_device_count++;

                if (func == 0)
                {
                    uint32_t valC = pci_read_config_dword(bus, slot, 0, 0x0C);
                    uint8_t header_type = (valC >> 16) & 0xFF;
                    if ((header_type & 0x80) == 0)
                    {
                        break;
                    }
                }
            }
        }
    }

    char count_str[32] = "Found ";
    int idx = 6;
    int temp = pci_device_count;
    if (temp == 0) {
        count_str[idx++] = '0';
    } else {
        char buf[16];
        int b_idx = 0;
        while (temp > 0) {
            buf[b_idx++] = (temp % 10) + '0';
            temp /= 10;
        }
        for (int i = b_idx - 1; i >= 0; i--) {
            count_str[idx++] = buf[i];
        }
    }
    const char* suffix = " PCI devices.";
    for (int i = 0; suffix[i] != '\0'; i++) count_str[idx++] = suffix[i];
    count_str[idx] = '\0';
    klog_info("PCI", count_str);
}

int pci_get_device_list(PCIDevice* list, int max_devs)
{
    int count = (pci_device_count < max_devs) ? pci_device_count : max_devs;
    for (int i = 0; i < count; i++)
    {
        list[i] = pci_devices[i];
    }
    return count;
}

const char* pci_class_to_string(uint8_t class_code)
{
    switch (class_code)
    {
        case 0x00: return "Unclassified";
        case 0x01: return "Mass Storage Controller";
        case 0x02: return "Network Controller";
        case 0x03: return "Display Controller";
        case 0x04: return "Multimedia Controller";
        case 0x05: return "Memory Controller";
        case 0x06: return "Bridge Device";
        case 0x07: return "Simple Communication Controller";
        case 0x08: return "Base System Peripheral";
        case 0x09: return "Input Device Controller";
        case 0x0A: return "Docking Station";
        case 0x0B: return "Processor";
        case 0x0C: return "Serial Bus Controller";
        case 0x0D: return "Wireless Controller";
        case 0x0E: return "Intelligent Controller";
        case 0x0F: return "Satellite Communication Controller";
        case 0x10: return "Encryption Controller";
        case 0x11: return "Signal Processing Controller";
        default:   return "Unknown Device Type";
    }
}

static void write_hex(uint32_t n)
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
    {
        VGA::terminal.putchar(buf[j]);
    }
}

static void write_dec(uint32_t n)
{
    if (n == 0)
    {
        VGA::terminal.putchar('0');
        return;
    }
    char buf[32];
    int i = 0;
    while (n > 0)
    {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    for (int j = i - 1; j >= 0; j--)
    {
        VGA::terminal.putchar(buf[j]);
    }
}

void pci_print_devices()
{
    VGA::terminal.write("PCI BUS SCAN RESULTS:\n");
    VGA::terminal.write("Bus:Dev:Func  Vendor:Device  Class Description\n");
    VGA::terminal.write("-----------------------------------------------------------------\n");
    for (int i = 0; i < pci_device_count; i++)
    {
        PCIDevice& dev = pci_devices[i];

        write_dec(dev.bus);
        VGA::terminal.putchar(':');
        write_dec(dev.slot);
        VGA::terminal.putchar(':');
        write_dec(dev.func);

        VGA::terminal.write("       ");

        write_hex(dev.vendor_id);
        VGA::terminal.putchar(':');
        write_hex(dev.device_id);

        VGA::terminal.write("    ");

        VGA::terminal.write(pci_class_to_string(dev.class_code));
        VGA::terminal.write(" (");
        write_hex(dev.class_code);
        VGA::terminal.write(")\n");
    }
}
