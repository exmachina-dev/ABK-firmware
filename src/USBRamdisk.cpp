/*
 * USBRamdisk.cpp
 * Copyright (C) 2017 Benoit Rapidel <benoit.rapidel+devs@exmachina.fr>
 *
 * Distributed under terms of the MIT license.
 */

#include "USBRamdisk.h"
#include "ramdisk.h"

USBRamdisk::USBRamdisk()  {
    //no init
    _status = 0x01;
}

int USBRamdisk::disk_initialize() {
    _status = 0x00;
    return RD_disk_initialize(&_status, disk_image);
}

int USBRamdisk::disk_write(const uint8_t * buffer, uint64_t block, uint8_t count) {
    return RD_disk_write(disk_image, buffer, block, count);
}

int USBRamdisk::disk_read(uint8_t * buffer, uint64_t block, uint8_t count) {
    return RD_disk_read(disk_image, buffer, block, count);
}

int USBRamdisk::disk_status() { return _status; }
int USBRamdisk::disk_sync() { return RD_disk_sync(); }
uint64_t USBRamdisk::disk_sectors() { return RD_disk_sectors(); }
uint64_t USBRamdisk::disk_size() { return RD_disk_blocksize();}
