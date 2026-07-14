
#ifndef PCI_H
#define PCI_H

#include "types.h"

struct PCIDevice {
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint32_t bar[6];
};

void pci_init();
int pci_get_device_list(PCIDevice* list, int max_devs);
void pci_print_devices();
const char* pci_class_to_string(uint8_t class_code);

#endif
