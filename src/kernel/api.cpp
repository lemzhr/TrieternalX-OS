
#include "api.h"
#include "vga.h"
#include "io.h"
#include "fs.h"

void api_draw_rect(int x, int y, int w, int h, char c, uint8_t color)
{
    uint16_t *vga_buffer = (uint16_t *)0xB8000;
    for (int r = y; r < y + h; r++)
    {
        if (r < 0 || r >= 25) continue;
        for (int col = x; col < x + w; col++)
        {
            if (col < 0 || col >= 80) continue;
            vga_buffer[r * 80 + col] = c | (color << 8);
        }
    }
}

const char* api_read_file(const char* path)
{
    int32_t idx = vfs_stat(path);
    if (idx < 0) return nullptr;
    VFSNode* node = vfs_get_node(idx);
    if (!node || (node->flags & VFS_FLAG_DIR)) return nullptr;
    return (const char*)node->data;
}

bool api_write_file_legacy(const char* path, const char* name, const char* content)
{
    (void)name;
    int32_t fd = vfs_create(path);
    if (fd < 0)
        fd = vfs_open(path, 2);
    if (fd < 0) return false;

    uint32_t len = 0;
    { const char* s = content; while (*s++) len++; }
    vfs_write(fd, content, len);
    vfs_close(fd);
    return true;
}

int api_list_files(const char* dir_path, VirtualFile* out_files, int max_files)
{
    int32_t dir_idx = vfs_stat(dir_path);
    if (dir_idx < 0) return 0;

    VFSNode* dir_node = vfs_get_node(dir_idx);
    if (!dir_node) return 0;

    int count = 0;
    int32_t child = dir_node->first_child;
    while (child >= 0 && count < max_files)
    {
        VFSNode* node = vfs_get_node(child);
        if (node)
        {
            out_files[count].path = node->path;
            out_files[count].name = node->name;
            out_files[count].content = node->data ? (const char*)node->data : "";
            out_files[count].is_dir = (node->flags & VFS_FLAG_DIR) != 0;
            out_files[count].size = node->size;
            count++;
        }
        child = vfs_get_node(child) ? vfs_get_node(child)->next_sibling : -1;
    }
    return count;
}

int32_t vfs_read_file(const char* path, void* buf, uint32_t max_size)
{
    int32_t fd = vfs_open(path, 0);
    if (fd < 0) return -1;
    int32_t bytes = vfs_read(fd, buf, max_size);
    vfs_close(fd);
    return bytes;
}

void api_reboot()
{
    outb(0x64, 0xFE);
}

void api_shutdown()
{
    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);
    outw(0x4004, 0x3400);
}

size_t api_get_ram_total()
{
    return get_total_memory();
}

size_t api_get_ram_used()
{
    return get_used_memory();
}

int api_get_services(SystemService *out_services, int max)
{
    return get_services(out_services, max);
}

void api_toggle_service(const char *name)
{
    toggle_service(name);
}
