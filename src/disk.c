/*
** File:	disk.c
**
** Author:	Koen Komeya
**
** Contributor:
**
** Description:	Disk protocol.
**              Inspired by https://wiki.osdev.org/AHCI
**
*/

#include "common.h"

#include "klib.h"
#include "sata.h"
#include "disk.h"
#include "kfs.h"
#include "clock.h"
#include "driverInterface.h"

_sata_device_t _disk_device;//s[8];
//uint8_t _disk_devices_count;

int _disk_read( uint32_t blockNr, char* buf, uint8_t devId );
int _disk_write( uint32_t blockNr, char* buf, uint8_t devId );

//
#define ATA_SR_BSY     0x80    
#define ATA_SR_DRDY    0x40    
#define ATA_SR_DF      0x20    
#define ATA_SR_DSC     0x10    
#define ATA_SR_DRQ     0x08    
#define ATA_SR_CORR    0x04    
#define ATA_SR_IDX     0x02    
#define ATA_SR_ERR     0x01    
#define ATA_ER_BBK      0x80    
#define ATA_ER_UNC      0x40    
#define ATA_ER_MC       0x20    
#define ATA_ER_IDNF     0x10    
#define ATA_ER_MCR      0x08    
#define ATA_ER_ABRT     0x04    
#define ATA_ER_TK0NF    0x02    
#define ATA_ER_AMNF     0x01    
#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC
#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200
#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01
#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_READ      0x00
#define ATA_WRITE     0x01

struct _ide_channel_regs {
    uint16_t base;
    uint16_t ctrl;
    uint16_t bmide;
    uint8_t nien;
} _ide_channels[2];

unsigned char _ide_buf[2048] = {0};

struct _ide_device {
    uint8_t channel;
    uint8_t drive; 
    uint16_t capabilities;
    uint32_t command_sets; 
    uint32_t size;
} _ide_devices[4];

int _ide_device_count = 0;

static void _sleep1ms(void) {
    __delay(1);
}

static void _ide_write(unsigned char channel, unsigned char reg, unsigned char data) {
   
    if (reg > 0x07 && reg < 0x0C)
        _ide_write(channel, ATA_REG_CONTROL, 0x80 | _ide_channels[channel].nien);
    if (reg < 0x08)
        __outb(_ide_channels[channel].base + reg, data);
    else if (reg < 0x0C)
        __outb(_ide_channels[channel].base + reg - 0x06, data);
    else if (reg < 0x0E)
        __outb(_ide_channels[channel].ctrl + reg - 0x0A, data);
    else if (reg < 0x16)
        __outb(_ide_channels[channel].bmide + reg - 0x0E, data);
    if (reg > 0x07 && reg < 0x0C)
        _ide_write(channel, ATA_REG_CONTROL, _ide_channels[channel].nien);
}

static unsigned char _ide_read(unsigned char channel, unsigned char reg) {
    uint8_t result;
    if (reg > 0x07 && reg < 0x0C)
        _ide_write(channel, ATA_REG_CONTROL, 0x80 | _ide_channels[channel].nien);
    if (reg < 0x08)
        result = __inb(_ide_channels[channel].base + reg);
    else if (reg < 0x0C)
        result = __inb(_ide_channels[channel].base + reg - 0x06);
    else if (reg < 0x0E)
        result = __inb(_ide_channels[channel].ctrl + reg - 0x0A);
    else if (reg < 0x16)
        result = __inb(_ide_channels[channel].bmide + reg - 0x0E);
    if (reg > 0x07 && reg < 0x0C)
        _ide_write(channel, ATA_REG_CONTROL, _ide_channels[channel].nien);
    return result;
}

static void _ide_read_buffer(unsigned char channel, unsigned char reg, uint32_t *buffer,
                     unsigned int quads) {
    if (reg > 0x07 && reg < 0x0C)
        _ide_write(channel, ATA_REG_CONTROL, 0x80 | _ide_channels[channel].nien);
    if (reg < 0x08)
        for (int i = 0; i < quads; i++)
            buffer[i] = __inl(_ide_channels[channel].base + reg);
    else if (reg < 0x0C)
        for (int i = 0; i < quads; i++)
            buffer[i] = __inl(_ide_channels[channel].base + reg - 0x06);
    else if (reg < 0x0E)
        for (int i = 0; i < quads; i++)
            buffer[i] = __inl(_ide_channels[channel].ctrl + reg - 0x0A);
    else if (reg < 0x16)
        for (int i = 0; i < quads; i++)
            buffer[i] = __inl(_ide_channels[channel].bmide + reg - 0x0E);
    if (reg > 0x07 && reg < 0x0C)
        _ide_write(channel, ATA_REG_CONTROL, _ide_channels[channel].nien);
}

