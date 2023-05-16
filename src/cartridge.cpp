#include "cartridge.h"

cartridge cart;

bool cartridge_load(char* path){
    FILE *fp = fopen(path, "r");

    if (!fp) {
        printf("Failed to open: %s\n", path);
        return false;
    }

    //printf("Opened: %s\n", ctx.filename);

    fseek(fp, 0, SEEK_END);
    cart.rom_size = ftell(fp);

    rewind(fp);

    cart.data = (u8*)malloc(cart.rom_size);
    fread(cart.data, cart.rom_size, 1, fp);
    fclose(fp);

    cart.header = (cartridge_header*)(cart.data + 0x100);
    cart.header->title[15] = 0;

    printf("Cartridge Loaded:\n");
    printf("\t Title    : %s\n", cart.header->title);
    printf("\t Type     : %2.2X\n", cart.header->type);
    printf("\t ROM Size : %d KB\n", 32 << cart.header->rom_size);
    printf("\t RAM Size : %2.2X\n", cart.header->ram_size);
    printf("\t LIC Code : %2.2X\n", cart.header->lic_code);
    printf("\t ROM Vers : %2.2X\n", cart.header->version);

    u16 x = 0;
    for (u16 i=0x0134; i<=0x014C; i++) {
        x = x - cart.data[i] - 1;
    }

    printf("\t Checksum : %2.2X (%s)\n", cart.header->checksum, (x & 0xFF) ? "PASSED" : "FAILED");

    return true;
}

u8 cartridge_read(u16 address){
    //printf("$(%4.4X) :  %2.2X\n", address,cart.data[address]);
    return cart.data[address];
}

void cartridge_write(u16 address, u8 value){
    cart.data[address] = value;
}
