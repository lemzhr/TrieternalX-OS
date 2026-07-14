
#include "services.h"
#include "pmm.h"

#define MAX_SERVICES 8

static SystemService services_list[MAX_SERVICES];
static int services_count = 0;

static size_t simulated_used_mem = 24110592;

static int mystrcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

void services_init()
{
    services_count = 0;

    services_list[0] = {"VGA Console Driver", "Menangani visual keluaran teks 80x25", true};
    services_list[1] = {"PS/2 Keyboard Driver", "Mengatur input papan ketik & pemetaan tombol", true};
    services_list[2] = {"Virtual Filesystem", "Mengatur berkas virtual RAM-based", true};
    services_list[3] = {"TUI Desktop GUI Engine", "Menggambar jendela dan mengatur fokus UI", true};
    services_list[4] = {"Process Scheduler", "Mengatur eksekusi multitasking proses", true};
    services_list[5] = {"ACPI Power Manager", "Menangani reboot dan shutdown sistem", true};

    services_count = 6;
}

int get_services(SystemService *out_services, int max_services)
{
    int count = 0;
    for (int i = 0; i < services_count; i++)
    {
        if (count < max_services)
        {
            out_services[count] = services_list[i];
            count++;
        }
    }
    return count;
}

void toggle_service(const char *name)
{
    for (int i = 0; i < services_count; i++)
    {
        if (mystrcmp(services_list[i].name, name) == 0)
        {
            services_list[i].is_running = !services_list[i].is_running;

            if (services_list[i].is_running)
            {
                simulated_used_mem += 524288;
            }
            else
            {
                simulated_used_mem -= 524288;
            }
            return;
        }
    }
}

size_t get_total_memory()
{
    return 134217728;
}

size_t get_used_memory()
{
    return pmm_get_used_frames() * 4096;
}
