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
static void _sata_load_device( _sata_dev_itr_t *itr. _sata_device_t *dev_buf ){
    uint16_t bus = dev_buf->bus = itr->bus;
    uint8_t device = dev_buf->device = itr->device;
    uint8_t func = dev_buf->func = itr->func;

    dev_buf->vendorID = _sata_pci_config_read(bus, device, func, 0);
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
static uint8_t _sata_get_device( _sata_dev_itr_t *itr. _sata_device_t *dev_buf ){
    uint16_t bus = itr->bus;
    uint8_t device = itr->device;
    if (itr->func == -1) {
        //Check vendor id to see if device exists
        if (_sata_pci_config_read(bus, device, 0, 0) == 0xFFFF) return 0; 
        itr->func = 0;
        _sata_get_device(itr, dev_buf);
        return 1;
    }
    if (itr->func == 0 && (_sata_pci_config_read(bus, device, 0, 0xE) & 0x80 == 0)) {
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
uint8_t _sata_devices_next( _sata_dev_itr_t *itr. _sata_device_t *dev_buf ){
    for (;itr->bus < 256; itr->bus += 1) {
        for (;itr->device < 32; itr->device += 1) {
            if (_sata_get_device(itr, dev_buf))
                return 1;
        }
    }
    return found_device;
}

/**
** _sata_get
**
** Interrupt handler for the Disk module.  Handles all pending
** events.
**
** @param vector   The interrupt vector number for this interrupt
** @param ecode    The error code associated with this interrupt
*/


/**
** _sata_isr(vector,ecode)
**
** Interrupt handler for the SATA module.  Handles all pending
** events.
**
** @param vector   The interrupt vector number for this interrupt
** @param ecode    The error code associated with this interrupt
*/
static void _sata_isr( int vector, int ecode ) {
    pcb_t *pcb;
    int eir, lsr, msr;
    int ch;

    //
    // Must process all pending events; loop until the EIR
    // says there's nothing else to do.
    //

    for(;;) {

        // get the "pending event" indicator
        eir = __inb( UA4_EIR ) & UA4_EIR_INT_PRI_MASK;

        // process this event
        switch( eir ) {

        case UA4_EIR_LINE_STATUS_INT_PENDING:
            // shouldn't happen, but just in case....
            lsr = __inb( UA4_LSR );
            __cio_printf( "** SIO line status, LSR = %02x\n", lsr );
            break;

        case UA4_EIR_RX_INT_PENDING:
            // get the character
            ch = __inb( UA4_RXD );
            if( ch == '\r' ) {    // map CR to LF
                ch = '\n';
            }

            //
            // If there is a waiting process, this must be
            // the first input character; give it to that
            // process and awaken the process.
            //

            if( QLENGTH(READQ) > 0 ) {

        pcb = (pcb_t *) QDEQUE( READQ );
        assert( pcb );

                // return char via arg #2 and count in EAX
        char *buf = (char *) ARG(pcb,2);
        *buf = ch & 0xff;
                RET(pcb) = 1;
                SCHED( pcb );

            } else {

                //
                // Nobody waiting - add to the input buffer
                // if there is room, otherwise just ignore it.
                //

                if( _incount < BUF_SIZE ) {
                    *_inlast++ = ch;
                    ++_incount;
                }

            }
            break;

        case UA5_EIR_RX_FIFO_TIMEOUT_INT_PENDING:
            // shouldn't happen, but just in case....
            ch = __inb( UA4_RXD );
            __cio_printf( "** SIO FIFO timeout, RXD = %02x\n", ch );
            break;

        case UA4_EIR_TX_INT_PENDING:
            // if there is another character, send it
            if( _sending && _outcount > 0 ) {
                __outb( UA4_TXD, *_outnext );
                ++_outnext;
                // wrap around if necessary
                if( _outnext >= (_outbuffer + BUF_SIZE) ) {
                    _outnext = _outbuffer;
                }
                --_outcount;
            } else {
                // no more data - reset the output vars
                _outcount = 0;
                _outlast = _outnext = _outbuffer;
                _sending = 0;
                // disable TX interrupts
                _sio_disable( SIO_TX );
            }
            break;

        case UA4_EIR_NO_INT:
            // nothing to do - tell the PIC we're done
            __outb( PIC_PRI_CMD_PORT, PIC_EOI );
            return;

        case UA4_EIR_MODEM_STATUS_INT_PENDING:
            // shouldn't happen, but just in case....
            msr = __inb( UA4_MSR );
            __cio_printf( "** SIO modem status, MSR = %02x\n", msr );
            break;

        default:
            // uh-oh....
            __cio_printf( "sio isr: eir %02x\n", ((uint32_t) eir) & 0xff );
            _kpanic( "_sio_isr", "unknown device status" );

        }
    
    }

    // should never reach this point!
    assert( false );
}
