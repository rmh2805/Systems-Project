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
#include "cio.h"
#include "support.h"

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
#define ATA_CMD_IDENTIFY          0xEC
#define ATA_IDENT_MODEL        (27 * 2)
#define ATA_IDENT_CAPABILITIES (49 * 2)
#define ATA_IDENT_MAX_LBA      (60 * 2)
#define ATA_IDENT_COMMANDSETS  (82 * 2)
#define ATA_IDENT_MAX_LBA_EXT  (100 * 2)
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
#define ATA_REG_CONTROL    0x0C
#define ATA_READ      0x00
#define ATA_WRITE     0x01

struct _ide_channel_regs {
    uint16_t base;
    uint16_t ctrl;
    uint8_t nien;
} _ide_channels[2];

unsigned static char _ide_buf[1024] = {0};

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


static char _hex[] = "0123456789ABCDEF";
/**
** Name:	__cvthex2
**
** Description:	Convert an byte into a hex character string.
**
** @param buf    Result buffer
** @param value  Value to be converted
*/
void __cvthex2( char *buf, int value ){
    buf[0] = _hex[(value >> 4) & 0xf];
    buf[1] = _hex[value & 0xf];
    buf[2] = 0;
}
/**
** Name:	__cvthex4
**
** Description:	Convert an short into a hex character string.
**
** @param buf    Result buffer
** @param value  Value to be converted
*/
void __cvthex4( char *buf, int value ){
    buf[0] = _hex[(value >> 12) & 0xf];
    buf[1] = _hex[(value >> 8) & 0xf];
    buf[2] = _hex[(value >> 4) & 0xf];
    buf[3] = _hex[value & 0xf];
    buf[4] = 0;
}
/**
** Name:	__cvthex8
**
** Description:	Convert an int into a hex character string.
**
** @param buf    Result buffer
** @param value  Value to be converted
*/
void __cvthex8( char *buf, int value ){
    buf[0] = _hex[(value >> 28) & 0xf];
    buf[1] = _hex[(value >> 24) & 0xf];
    buf[2] = _hex[(value >> 20) & 0xf];
    buf[3] = _hex[(value >> 16) & 0xf];
    buf[4] = _hex[(value >> 12) & 0xf];
    buf[5] = _hex[(value >> 8) & 0xf];
    buf[6] = _hex[(value >> 4) & 0xf];
    buf[7] = _hex[value & 0xf];
    buf[8] = 0;
}

static void _ide_write(unsigned char chan, unsigned char reg, unsigned char data) {
    if (reg < 0x08)
        __outb(_ide_channels[chan].base + reg, data);
    else
        __outb(_ide_channels[chan].ctrl + reg - 0x0A, data);
}

static unsigned char _ide_read(unsigned char chan, unsigned char reg) {
    uint8_t result;
    if (reg < 0x08)
        result = __inb(_ide_channels[chan].base + reg);
    else
        result = __inb(_ide_channels[chan].ctrl + reg - 0x0A);
    return result;
}

static void _ide_read_buffer(unsigned char chan, unsigned char reg, uint32_t *buffer,
                     unsigned int dwords) {
    if (reg < 0x08) {
        //char dbg[16];
        for (int i = 0; i < dwords; i++) {
            buffer[i] = __inl(_ide_channels[chan].base + reg);
            //uint8_t *test = &((uint8_t*)buffer)[i*4];
            //if (test[1] >= 0x20 && test[1] < 0x7F) __cio_putchar(test[1]);
            //if (test[0] >= 0x20 && test[0] < 0x7F) __cio_putchar(test[0]);
            //if (test[3] >= 0x20 && test[3] < 0x7F) __cio_putchar(test[3]);
            //if (test[2] >= 0x20 && test[2] < 0x7F) __cio_putchar(test[2]);
            //__cvthex8(dbg, buffer[i]);
            //__cio_putchar(' ');
            //__cio_puts(dbg);
        }
        //__cio_putchar('\n');
    }
    else
        for (int i = 0; i < dwords; i++)
            buffer[i] = __inl(_ide_channels[chan].ctrl + reg - 0x0A);
}

static unsigned char _ide_poll_wait(unsigned char chan, unsigned int advanced_check) {
 
   // Wait for BSY to be set
   _sleep1ms();
 
   // Wait for BSY to be cleared
   while (_ide_read(chan, ATA_REG_STATUS) & ATA_SR_BSY);

   // Wait for RDY to be set
   while (!(_ide_read(chan, ATA_REG_STATUS) & ATA_SR_DRDY));
 
   if (advanced_check) {
      uint8_t state = _ide_read(chan, ATA_REG_STATUS); 

      if (state & ATA_SR_ERR)
         return -2;

      if (state & ATA_SR_DF)
         return -4; 

      if ((state & ATA_SR_DRQ) == 0)
         return -3; 
 
   }
 
   return 0; 
 
}

