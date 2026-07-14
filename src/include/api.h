
#ifndef API_H
#define API_H

#include "types.h"
#include "services.h"

void api_draw_rect(int x, int y, int w, int h, char c, uint8_t color);

struct VirtualFile {
    const char* path;
    const char* name;
    const char* content;
    bool is_dir;
    size_t size;
};

const char* api_read_file(const char* path);
bool api_write_file_legacy(const char* path, const char* name, const char* content);
int api_list_files(const char* dir_path, VirtualFile* out_files, int max_files);

int32_t vfs_read_file(const char* path, void* buf, uint32_t max_size);

void api_reboot();
void api_shutdown();
size_t api_get_ram_total();
size_t api_get_ram_used();
int api_get_services(SystemService* out_services, int max);
void api_toggle_service(const char* name);

#endif
