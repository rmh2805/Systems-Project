/*
** File:	disk.c
**
** Author:	Koen Komeya
**
** Contributor:
**
** Description:	Disk protocol.
**
*/

#include "common.h"

#include "klib.h"
#include "sata.h"
#include "disk.h"

_sata_device_t _disk_devices[8];
uint8_t _disk_devices_count;

/**
** _disk_init()
**
** Initialize the disk protocol
*/
void _disk_init( void ) {
    _disk_devices_count = 0;
    _sata_dev_itr_t itr;
    _sata_device_t dev;
    for (_sata_get_devices(&itr); _sata_devices_next(&itr, &dev) && _disk_devices_count < 8;) {
        if (dev.class_code == 0x01 && dev.subclass == 0x06) {
            _disk_devices[_disk_devices_count] = dev;
            _disk_devices_count++;
        }
    }
}