static unsigned char _ide_poll_wait(unsigned char channel, unsigned int advanced_check) {
 
   // Wait for BSY to be set
   _sleep1ms();
 
   // Wait for BSY to be cleared
   while (_ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY);
 
   if (advanced_check) {
      uint8_t state = _ide_read(channel, ATA_REG_STATUS); 

      if (state & ATA_SR_ERR)
         return 2;

      if (state & ATA_SR_DF)
         return 1; 

      if ((state & ATA_SR_DRQ) == 0)
         return 3; 
 
   }
 
   return 0; 
 
}

void _ide_initialize(void) {
 
    // Get I/O Ports
    _ide_channels[0].base = (_disk_device.bar0 & 0xFFFFFFFC);
    if (!_ide_channels[0].base) _ide_channels[0].base = 0x1F0;
    _ide_channels[0].ctrl = (_disk_device.bar1 & 0xFFFFFFFC);
    if (!_ide_channels[0].ctrl) _ide_channels[0].ctrl = 0x3F6;
    _ide_channels[1].base = (_disk_device.bar2 & 0xFFFFFFFC);
    if (!_ide_channels[1].base) _ide_channels[1].base = 0x170;
    _ide_channels[1].ctrl = (_disk_device.bar3 & 0xFFFFFFFC);
    if (!_ide_channels[1].ctrl) _ide_channels[1].ctrl = 0x376;
    _ide_channels[0].bmide = (_disk_device.bar4 & 0xFFFFFFFC) + 0;
    _ide_channels[1].bmide = (_disk_device.bar4 & 0xFFFFFFFC) + 8;

    // Disable IRQs
    _ide_write(0, ATA_REG_CONTROL, 2);
    _ide_write(1, ATA_REG_CONTROL, 2);

    for (int ch = 0; ch < 2; ch++) {
        for (int dr = 0; dr < 2; dr++) {
            // Select drive
            _ide_write(ch, ATA_REG_HDDEVSEL, 0xA0 | (dr << 4)); 
            _sleep1ms(); 
 
            // Identify command
            _ide_write(ch, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            _sleep1ms();
 
            // Ensure this is ATA
            if (_ide_read(ch, ATA_REG_STATUS) == 0)
                continue;
            int fail = 0;
            unsigned char status;
            do {
                status = _ide_read(ch, ATA_REG_STATUS);
                if ((status & ATA_SR_ERR)) {fail = 1; break;} 
            } while ((status & ATA_SR_BSY) || !(status & ATA_SR_DRQ));
            if (fail)
                continue;
 
            // Read identification
            _ide_read_buffer(ch, ATA_REG_DATA, (uint32_t *) _ide_buf, 128);
            _ide_devices[_ide_device_count].channel      = ch;
            _ide_devices[_ide_device_count].drive        = dr;
            _ide_devices[_ide_device_count].capabilities = *((unsigned short *)(_ide_buf + ATA_IDENT_CAPABILITIES));
            _ide_devices[_ide_device_count].command_sets  = *((unsigned int *)(_ide_buf + ATA_IDENT_COMMANDSETS));
 
            // Get size
            if (_ide_devices[_ide_device_count].command_sets & (1 << 26))
                // 48-bit
                _ide_devices[_ide_device_count].size   = *((unsigned int *)(_ide_buf + ATA_IDENT_MAX_LBA_EXT));
            else
                // CHS or 28-bit
                _ide_devices[_ide_device_count].size   = *((unsigned int *)(_ide_buf + ATA_IDENT_MAX_LBA));

            driverInterface_t dri = { 0, _ide_device_count, _disk_read, _disk_write };
            if (_fs_registerDev(dri) < 0) _kpanic("disk", "Errored on adding disk");
 
            _ide_device_count++;
        }
    }
}

unsigned char _ide_ata_io(unsigned char direction, unsigned char drive, unsigned int lba, 
                          unsigned char numsects, char *buf) {

    static const uint8_t flush[3] = {ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH_EXT};
    uint8_t lba_mode;
    uint8_t cmd;
    uint8_t lba_io[6];
    uint32_t channel = _ide_devices[drive].channel; 
    uint32_t slavebit = _ide_devices[drive].drive; 
    uint32_t bus = _ide_channels[channel].base; 
    uint32_t words = 256; 
    uint16_t cyl;
    uint8_t head;
    uint8_t sect;

    _ide_write(channel, ATA_REG_CONTROL, _ide_channels[channel].nien = 0x02);

    if (lba >= 0x10000000) { 
        // LBA48
        lba_mode = 2;
        lba_io[0] = (lba & 0x000000FF) >> 0;
        lba_io[1] = (lba & 0x0000FF00) >> 8;
        lba_io[2] = (lba & 0x00FF0000) >> 16;
        lba_io[3] = (lba & 0xFF000000) >> 24;
        lba_io[4] = 0; 
        lba_io[5] = 0; 
        head = 0; 
    } else if (_ide_devices[drive].capabilities & 0x200)  { 
        // LBA28
        lba_mode = 1;
        lba_io[0] = (lba & 0x00000FF) >> 0;
        lba_io[1] = (lba & 0x000FF00) >> 8;
        lba_io[2] = (lba & 0x0FF0000) >> 16;
        head = (lba & 0xF000000) >> 24;
    } else {
        // CHS
        lba_mode = 0;
        sect = (lba % 63) + 1;
        cyl = (lba + 1 - sect) / (16 * 63);
        lba_io[0] = sect;
        lba_io[1] = (cyl >> 0) & 0xFF;
        lba_io[2] = (cyl >> 8) & 0xFF;
        head = (lba + 1 - sect) % (16 * 63) / (63); 
    }

    // Wait for drive to be free
    while (_ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY);
    // Get drive with mode
    if (lba_mode == 0) // CHS
        _ide_write(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head); 
    else
        _ide_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head); 
    if (lba_mode == 2) {
        _ide_write(channel, ATA_REG_SECCOUNT1, 0);
        _ide_write(channel, ATA_REG_LBA3, lba_io[3]);
        _ide_write(channel, ATA_REG_LBA4, lba_io[4]);
        _ide_write(channel, ATA_REG_LBA5, lba_io[5]);
    }
    _ide_write(channel, ATA_REG_SECCOUNT0, numsects);
    _ide_write(channel, ATA_REG_LBA0, lba_io[0]);
    _ide_write(channel, ATA_REG_LBA1, lba_io[1]);
    _ide_write(channel, ATA_REG_LBA2, lba_io[2]); 

    // Select mode
    if (lba_mode == 0 && direction == 0) cmd = ATA_CMD_READ_PIO;
    if (lba_mode == 1 && direction == 0) cmd = ATA_CMD_READ_PIO;   
    if (lba_mode == 2 && direction == 0) cmd = ATA_CMD_READ_PIO_EXT;
    if (lba_mode == 0 && direction == 1) cmd = ATA_CMD_WRITE_PIO;
    if (lba_mode == 1 && direction == 1) cmd = ATA_CMD_WRITE_PIO;
    if (lba_mode == 2 && direction == 1) cmd = ATA_CMD_WRITE_PIO_EXT;

    _ide_write(channel, ATA_REG_COMMAND, cmd); 

    if (direction == 0) {
        // PIO Read.
        for (int i = 0; i < numsects; i++) {
            unsigned char err = _ide_poll_wait(channel, 1);
            if (err)
                return -err; 
            for (int j = 0; j < words; j++)
                ((uint16_t *)buf)[j] = __inw(bus);
            buf += (words*2);
        }
    } else {
        // PIO Write.
        for (int i = 0; i < numsects; i++) {
            unsigned char err = _ide_poll_wait(channel, 1);
            if (err)
                return -err; 
            for (int j = 0; j < words; j++)
                __outw(bus, ((uint16_t *)buf)[j]);
            buf += (words*2);
        }
        _ide_write(channel, ATA_REG_COMMAND, flush[lba_mode]);
        _ide_poll_wait(channel, 0);
    }
    return 0;
}

