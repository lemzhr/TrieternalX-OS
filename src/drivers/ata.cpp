
#include "ata.h"
#include "io.h"

static bool disk_present = false;
static char disk_model[41] = "TrieternalX Virtual Disk";

void ata_init()
{

    outb(0x1F6, 0xA0);
    outb(0x1F2, 0);
    outb(0x1F3, 0);
    outb(0x1F4, 0);
    outb(0x1F5, 0);
    outb(0x1F7, 0xEC);

    uint8_t status = inb(0x1F7);
    if (status == 0)
    {
        disk_present = false;
        return;
    }

    while ((inb(0x1F7) & 0x80) != 0);

    uint8_t cl = inb(0x1F4);
    uint8_t ch = inb(0x1F5);
    if (cl != 0 || ch != 0)
    {
        disk_present = false;
        return;
    }

    int timeout = 10000;
    while (timeout > 0)
    {
        status = inb(0x1F7);
        if ((status & 0x01) != 0)
        {
            disk_present = false;
            return;
        }
        if ((status & 0x08) != 0)
        {
            break;
        }
        timeout--;
    }

    if (timeout == 0)
    {
        disk_present = false;
        return;
    }

    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++)
    {
        identify_data[i] = inw(0x1F0);
    }

    int idx = 0;
    for (int i = 27; i <= 46; i++)
    {
        disk_model[idx++] = (char)(identify_data[i] >> 8);
        disk_model[idx++] = (char)(identify_data[i] & 0xFF);
    }
    disk_model[idx] = '\0';

    for (int i = 39; i >= 0; i--)
    {
        if (disk_model[i] == ' ')
        {
            disk_model[i] = '\0';
        }
        else if (disk_model[i] != '\0')
        {
            break;
        }
    }

    disk_present = true;
}

bool ata_disk_exists()
{
    return disk_present;
}

const char* ata_get_model()
{
    return disk_model;
}

bool ata_read_sectors(uint32_t lba, uint8_t sector_count, uint8_t* buffer)
{
    if (sector_count == 0) return false;

    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, sector_count);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x20);

    uint16_t* ptr = (uint16_t*)buffer;

    for (int s = 0; s < sector_count; s++)
    {

        while ((inb(0x1F7) & 0x80) != 0);
        while ((inb(0x1F7) & 0x08) == 0)
        {

            if ((inb(0x1F7) & 0x01) != 0)
            {
                return false;
            }
        }

        for (int i = 0; i < 256; i++)
        {
            *ptr = inw(0x1F0);
            ptr++;
        }
    }

    return true;
}

bool ata_write_sectors(uint32_t lba, uint8_t sector_count, const uint8_t* buffer)
{
    if (sector_count == 0) return false;

    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, sector_count);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x30);

    const uint16_t* ptr = (const uint16_t*)buffer;

    for (int s = 0; s < sector_count; s++)
    {

        while ((inb(0x1F7) & 0x80) != 0);
        while ((inb(0x1F7) & 0x08) == 0)
        {
            if ((inb(0x1F7) & 0x01) != 0)
            {
                return false;
            }
        }

        for (int i = 0; i < 256; i++)
        {
            outw(0x1F0, *ptr);
            ptr++;
        }
    }

    outb(0x1F7, 0xE7);
    while ((inb(0x1F7) & 0x80) != 0);

    return true;
}
