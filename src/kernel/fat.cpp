
#include "fat.h"
#include "ata.h"
#include "vga.h"
#include "kheap.h"

struct __attribute__((packed)) FATBootSector
{
    uint8_t  jmp[3];
    char     oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_entry_count;
    uint16_t total_sectors_short;
    uint8_t  media_descriptor;
    uint16_t sectors_per_fat_short;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_long;

    uint32_t sectors_per_fat_large;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];
};

struct __attribute__((packed)) FATDirEntry
{
    char     name[11];
    uint8_t  attr;
    uint8_t  nt_res;
    uint8_t  creation_time_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
};

static bool fat_active = false;
static bool is_fat32 = false;
static uint16_t bytes_per_sector = 512;
static uint8_t sectors_per_cluster = 1;
static uint16_t reserved_sectors = 0;
static uint8_t fat_count = 2;
static uint32_t sectors_per_fat = 0;
static uint32_t root_dir_sector = 0;
static uint32_t root_dir_sectors = 0;
static uint32_t data_sector = 0;
static uint32_t root_cluster = 0;

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

static void to_fat_name(const char* src, char* dest)
{
    for (int i = 0; i < 11; i++) dest[i] = ' ';

    int i = 0;
    int dest_idx = 0;

    while (src[i] != '\0' && src[i] != '.' && dest_idx < 8)
    {
        char c = src[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        dest[dest_idx++] = c;
        i++;
    }

    if (src[i] == '.')
    {
        i++;
        dest_idx = 8;
        while (src[i] != '\0' && dest_idx < 11)
        {
            char c = src[i];
            if (c >= 'a' && c <= 'z') c -= 32;
            dest[dest_idx++] = c;
            i++;
        }
    }
}

static bool fat_name_match(const char* name1, const char* name2)
{
    for (int i = 0; i < 11; i++)
    {
        if (name1[i] != name2[i]) return false;
    }
    return true;
}

void fat_init()
{
    fat_active = false;
    if (!ata_disk_exists()) return;

    uint8_t boot_sector_buf[512];
    if (!ata_read_sectors(0, 1, boot_sector_buf))
    {
        return;
    }

    if (boot_sector_buf[510] != 0x55 || boot_sector_buf[511] != 0xAA)
    {
        return;
    }

    FATBootSector* bpb = (FATBootSector*)boot_sector_buf;
    bytes_per_sector = bpb->bytes_per_sector;
    sectors_per_cluster = bpb->sectors_per_cluster;
    reserved_sectors = bpb->reserved_sectors;
    fat_count = bpb->fat_count;

    uint16_t root_entry_count = bpb->root_entry_count;
    uint32_t total_sectors = (bpb->total_sectors_short != 0) ? bpb->total_sectors_short : bpb->total_sectors_long;
    sectors_per_fat = (bpb->sectors_per_fat_short != 0) ? bpb->sectors_per_fat_short : bpb->sectors_per_fat_large;

    root_dir_sectors = ((root_entry_count * 32) + (bytes_per_sector - 1)) / bytes_per_sector;

    uint32_t fat_size = sectors_per_fat;
    uint32_t data_sectors = total_sectors - (reserved_sectors + (fat_count * fat_size) + root_dir_sectors);
    uint32_t total_clusters = data_sectors / sectors_per_cluster;

    if (total_clusters >= 65525)
    {
        is_fat32 = true;
        root_cluster = bpb->root_cluster;
        data_sector = reserved_sectors + (fat_count * sectors_per_fat);
    }
    else
    {
        is_fat32 = false;
        root_dir_sector = reserved_sectors + (fat_count * sectors_per_fat);
        data_sector = root_dir_sector + root_dir_sectors;
    }

    fat_active = true;
}

bool fat_is_active()
{
    return fat_active;
}

static uint32_t get_cluster_sector(uint32_t cluster)
{
    return data_sector + ((cluster - 2) * sectors_per_cluster);
}

static uint32_t get_next_cluster(uint32_t cluster)
{
    uint8_t fat_buf[512];
    if (is_fat32)
    {
        uint32_t fat_offset = cluster * 4;
        uint32_t fat_sector = reserved_sectors + (fat_offset / bytes_per_sector);
        uint32_t ent_offset = fat_offset % bytes_per_sector;

        if (!ata_read_sectors(fat_sector, 1, fat_buf)) return 0x0FFFFFFF;
        return (*(uint32_t*)&fat_buf[ent_offset]) & 0x0FFFFFFF;
    }
    else
    {
        uint32_t fat_offset = cluster * 2;
        uint32_t fat_sector = reserved_sectors + (fat_offset / bytes_per_sector);
        uint32_t ent_offset = fat_offset % bytes_per_sector;

        if (!ata_read_sectors(fat_sector, 1, fat_buf)) return 0xFFFF;
        return *(uint16_t*)&fat_buf[ent_offset];
    }
}

void fat_list_root()
{
    if (!fat_active)
    {
        VGA::terminal.write("FAT Filesystem tidak aktif / tidak terdeteksi.\n");
        return;
    }

    VGA::terminal.write("Daftar berkas di partisi disk (FAT):\n");
    VGA::terminal.write("Nama Berkas     Atribut     Ukuran (Bytes)\n");
    VGA::terminal.write("-------------------------------------------\n");

    uint8_t sector_buf[512];

    if (!is_fat32)
    {

        for (uint32_t s = 0; s < root_dir_sectors; s++)
        {
            if (!ata_read_sectors(root_dir_sector + s, 1, sector_buf)) break;
            FATDirEntry* entries = (FATDirEntry*)sector_buf;

            for (int i = 0; i < 16; i++)
            {
                FATDirEntry* entry = &entries[i];
                if (entry->name[0] == 0x00) return;
                if (entry->name[0] == (char)0xE5) continue;
                if ((entry->attr & 0x0F) == 0x0F) continue;

                for (int c = 0; c < 11; c++)
                {
                    if (c == 8 && (entry->attr & 0x10) == 0)
                    {
                        VGA::terminal.putchar('.');
                    }
                    if (entry->name[c] != ' ')
                    {
                        VGA::terminal.putchar(entry->name[c]);
                    }
                }

                VGA::terminal.write("     ");
                if (entry->attr & 0x10)
                {
                    VGA::terminal.write("<DIR>       ");
                }
                else
                {
                    VGA::terminal.write("<FILE>      ");
                }

                write_dec(entry->file_size);
                VGA::terminal.write("\n");
            }
        }
    }
    else
    {

        uint32_t cluster = root_cluster;
        while (cluster < 0x0FFFFFF8)
        {
            uint32_t sector = get_cluster_sector(cluster);
            for (uint8_t s = 0; s < sectors_per_cluster; s++)
            {
                if (!ata_read_sectors(sector + s, 1, sector_buf)) break;
                FATDirEntry* entries = (FATDirEntry*)sector_buf;

                for (int i = 0; i < 16; i++)
                {
                    FATDirEntry* entry = &entries[i];
                    if (entry->name[0] == 0x00) return;
                    if (entry->name[0] == (char)0xE5) continue;
                    if ((entry->attr & 0x0F) == 0x0F) continue;

                    for (int c = 0; c < 11; c++)
                    {
                        if (c == 8 && (entry->attr & 0x10) == 0)
                        {
                            VGA::terminal.putchar('.');
                        }
                        if (entry->name[c] != ' ')
                        {
                            VGA::terminal.putchar(entry->name[c]);
                        }
                    }

                    VGA::terminal.write("     ");
                    if (entry->attr & 0x10)
                    {
                        VGA::terminal.write("<DIR>       ");
                    }
                    else
                    {
                        VGA::terminal.write("<FILE>      ");
                    }

                    write_dec(entry->file_size);
                    VGA::terminal.write("\n");
                }
            }
            cluster = get_next_cluster(cluster);
        }
    }
}

bool fat_read_file(const char* filename, uint8_t* buffer, uint32_t max_size)
{
    if (!fat_active) return false;

    char fat_name[11];
    to_fat_name(filename, fat_name);

    uint8_t sector_buf[512];
    FATDirEntry target_entry;
    bool found = false;

    if (!is_fat32)
    {
        for (uint32_t s = 0; s < root_dir_sectors; s++)
        {
            if (!ata_read_sectors(root_dir_sector + s, 1, sector_buf)) break;
            FATDirEntry* entries = (FATDirEntry*)sector_buf;

            for (int i = 0; i < 16; i++)
            {
                if (entries[i].name[0] == 0x00) break;
                if (entries[i].name[0] == (char)0xE5) continue;

                if (fat_name_match(entries[i].name, fat_name))
                {
                    target_entry = entries[i];
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
    }
    else
    {
        uint32_t cluster = root_cluster;
        while (cluster < 0x0FFFFFF8 && !found)
        {
            uint32_t sector = get_cluster_sector(cluster);
            for (uint8_t s = 0; s < sectors_per_cluster; s++)
            {
                if (!ata_read_sectors(sector + s, 1, sector_buf)) break;
                FATDirEntry* entries = (FATDirEntry*)sector_buf;

                for (int i = 0; i < 16; i++)
                {
                    if (entries[i].name[0] == 0x00) break;
                    if (entries[i].name[0] == (char)0xE5) continue;

                    if (fat_name_match(entries[i].name, fat_name))
                    {
                        target_entry = entries[i];
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
            cluster = get_next_cluster(cluster);
        }
    }

    if (!found) return false;

    uint32_t cluster = ((uint32_t)target_entry.first_cluster_high << 16) | target_entry.first_cluster_low;
    uint32_t bytes_read = 0;
    uint32_t file_size = target_entry.file_size;

    if (max_size < file_size) file_size = max_size;

    uint8_t* out_ptr = buffer;

    while (bytes_read < file_size)
    {

        if (is_fat32)
        {
            if (cluster >= 0x0FFFFFF8 || cluster == 0) break;
        }
        else
        {
            if (cluster >= 0xFFF8 || cluster == 0) break;
        }

        uint32_t sector = get_cluster_sector(cluster);

        for (uint8_t s = 0; s < sectors_per_cluster; s++)
        {
            if (bytes_read >= file_size) break;

            if (!ata_read_sectors(sector + s, 1, sector_buf)) return false;

            uint32_t bytes_to_copy = 512;
            if (file_size - bytes_read < 512)
            {
                bytes_to_copy = file_size - bytes_read;
            }

            for (uint32_t i = 0; i < bytes_to_copy; i++)
            {
                *out_ptr++ = sector_buf[i];
            }
            bytes_read += bytes_to_copy;
        }

        cluster = get_next_cluster(cluster);
    }

    return true;
}

uint32_t fat_get_file_size(const char* filename)
{
    if (!fat_active) return 0;

    char fat_name[11];
    to_fat_name(filename, fat_name);

    uint8_t sector_buf[512];
    bool found = false;
    uint32_t file_size = 0;

    if (!is_fat32)
    {
        for (uint32_t s = 0; s < root_dir_sectors; s++)
        {
            if (!ata_read_sectors(root_dir_sector + s, 1, sector_buf)) break;
            FATDirEntry* entries = (FATDirEntry*)sector_buf;

            for (int i = 0; i < 16; i++)
            {
                if (entries[i].name[0] == 0x00) break;
                if (entries[i].name[0] == (char)0xE5) continue;

                if (fat_name_match(entries[i].name, fat_name))
                {
                    file_size = entries[i].file_size;
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
    }
    else
    {
        uint32_t cluster = root_cluster;
        while (cluster < 0x0FFFFFF8 && !found)
        {
            uint32_t sector = get_cluster_sector(cluster);
            for (uint8_t s = 0; s < sectors_per_cluster; s++)
            {
                if (!ata_read_sectors(sector + s, 1, sector_buf)) break;
                FATDirEntry* entries = (FATDirEntry*)sector_buf;

                for (int i = 0; i < 16; i++)
                {
                    if (entries[i].name[0] == 0x00) break;
                    if (entries[i].name[0] == (char)0xE5) continue;

                    if (fat_name_match(entries[i].name, fat_name))
                    {
                        file_size = entries[i].file_size;
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
            cluster = get_next_cluster(cluster);
        }
    }

    return file_size;
}