/**
** _disk_init()
**
** Initialize the disk protocol
*/
void _disk_init( void ) {
    //_disk_devices_count = 0;
    _sata_dev_itr_t itr;
    _sata_device_t dev;
    for (_sata_get_devices(&itr); _sata_devices_next(&itr, &dev);) { //&& _disk_devices_count < 8;) {
        if (dev.class_code == 0x01 && dev.subclass == 0x06) {
            _disk_device = dev;
            //driverInterface_t dri = { 0, _disk_devices_count, _disk_read, _disk_write };
            //_disk_devices_count++;
            //if (_fs_registerDev(dri) < 0) _kpanic("disk", "Errored on adding disk");
            _ide_initialize();
            return;
        }
    }
}

//512 bytes

int _disk_read( uint32_t blockNr, char* buf, uint8_t devId ) {
    
    if (devId >= _ide_device_count) 
        return -1;
 
    if (blockNr + 1 > _ide_devices[devId].size)
        return -1;

    return _ide_ata_io(ATA_READ, devId, blockNr, 1, buf);
}

int _disk_write( uint32_t blockNr, char* buf, uint8_t devId ) {
    if (devId >= _ide_device_count) 
        return -1;
 
    if (blockNr + 1 > _ide_devices[devId].size)
        return -1;

    return _ide_ata_io(ATA_WRITE, devId, blockNr, 1, buf);
}

