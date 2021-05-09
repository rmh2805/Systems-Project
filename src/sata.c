/*
** File:	sata.c
**
** Author:	Koen Komeya
**
** Contributor:
**
** Description:	SATA protocol. Based loosely on https://wiki.osdev.org/PCI#
**
*/

#include "common.h"

#include "klib.h"
#include "sata.h"

/**
** _sata_init()
**
** Initialize the SATA protocol
*/
void _sata_init( void ) {
}

/**
** _sata_get_devices(buf)
**
** Get SATA device iterator
*/
void _sata_get_devices( _sata_dev_itr_t *itr_buf ){
    itr_buf->bus = 0;
    itr_buf->device = 0;
    itr_buf->func = 0;
}

/**
** _sata_pci_config_read()
**
** Reads the PCI config word
** @return read word
*/
static uint16_t _sata_pci_config_read( uint32_t bus, uint32_t slot, uint32_t func, uint8_t offset ){
    uint32_t address = (((uint32_t)0x80000000) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xfc));
    __outl(0xCF8, address);
    return (uint16_t)((__inl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
}


/**
** _sata_pci_config_read_32()
**
** Reads the PCI config doubleword
** @return read doubleword
*/
static uint32_t _sata_pci_config_read_32( uint32_t bus, uint32_t slot, uint32_t func, uint8_t offset ){
    uint32_t address = (((uint32_t)0x80000000) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xfc));
    __outl(0xCF8, address);
    return __inl(0xCFC);
}

/**
** _sata_load_func()
**
** Loads function
*/
static void _sata_load_device( _sata_dev_itr_t *itr, _sata_device_t *dev_buf ){
    uint16_t bus = dev_buf->bus = itr->bus;
    uint8_t device = dev_buf->device = itr->device;
    uint8_t func = dev_buf->func = itr->func;

    dev_buf->vendor_id = _sata_pci_config_read(bus, device, func, 0);
    //Load stuff
    uint16_t offset0xA = _sata_pci_config_read(bus, device, func, 0xA);
    dev_buf->class_code = (uint8_t) (offset0xA >> 8);
    dev_buf->subclass = (uint8_t) (offset0xA & 0xff);
    dev_buf->prog_if = (uint8_t) (_sata_pci_config_read(bus, device, func, 0x8) >> 8);
    dev_buf->bar0 = _sata_pci_config_read_32(bus, device, func, 0x10);
    dev_buf->bar1 = _sata_pci_config_read_32(bus, device, func, 0x14);
    dev_buf->bar2 = _sata_pci_config_read_32(bus, device, func, 0x18);
    dev_buf->bar3 = _sata_pci_config_read_32(bus, device, func, 0x1C);
    dev_buf->bar4 = _sata_pci_config_read_32(bus, device, func, 0x20);
    dev_buf->bar5 = _sata_pci_config_read_32(bus, device, func, 0x24);
}

/**
** _sata_get_device()
**
** Checks if there is a device on bus/device.
** @return 1 if success, 0 if not a device
*/
static uint8_t _sata_get_device( _sata_dev_itr_t *itr, _sata_device_t *dev_buf ){
    uint16_t bus = itr->bus;
    uint8_t device = itr->device;
    if (itr->func == -1) {
        //Check vendor id to see if device exists
        if (_sata_pci_config_read(bus, device, 0, 0) == 0xFFFF) return 0; 
        itr->func = 0;
        _sata_get_device(itr, dev_buf);
        return 1;
    }
    if (itr->func == 0 && ((_sata_pci_config_read(bus, device, 0, 0xE) & 0x80) == 0)) {
        //Not multifunction
    } else {
        itr->func += 1;
        for (; itr->func < 8; itr->func += 1) {
            //Check vendor id to see if device exists.
            if (_sata_pci_config_read(bus, device, 0, 0) != 0xFFFF) {
                _sata_load_device(itr, dev_buf);
                return 1;
            }
        }
    }
    itr->func = -1;
    return 0;
}

/**
** _sata_devices_next()
**
** Get next SATA device from iterator
** @return 1 if success, 0 if no more devices
*/
uint8_t _sata_devices_next( _sata_dev_itr_t *itr, _sata_device_t *dev_buf ){
    for (;itr->bus < 256; itr->bus += 1) {
        for (;itr->device < 32; itr->device += 1) {
            if (_sata_get_device(itr, dev_buf))
                return 1;
        }
    }
    return 0;
}
