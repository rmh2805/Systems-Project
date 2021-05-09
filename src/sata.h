/**
** @file sata.h
**
** @author Koen Komeya
**
** SATA protocol
*/

#ifndef SATA_H_
#define SATA_H_

struct _sata_dev_itr {
    uint16_t bus;
    uint8_t device;
    uint8_t func;
}

typedef struct _sata_dev_itr _sata_dev_itr_t;

struct _sata_device {
    uint16_t bus;
    uint8_t device;
    uint8_t func;
    uint16_t vendor_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint32_t bar0;
    uint32_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;
}

typedef struct _sata_device _sata_device_t;

/**
** _sata_init()
**
** Initialize the SATA protocol
*/
void _sata_init( void );

/**
** _sata_get_devices()
**
** Get SATA device iterator.
*/
uint8_t _sata_get_devices( _sata_dev_itr_t *itr_buf );

/**
** _sata_devices_next()
**
** Get next SATA device from iterator
** @return 1 if success, 0 if no more devices
*/
void _sata_devices_next( _sata_dev_itr_t *itr. _sata_device_t *dev_buf );

#endif

