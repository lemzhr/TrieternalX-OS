
#ifndef ATA_H
#define ATA_H

#include "types.h"

void ata_init();
bool ata_read_sectors(uint32_t lba, uint8_t sector_count, uint8_t* buffer);
bool ata_write_sectors(uint32_t lba, uint8_t sector_count, const uint8_t* buffer);
bool ata_disk_exists();
const char* ata_get_model();

#endif
