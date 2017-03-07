/*
 * USBRamdisk.h
 * Copyright (C) 2017 Benoit Rapidel <benoit.rapidel+devs@exmachina.fr>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef USBRAMDISK_H
#define USBRAMDISK_H


#include "mbed.h"
#include "USBMSD.h"

extern "C" {
#include "ramdisk.h"
}

class USBRamdisk : public USBMSD {

    public:
        USBRamdisk();
        virtual int disk_initialize();
        virtual int disk_write(const uint8_t *data, uint64_t block, uint8_t count);
        virtual int disk_read(uint8_t *data, uint64_t block, uint8_t count);
        virtual int disk_status();
        virtual int disk_sync();
        virtual uint64_t disk_sectors();
        virtual uint64_t disk_size();

    protected:
        int _status;
        char disk_image[RD_NB_SECTORS];

};

#endif /* !USBRAMDISK_H */
