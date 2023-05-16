#include "common.h"

typedef struct {
    u8 entry[4];
    u8 logo[0x30];

    char title[16];
    u16 new_lic_code;
    u8 sgb_flag;
    u8 type;
    u8 rom_size;
    u8 ram_size;
    u8 dest_code;
    u8 lic_code;
    u8 version;
    u8 checksum;
    u16 global_checksum;
} cartridge_header;


typedef struct {
    char* title;
    u8* data;
    long rom_size;
    cartridge_header *header;
} cartridge;


bool cartridge_load(char* path);

u8 cartridge_read(u16 address);

void cartridge_write(u16 address, u8 value);

