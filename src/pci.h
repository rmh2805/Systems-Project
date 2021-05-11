/**
** @file pci.h
**
** @author Koen Komeya
**
** PCI protocol. See pci.c for explanation of this module.
*/

#ifndef PCI_H_
#define PCI_H_

// The struct for _pci_dev_itr_t
struct _pci_dev_itr {
    uint16_t bus;
    uint8_t device;
    uint8_t func;
};

// Specifies an iterator to iterate over PCI devices.
typedef struct _pci_dev_itr _pci_dev_itr_t;

// The struct for _pci_device_t
struct _pci_device {
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
};

// Specifies a device that can be accessed by PCI.
typedef struct _pci_device _pci_device_t;

/**
** _pci_init()
**
** Initialize the SATA protocol
*/
void _pci_init( void );

/**
** _pci_get_devices(buf)
**
** Creates a SATA device iterator given a buffer
**
** @param itr_buf  the buffer to create the iterator in
*/
void _pci_get_devices( _pci_dev_itr_t *itr_buf );

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
uint8_t _pci_devices_next( _pci_dev_itr_t *itr, _pci_device_t *dev_buf );

#endif