void _ide_initialize(void) {
    //char dbg[16];
 
    // Get I/O Ports
    _ide_channels[0].base = (_disk_device.bar0 & 0xFFFFFFFC);
    if (!_ide_channels[0].base) _ide_channels[0].base = 0x1F0;
    _ide_channels[0].ctrl = (_disk_device.bar1 & 0xFFFFFFFC);
    if (!_ide_channels[0].ctrl) _ide_channels[0].ctrl = 0x3F6;
    _ide_channels[1].base = (_disk_device.bar2 & 0xFFFFFFFC);
    if (!_ide_channels[1].base) _ide_channels[1].base = 0x170;
    _ide_channels[1].ctrl = (_disk_device.bar3 & 0xFFFFFFFC);
    if (!_ide_channels[1].ctrl) _ide_channels[1].ctrl = 0x376;
    //__cvthex(dbg, _ide_channels[0].base);
    //__cio_puts("C0B ");__cio_puts(dbg); __cio_putchar('\n');
    //__cvthex(dbg, _ide_channels[0].ctrl);
    //__cio_puts("C0C ");__cio_puts(dbg); __cio_putchar('\n');

    // Disable IRQs
    _ide_write(0, ATA_REG_CONTROL, _ide_channels[0].nien = 2);
    _ide_write(1, ATA_REG_CONTROL, _ide_channels[1].nien = 2);

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

            // Disable IRQs
            _ide_write(0, ATA_REG_CONTROL, 2);
            _ide_write(1, ATA_REG_CONTROL, 2);
 
            // Read identification
            _ide_read_buffer(ch, ATA_REG_DATA, (uint32_t *) _ide_buf, 128);
            //for (int k = 0; k < 512; k++) {
            //    __cvthex2(dbg, _ide_buf[k]);
            //    __cio_puts(dbg);
            //}
            _ide_devices[_ide_device_count].channel = ch;
            //__cvtdec(dbg, ch);
            //__cio_puts("chan ");__cio_puts(dbg); __cio_putchar('\n');
            _ide_devices[_ide_device_count].drive = dr;
            _ide_devices[_ide_device_count].capabilities = *((uint16_t *)(_ide_buf + ATA_IDENT_CAPABILITIES));
            _ide_devices[_ide_device_count].command_sets = *((uint32_t *)(_ide_buf + ATA_IDENT_COMMANDSETS));
 
            // Get size
            if (_ide_devices[_ide_device_count].command_sets & (1 << 26)) {
                // 48-bit
                _ide_devices[_ide_device_count].size = *((uint32_t *)(_ide_buf + ATA_IDENT_MAX_LBA_EXT));
                if (*((uint32_t *)(_ide_buf + ATA_IDENT_MAX_LBA + 4)) != 0)
                    _ide_devices[_ide_device_count].size = (uint32_t) 0xFFFFFFFC;
            } else {
                // CHS or 28-bit
                _ide_devices[_ide_device_count].size = *((uint32_t *)(_ide_buf + ATA_IDENT_MAX_LBA));
            }
            //__cvtdec(dbg, _ide_devices[_ide_device_count].size);
            //__cio_puts("Size ");__cio_puts(dbg); __cio_putchar('\n');
            //__cio_putchar(_ide_buf[ATA_IDENT_MODEL + 1]);
            //__delay(500);

            driverInterface_t dri = { 0, _ide_device_count, _disk_read, _disk_write };
            _ide_device_count++;
            int err = _fs_registerDev(dri);
            if (err < 0) {
                //__cvtdec(dbg, err);
                //__cio_puts("Error ");__cio_puts(dbg); __cio_putchar('\n');
                _kpanic("disk", "Errored on adding disk");
            }
        }
    }
}

/**
 * Conducts an ATA I/O operation
 * 
 * @param dir 0 if read, non-zero if write
 * @param 
 * 
 * @returns The number this was registered as (error if return < 0)
 */
