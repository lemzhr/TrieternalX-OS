
#ifndef FAT_H
#define FAT_H

#include "types.h"

void fat_init();
bool fat_is_active();
void fat_list_root();
bool fat_read_file(const char* filename, uint8_t* buffer, uint32_t max_size);
uint32_t fat_get_file_size(const char* filename);

#endif
