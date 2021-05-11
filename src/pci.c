/*
** File:	pci.c
**
** Author:	Koen Komeya
**
** Contributor:
**
** Description:	PCI protocol. Based loosely on https://wiki.osdev.org/PCI#
**              This allows for iteration over the devices connected to the PCI bus.
**              _pci_get_devices() and _pci_devices_next() can be used to iterate over 
**              all the devices. The device that's desired can be obtained this way.
**
*/

#include "common.h"

#include "klib.h"
#include "pci.h"

/**
** _pci_init()
**
** Initialize the SATA protocol
*/
void _pci_init( void ) {
    // Does not do anything at the moment.
}

/**
** _pci_get_devices(buf)
**
** Creates a SATA device iterator given a buffer
**
** @param itr_buf  the buffer to create the iterator in
*/
void _pci_get_devices( _pci_dev_itr_t *itr_buf ){
    itr_buf->bus = 0;
    itr_buf->device = 0;
    itr_buf->func = 0;
}

/**
** _pci_config_read()
**
** Reads the PCI config word from a given register.
** 
** @param bus     the PCI bus to select
** @param slot    the slot number to select
** @param func    the function number to read
** @param offset  the register offset to read
**
** @return   read word
*/
static uint16_t _pci_config_read( uint32_t bus, uint32_t slot, uint32_t func, uint8_t offset ){
    //Calculate "address" to put in the register
    uint32_t addr = (((uint32_t)0x80000000) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xfc));
    //Write to the PCI config register
    __outl(0xCF8, addr);
    //Read the register
    return (uint16_t)((__inl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
}


/**
** _pci_config_read_32()
**
** Reads the PCI config doubleword from a given register.
** 
** @param bus     the PCI bus to select
** @param slot    the slot number to select
** @param func    the function number to read
** @param offset  the register offset to read
**
** @return        read doubleword
*/
static uint32_t _pci_config_read_32( uint32_t bus, uint32_t slot, uint32_t func, uint8_t offset ){
    //Calculate "address" to put in the register
    uint32_t addr = (((uint32_t)0x80000000) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xfc));
    //Write to the PCI config register
    __outl(0xCF8, addr);
    //Read the register
    return __inl(0xCFC);
}

/**
** _pci_load_device()
**
** Loads device function info into the buffer
** 
** @param itr     the iterator
** @param dev_buf the buffer to hold the device info
*/
static void _pci_load_device( _pci_dev_itr_t *itr, _pci_device_t *dev_buf ){
    uint16_t bus = dev_buf->bus = itr->bus;
    uint8_t device = dev_buf->device = itr->device;
    uint8_t func = dev_buf->func = itr->func;

    dev_buf->vendor_id = _pci_config_read(bus, device, func, 0);
    // Load stuff
    uint16_t offset0xA = _pci_config_read(bus, device, func, 0xA);
    dev_buf->class_code = (uint8_t) (offset0xA >> 8);
    dev_buf->subclass = (uint8_t) (offset0xA & 0xff);
    dev_buf->prog_if = (uint8_t) (_pci_config_read(bus, device, func, 0x8) >> 8);
    dev_buf->bar0 = _pci_config_read_32(bus, device, func, 0x10);
    dev_buf->bar1 = _pci_config_read_32(bus, device, func, 0x14);
    dev_buf->bar2 = _pci_config_read_32(bus, device, func, 0x18);
    dev_buf->bar3 = _pci_config_read_32(bus, device, func, 0x1C);
    dev_buf->bar4 = _pci_config_read_32(bus, device, func, 0x20);
    dev_buf->bar5 = _pci_config_read_32(bus, device, func, 0x24);
}

/**
** _pci_get_device()
**
** Checks if there is a device on bus/device and if so, enters it into dev_buf.
** 
** @param itr     the iterator
** @param dev_buf the buffer to hold the device info
**
** @return 1 if success, 0 if not a device
*/
static uint8_t _pci_get_device( _pci_dev_itr_t *itr, _pci_device_t *dev_buf ){
    uint16_t bus = itr->bus;
    uint8_t device = itr->device;
    if (itr->func == -1) {
        //Check vendor id to see if device exists
        if (_pci_config_read(bus, device, 0, 0) == 0xFFFF) return 0; 
        itr->func = 0;
        _pci_get_device(itr, dev_buf);
        return 1;
    }
    if (itr->func == 0 && ((_pci_config_read(bus, device, 0, 0xE) & 0x80) == 0)) {
        //Not multifunction
    } else {
        itr->func += 1;
        for (; itr->func < 8; itr->func += 1) {
            //Check vendor id to see if device exists.
            if (_pci_config_read(bus, device, 0, 0) != 0xFFFF) {
                _pci_load_device(itr, dev_buf);
                return 1;
            }
        }
    }
    itr->func = -1;
    return 0;
}

/**
** _pci_devices_next()
**
** Get next SATA device from iterator
** 
** @param itr     the iterator
** @param dev_buf the buffer to hold the device info
**
** @return        1 if success, 0 if no more devices
*/
uint8_t _pci_devices_next( _pci_dev_itr_t *itr, _pci_device_t *dev_buf ){
    for (;itr->bus < 256; itr->bus += 1) {
        for (;itr->device < 32; itr->device += 1) {
            if (_pci_get_device(itr, dev_buf))
                return 1;
        }
    }
    return 0;
}
