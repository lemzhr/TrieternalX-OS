
#ifndef USER_API_H
#define USER_API_H

#include "types.h"

void api_print(const char* str);
char api_getkey();
void* api_malloc(size_t size);
void api_free(void* ptr);
void api_sleep(uint32_t ms);
void api_exit();

#endif