unsigned char _ide_ata_io(unsigned char dir, unsigned char drive, unsigned int block, char *buf) {

    static const uint8_t flush[3] = {ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH_EXT};
    uint8_t mode;
    uint8_t lba_reg[6];
    uint32_t chan = _ide_devices[drive].channel; 
    uint32_t ddrive = _ide_devices[drive].drive; 
    uint32_t bus = _ide_channels[chan].base; 
    uint32_t words = 256;
    uint8_t top;

    _ide_write(chan, ATA_REG_CONTROL, _ide_channels[chan].nien = 0x02);

    if (_ide_devices[drive].capabilities & 0x200)  { 
        // LBA
        mode = 1;
        lba_reg[0] = (block & 0xFF);
        lba_reg[1] = ((block >> 8) & 0xFF);
        lba_reg[2] = ((block >> 16) & 0xFF);
        top = ((block >> 24) & 0xF);
    } else {
        // CHS - assumptions
        mode = 0;
        uint8_t sect = (block % 63) + 1;
        uint16_t cyl = (block + 1 - sect) / (16 * 63);
        lba_reg[0] = sect;
        lba_reg[1] = (cyl >> 0) & 0xFF;
        lba_reg[2] = (cyl >> 8) & 0xFF;
        top = (block + 1 - sect) % (16 * 63) / (63); 
    }

    // Wait for drive to be free
    while (_ide_read(chan, ATA_REG_STATUS) & ATA_SR_BSY);
    // Get drive with mode
    if (mode == 0) // CHS
        _ide_write(chan, ATA_REG_HDDEVSEL, 0xA0 | (ddrive << 4) | top); 
    else
        _ide_write(chan, ATA_REG_HDDEVSEL, 0xE0 | (ddrive << 4) | top); 

    //Write to registers
    _ide_write(chan, ATA_REG_SECCOUNT0, 1);
    _ide_write(chan, ATA_REG_LBA0, lba_reg[0]);
    _ide_write(chan, ATA_REG_LBA1, lba_reg[1]);
    _ide_write(chan, ATA_REG_LBA2, lba_reg[2]); 
    /*
    char res[16];
    __cvtdec(res, lba_reg[0]);
    __cio_puts("LBA0 ");__cio_puts(res); __cio_putchar('\n');
    __cvtdec(res, lba_reg[1]);
    __cio_puts("LBA1 ");__cio_puts(res); __cio_putchar('\n');
    __cvtdec(res, lba_reg[2]);
    __cio_puts("LBA2 ");__cio_puts(res); __cio_putchar('\n');
    __cvtdec(res, drive);
    __cio_puts("Drive ");__cio_puts(res); __cio_putchar('\n');
    __cvtdec(res, numsects);
    __cio_puts("LBA Mode ");__cio_puts(res); __cio_putchar('\n');
    __cvthex(res, bus);
    __cio_puts("Bus ");__cio_puts(res); __cio_putchar('\n');
    */
    //__delay(500);

    // Select mode
    uint8_t cmd;
    if (mode == 0 && dir == 0) cmd = ATA_CMD_READ_PIO;
    if (mode == 1 && dir == 0) cmd = ATA_CMD_READ_PIO;   
    if (mode == 2 && dir == 0) cmd = ATA_CMD_READ_PIO_EXT;
    if (mode == 0 && dir == 1) cmd = ATA_CMD_WRITE_PIO;
    if (mode == 1 && dir == 1) cmd = ATA_CMD_WRITE_PIO;
    if (mode == 2 && dir == 1) cmd = ATA_CMD_WRITE_PIO_EXT;

    _ide_write(chan, ATA_REG_COMMAND, cmd); 

    if (dir == 0) {
        // PIO Read.
        uint8_t err = _ide_poll_wait(chan, 1);
        if (err)
            return err; 
        for (int j = 0; j < words; j++) {
            ((uint16_t *)buf)[j] = __inw(bus);
            //__cvthex4(res, in);
            //__cio_puts(res);
        }
        //__cio_putchar('\n');
        buf += (words*2);
    } else {
        // PIO Write.
        uint8_t err = _ide_poll_wait(chan, 1);
        if (err)
            return err; 
        for (int j = 0; j < words; j++)
            __outw(bus, ((uint16_t *)buf)[j]);
        buf += (words*2);
        
        _ide_write(chan, ATA_REG_COMMAND, flush[mode]);
        err = _ide_poll_wait(chan, 1);
        if (err)
            return err; 
    }
    //__delay(500);
    //__cio_puts("IO Successful\n");
    return 0;
}

static void dummy_isr(int vector, int code) {
    //uint8_t state = _ide_read(0, ATA_REG_STATUS); 
    //char res[16];
    //__cvtdec(res, state);
    //__cio_puts("ISR Status ");__cio_puts(res); __cio_putchar('\n');
}

/**
** _disk_init()
**
** Initialize the disk protocol
*/
void _disk_init( void ) {
    _sata_dev_itr_t itr;
    _sata_device_t dev;

    __install_isr(46, dummy_isr);
    
    for (_sata_get_devices(&itr); _sata_devices_next(&itr, &dev);) {
        if (dev.class_code == 0x01 && dev.subclass == 0x01) {
            _disk_device = dev;
            _ide_initialize();
            return;
        }
    }
}

//512 bytes

int _disk_read( uint32_t blockNr, char* buf, uint8_t devId ) {
    char res[16];
    __cvtdec(res, blockNr);
    __cio_puts("Block ");__cio_puts(res); __cio_putchar('\n');
    __cvtdec(res, devId);
    __cio_puts("DI ");__cio_puts(res); __cio_putchar('\n');
    
    if (devId >= _ide_device_count) 
        return -1;
 
    if (blockNr + 1 > _ide_devices[devId].size)
        return -1;

    return _ide_ata_io(ATA_READ, devId, blockNr, buf);
}

int _disk_write( uint32_t blockNr, char* buf, uint8_t devId ) {
    if (devId >= _ide_device_count) 
        return -1;
 
    if (blockNr + 1 > _ide_devices[devId].size)
        return -1;

    return _ide_ata_io(ATA_WRITE, devId, blockNr, buf);
}

