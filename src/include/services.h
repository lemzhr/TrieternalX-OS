
#ifndef SERVICES_H
#define SERVICES_H

#include "types.h"

struct SystemService {
    const char *name;
    const char *description;
    bool is_running;
};

void services_init();

int get_services(SystemService *out_services, int max_services);

void toggle_service(const char *name);

size_t get_total_memory();
size_t get_used_memory();

#endif
