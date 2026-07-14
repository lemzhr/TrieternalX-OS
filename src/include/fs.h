
#ifndef FS_H
#define FS_H

#include "types.h"

#define VFS_MAX_NODES 64
#define VFS_MAX_NAME  32
#define VFS_MAX_PATH  128

#define VFS_FLAG_DIR    0x1
#define VFS_FLAG_FILE   0x2
#define VFS_FLAG_USED   0x4

struct VFSNode {
    char name[VFS_MAX_NAME];
    char path[VFS_MAX_PATH];
    uint32_t flags;
    uint32_t size;
    uint8_t* data;
    int32_t parent_idx;
    int32_t first_child;
    int32_t next_sibling;
};

void fs_init();
int32_t vfs_open(const char* path, uint32_t flags);
int32_t vfs_close(int32_t fd);
int32_t vfs_read(int32_t fd, void* buf, uint32_t count);
int32_t vfs_write(int32_t fd, const void* buf, uint32_t count);
int32_t vfs_mkdir(const char* path);
int32_t vfs_create(const char* path);
int32_t vfs_stat(const char* path);
void vfs_list(const char* path);
int32_t vfs_node_count();
VFSNode* vfs_get_node(int32_t idx);

#endif
