
#include "fs.h"
#include "kheap.h"
#include "vga.h"

static VFSNode vfs_nodes[VFS_MAX_NODES];
static int vfs_count = 0;

static int mystrcmp_vfs(const char* s1, const char* s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static size_t mystrlen_vfs(const char* s)
{
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static void mystrcpy_vfs(char* dest, const char* src)
{
    while (*src)
        *dest++ = *src++;
    *dest = '\0';
}

static void vfs_node_init(int idx, const char* name, const char* path, uint32_t flags, int32_t parent)
{
    mystrcpy_vfs(vfs_nodes[idx].name, name);
    mystrcpy_vfs(vfs_nodes[idx].path, path);
    vfs_nodes[idx].flags = flags | VFS_FLAG_USED;
    vfs_nodes[idx].size = 0;
    vfs_nodes[idx].data = nullptr;
    vfs_nodes[idx].parent_idx = parent;
    vfs_nodes[idx].first_child = -1;
    vfs_nodes[idx].next_sibling = -1;
}

static int32_t vfs_find_node(const char* path)
{
    for (int i = 0; i < vfs_count; i++)
    {
        if ((vfs_nodes[i].flags & VFS_FLAG_USED) && mystrcmp_vfs(vfs_nodes[i].path, path) == 0)
            return i;
    }
    return -1;
}

static int32_t vfs_find_child(int32_t parent_idx, const char* name)
{
    if (parent_idx < 0) return -1;
    int32_t child = vfs_nodes[parent_idx].first_child;
    while (child >= 0)
    {
        if (mystrcmp_vfs(vfs_nodes[child].name, name) == 0)
            return child;
        child = vfs_nodes[child].next_sibling;
    }
    return -1;
}

static int32_t vfs_alloc_node()
{
    for (int i = 0; i < VFS_MAX_NODES; i++)
    {
        if (!(vfs_nodes[i].flags & VFS_FLAG_USED))
            return i;
    }
    return -1;
}

void fs_init()
{
    for (int i = 0; i < VFS_MAX_NODES; i++)
        vfs_nodes[i].flags = 0;
    vfs_count = 0;

    vfs_node_init(0, "/", "/", VFS_FLAG_DIR, -1);
    vfs_count = 1;

    vfs_mkdir("/documents");
    vfs_mkdir("/system");
    vfs_mkdir("/games");

    vfs_create("/welcome.txt");
    {
        int32_t idx = vfs_find_node("/welcome.txt");
        if (idx >= 0)
        {
            const char* content = "Selamat datang di TrieternalX-OS!\n";
            size_t len = 0;
            { const char* s = content; while (*s++) len++; }
            vfs_nodes[idx].data = (uint8_t*)kmalloc(len + 1);
            if (vfs_nodes[idx].data)
            {
                mystrcpy_vfs((char*)vfs_nodes[idx].data, content);
                vfs_nodes[idx].size = len;
            }
        }
    }

    vfs_create("/documents/note.txt");
    {
        int32_t idx = vfs_find_node("/documents/note.txt");
        if (idx >= 0)
        {
            const char* content = "Tugas Akhir: Membangun microkernel OS.\n";
            size_t len = 0;
            { const char* s = content; while (*s++) len++; }
            vfs_nodes[idx].data = (uint8_t*)kmalloc(len + 1);
            if (vfs_nodes[idx].data)
            {
                mystrcpy_vfs((char*)vfs_nodes[idx].data, content);
                vfs_nodes[idx].size = len;
            }
        }
    }

    vfs_create("/system/kernel.sys");
    {
        int32_t idx = vfs_find_node("/system/kernel.sys");
        if (idx >= 0)
        {
            const char* content = "TrieternalX Kernel v2.5\nProtected Mode: Enabled\n";
            size_t len = 0;
            { const char* s = content; while (*s++) len++; }
            vfs_nodes[idx].data = (uint8_t*)kmalloc(len + 1);
            if (vfs_nodes[idx].data)
            {
                mystrcpy_vfs((char*)vfs_nodes[idx].data, content);
                vfs_nodes[idx].size = len;
            }
        }
    }

    vfs_create("/system/config.cfg");
    {
        int32_t idx = vfs_find_node("/system/config.cfg");
        if (idx >= 0)
        {
            const char* content = "boot_delay=3\nresolution=80x25\n";
            size_t len = 0;
            { const char* s = content; while (*s++) len++; }
            vfs_nodes[idx].data = (uint8_t*)kmalloc(len + 1);
            if (vfs_nodes[idx].data)
            {
                mystrcpy_vfs((char*)vfs_nodes[idx].data, content);
                vfs_nodes[idx].size = len;
            }
        }
    }
}

int32_t vfs_open(const char* path, uint32_t flags)
{
    (void)flags;
    int32_t idx = vfs_find_node(path);
    if (idx < 0)
        return -1;
    if (vfs_nodes[idx].flags & VFS_FLAG_DIR)
        return -2;
    return idx;
}

int32_t vfs_close(int32_t fd)
{
    (void)fd;
    return 0;
}

int32_t vfs_read(int32_t fd, void* buf, uint32_t count)
{
    if (fd < 0 || fd >= vfs_count)
        return -1;
    if (!(vfs_nodes[fd].flags & VFS_FLAG_USED))
        return -1;
    if (!vfs_nodes[fd].data)
        return 0;

    uint32_t to_read = count;
    if (to_read > vfs_nodes[fd].size)
        to_read = vfs_nodes[fd].size;

    uint8_t* dst = (uint8_t*)buf;
    for (uint32_t i = 0; i < to_read; i++)
        dst[i] = vfs_nodes[fd].data[i];

    return (int32_t)to_read;
}

int32_t vfs_write(int32_t fd, const void* buf, uint32_t count)
{
    if (fd < 0 || fd >= vfs_count)
        return -1;
    if (!(vfs_nodes[fd].flags & VFS_FLAG_USED))
        return -1;
    if (vfs_nodes[fd].flags & VFS_FLAG_DIR)
        return -1;

    uint32_t old_size = vfs_nodes[fd].size;
    uint32_t new_size = old_size + count;

    uint8_t* new_data = (uint8_t*)kmalloc(new_size + 1);
    if (!new_data)
        return -1;

    if (vfs_nodes[fd].data)
    {
        for (uint32_t i = 0; i < old_size; i++)
            new_data[i] = vfs_nodes[fd].data[i];
        kfree(vfs_nodes[fd].data);
    }

    const uint8_t* src = (const uint8_t*)buf;
    for (uint32_t i = 0; i < count; i++)
        new_data[old_size + i] = src[i];
    new_data[new_size] = '\0';

    vfs_nodes[fd].data = new_data;
    vfs_nodes[fd].size = new_size;
    return (int32_t)count;
}

int32_t vfs_mkdir(const char* path)
{
    int32_t idx = vfs_alloc_node();
    if (idx < 0)
        return -1;

    const char* name = path;
    const char* last_slash = nullptr;
    for (const char* p = path; *p; p++)
    {
        if (*p == '/')
            last_slash = p;
    }
    if (last_slash)
        name = last_slash + 1;

    int32_t parent_idx = -1;
    if (last_slash && last_slash > path)
    {
        char parent_path[VFS_MAX_PATH];
        size_t len = last_slash - path;
        if (len >= VFS_MAX_PATH) len = VFS_MAX_PATH - 1;
        for (size_t i = 0; i < len; i++)
            parent_path[i] = path[i];
        parent_path[len] = '\0';
        if (len == 0) parent_path[0] = '/';
        parent_idx = vfs_find_node(parent_path);
    }
    else
    {
        parent_idx = 0;
    }

    vfs_node_init(idx, name, path, VFS_FLAG_DIR, parent_idx);

    if (parent_idx >= 0)
    {
        vfs_nodes[idx].next_sibling = vfs_nodes[parent_idx].first_child;
        vfs_nodes[parent_idx].first_child = idx;
    }

    if (idx >= vfs_count)
        vfs_count = idx + 1;
    return idx;
}

int32_t vfs_create(const char* path)
{
    int32_t idx = vfs_alloc_node();
    if (idx < 0)
        return -1;

    const char* name = path;
    const char* last_slash = nullptr;
    for (const char* p = path; *p; p++)
    {
        if (*p == '/')
            last_slash = p;
    }
    if (last_slash)
        name = last_slash + 1;

    int32_t parent_idx = -1;
    if (last_slash && last_slash > path)
    {
        char parent_path[VFS_MAX_PATH];
        size_t len = last_slash - path;
        if (len >= VFS_MAX_PATH) len = VFS_MAX_PATH - 1;
        for (size_t i = 0; i < len; i++)
            parent_path[i] = path[i];
        parent_path[len] = '\0';
        if (len == 0) parent_path[0] = '/';
        parent_idx = vfs_find_node(parent_path);
    }
    else
    {
        parent_idx = 0;
    }

    vfs_node_init(idx, name, path, VFS_FLAG_FILE, parent_idx);

    if (parent_idx >= 0)
    {
        vfs_nodes[idx].next_sibling = vfs_nodes[parent_idx].first_child;
        vfs_nodes[parent_idx].first_child = idx;
    }

    if (idx >= vfs_count)
        vfs_count = idx + 1;
    return idx;
}

int32_t vfs_stat(const char* path)
{
    return vfs_find_node(path);
}

void vfs_list(const char* path)
{
    int32_t dir_idx = vfs_find_node(path);
    if (dir_idx < 0)
    {
        VGA::terminal.write("Directory not found: ");
        VGA::terminal.write(path);
        VGA::terminal.write("\n");
        return;
    }

    VGA::terminal.set_color(VGA::COLOR_LIGHT_CYAN, VGA::COLOR_BLACK);
    VGA::terminal.write("  TYPE  NAME\n");
    VGA::terminal.set_color(VGA::COLOR_WHITE, VGA::COLOR_BLACK);

    int32_t child = vfs_nodes[dir_idx].first_child;
    while (child >= 0)
    {
        VGA::terminal.write("  ");
        if (vfs_nodes[child].flags & VFS_FLAG_DIR)
            VGA::terminal.write("DIR   ");
        else
            VGA::terminal.write("FILE  ");

        VGA::terminal.write(vfs_nodes[child].name);

        if (vfs_nodes[child].flags & VFS_FLAG_FILE)
        {
            VGA::terminal.write("  (");
            uint32_t sz = vfs_nodes[child].size;
            if (sz == 0) VGA::terminal.putchar('0');
            else
            {
                char buf[16];
                int i = 0;
                while (sz > 0) { buf[i++] = (sz % 10) + '0'; sz /= 10; }
                for (int j = i - 1; j >= 0; j--) VGA::terminal.putchar(buf[j]);
            }
            VGA::terminal.write(" bytes)");
        }
        VGA::terminal.putchar('\n');

        child = vfs_nodes[child].next_sibling;
    }
}

int32_t vfs_node_count()
{
    return vfs_count;
}

VFSNode* vfs_get_node(int32_t idx)
{
    if (idx < 0 || idx >= VFS_MAX_NODES)
        return nullptr;
    if (!(vfs_nodes[idx].flags & VFS_FLAG_USED))
        return nullptr;
    return &vfs_nodes[idx];
}
