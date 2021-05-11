/*
** File:	disk.c
**
** Author:	Koen Komeya
**
** Contributor:
**
** Description:	Disk protocol. Inspired by the articles on https://wiki.osdev.org/
**              This driver allows for reading/writing to ATA-compatible hard disks.
**              Each sector is assumed to be 512 bytes.
**              To initialize this module, call the _disk_init() function.
**              That function automatically adds the drive hooks to the file system.
**              The driver can also be interacted with through its public functions.
**              The _DEBUG_DISK macro can be set to enable debugging functionality.
*/

#include "common.h"

#include "klib.h"
#include "pci.h"
#include "disk.h"
#include "kfs.h"
#include "clock.h"
#include "driverInterface.h"
#include "cio.h"
#include "support.h"

// Forward declarations
int _disk_read( uint32_t blockNr, char* buf, uint8_t devId );
int _disk_write( uint32_t blockNr, char* buf, uint8_t devId );

// Some constant declarations
#define _DISK_ATA_SR_BSY     0x80    
#define _DISK_ATA_SR_DRDY    0x40    
#define _DISK_ATA_SR_DF      0x20    
#define _DISK_ATA_SR_DSC     0x10    
#define _DISK_ATA_SR_DRQ     0x08    
#define _DISK_ATA_SR_CORR    0x04    
#define _DISK_ATA_SR_IDX     0x02    
#define _DISK_ATA_SR_ERR     0x01      
#define _DISK_ATA_CMD_READ_PIO          0x20
#define _DISK_ATA_CMD_READ_PIO_EXT      0x24
#define _DISK_ATA_CMD_WRITE_PIO         0x30
#define _DISK_ATA_CMD_WRITE_PIO_EXT     0x34
#define _DISK_ATA_CMD_CACHE_FLUSH       0xE7
#define _DISK_ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define _DISK_ATA_CMD_IDENTIFY          0xEC
#define _DISK_ATA_IDENT_CYLS         (1 * 2)
#define _DISK_ATA_IDENT_HEADS        (3 * 2)
#define _DISK_ATA_IDENT_PER_TRACK    (6 * 2)
#define _DISK_ATA_IDENT_MODEL        (27 * 2)
#define _DISK_ATA_IDENT_CAPABILITIES (49 * 2)
#define _DISK_ATA_IDENT_MAX_LBA      (60 * 2)
#define _DISK_ATA_IDENT_COMMANDSETS  (82 * 2)
#define _DISK_ATA_IDENT_MAX_LBA_EXT  (100 * 2)
#define _DISK_ATA_REG_BASE_DATA       0x00
#define _DISK_ATA_REG_BASE_ERROR      0x01
#define _DISK_ATA_REG_BASE_FEATURES   0x01
#define _DISK_ATA_REG_BASE_SECCOUNT0  0x02
#define _DISK_ATA_REG_BASE_LBA0       0x03
#define _DISK_ATA_REG_BASE_LBA1       0x04
#define _DISK_ATA_REG_BASE_LBA2       0x05
#define _DISK_ATA_REG_BASE_HDDEVSEL   0x06
#define _DISK_ATA_REG_BASE_COMMAND    0x07
#define _DISK_ATA_REG_BASE_STATUS     0x07
#define _DISK_ATA_REG_CTRL_CONTROL    0x02

#define _DISK_READ      0x00
#define _DISK_WRITE     0x01

//Per-channel registers
struct _disk_ide_channel_regs {
    uint16_t base;
    uint16_t ctrl;
} _disk_ide_channels[2];

// Buffer for holding identify command output.
unsigned static char _disk_ide_buf[1024] = {0};

//Per-device registers
struct _disk_ide_device {
    uint8_t channel;
    uint8_t drive; 
    uint16_t capabilities;
    uint32_t command_sets; 
    uint32_t size;
    uint16_t heads;
    uint16_t per_track;
} _disk_ide_devices[4];

// The Device count
int _disk_ide_device_count = 0;

// The ATA disk device
_pci_device_t _disk_device;

/**
** Name:	_sleep1ms
**
** Description:	Convert an byte into a hex character string.
**
** @param buf    Result buffer
** @param value  Value to be converted
*/
static void _sleep1ms(void) {
    __delay(1);
}

// A string containing ASCII representations of hex digits.
static char _hex[] = "0123456789ABCDEF";

/**
** _cvthex2(buf, value)
**
** Convert an byte into a zero-padded hex character string.
**
** @param buf    Result buffer
** @param value  Value to be converted
*/
void _cvthex2( char *buf, int value ){
    buf[0] = _hex[(value >> 4) & 0xf];
    buf[1] = _hex[value & 0xf];
    buf[2] = 0;
}

/**
** _cvthex4(buf, value)
**
** Convert an 16-byte number into a zero-padded hex character string.
**
** @param buf    Result buffer
** @param value  Value to be converted
*/
void _cvthex4( char *buf, int value ){
    buf[0] = _hex[(value >> 12) & 0xf];
    buf[1] = _hex[(value >> 8) & 0xf];
    buf[2] = _hex[(value >> 4) & 0xf];
    buf[3] = _hex[value & 0xf];
    buf[4] = 0;
}

/**
** _cvthex8(buf, value)
**
** Convert an 32-byte number into a zero-padded hex character string.
**
** @param buf    Result buffer
** @param value  Value to be converted
*/
void _cvthex8( char *buf, int value ){
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


/**
** _disk_ide_write_base(chan, reg, value)
**
** Writes a byte to a base register.
**
** @param chan   The channel to write to 
** @param reg    The register to write to 
** @param value  The value to write
*/
static void _disk_ide_write_base(uint8_t chan, uint8_t reg, unsigned char value) {
    __outb(_disk_ide_channels[chan].base + reg, value);
}

/**
** _disk_ide_write_ctrl(chan, reg, value)
**
** Writes a byte to a control register.
**
** @param chan   The channel to write to 
** @param reg    The register to write to 
** @param value  The value to write
*/
static void _disk_ide_write_ctrl(uint8_t chan, uint8_t reg, uint8_t value) {
    __outb(_disk_ide_channels[chan].ctrl + reg, value);
}


/**
** _disk_ide_read_base(chan, reg)
**
** Reads a byte from a base register.
**
** @param chan   The channel to read from
** @param reg    The register to read from
**
** @return       the value that was read
*/
static uint8_t _disk_ide_read_base(uint8_t chan, uint8_t reg) {
    return __inb(_disk_ide_channels[chan].base + reg);
}

/**
** _disk_ide_read_buffer(chan, reg, buffer, dwords)
**
** Reads a number of uint32_t's from a base register into a buffer.
**
** @param chan   The channel to read from
** @param reg    The register to read from
** @param buffer The buffer to read into
** @param dwords Number of uint32_t's to read
*/
static void _disk_ide_read_buffer(uint8_t chan, uint8_t reg, uint32_t *buffer,
                     unsigned int dwords) {
#if _DEBUG || _DEBUG_DISK
    // Debug reading buffer
    char dbg[16];
#endif 
    for (int i = 0; i < dwords; i++) {
        buffer[i] = __inl(_disk_ide_channels[chan].base + reg);
#if _DEBUG || _DEBUG_DISK
        uint8_t *test = &((uint8_t*)buffer)[i*4];
        if (test[1] >= 0x20 && test[1] < 0x7F) __cio_putchar(test[1]);
        if (test[0] >= 0x20 && test[0] < 0x7F) __cio_putchar(test[0]);
        if (test[3] >= 0x20 && test[3] < 0x7F) __cio_putchar(test[3]);
        if (test[2] >= 0x20 && test[2] < 0x7F) __cio_putchar(test[2]);
        _cvthex8(dbg, buffer[i]);
        __cio_putchar(' ');
        __cio_puts(dbg);
#endif
        }
#if _DEBUG || _DEBUG_DISK
    __cio_putchar('\n');
#endif 
}

/**
** _disk_ide_poll_wait(chan, check)
**
** Does a polling wait on an ATA channel until it becomes free.
** Also checks for errors if check is true.
**
** @param chan   The channel to poll wait on.
** @param check  Whether it should check for errors.
**
** @return       Negative if the check was able to find errors otherwise 0
*/
static uint8_t _disk_ide_poll_wait(uint8_t chan, uint8_t check) {
 
   // Wait for BSY to be set
   _sleep1ms();
 
   // Wait for BSY to be cleared
   while (_disk_ide_read_base(chan, _DISK_ATA_REG_BASE_STATUS) & _DISK_ATA_SR_BSY);

   // Wait for RDY to be set
   while (!(_disk_ide_read_base(chan, _DISK_ATA_REG_BASE_STATUS) & _DISK_ATA_SR_DRDY));
 
   if (check) {
      uint8_t state = _disk_ide_read_base(chan, _DISK_ATA_REG_BASE_STATUS); 

      if (state & _DISK_ATA_SR_ERR)
         return -2; //We recieved an error

      if (state & _DISK_ATA_SR_DF)
         return -3; //Drive Fault

      if ((state & _DISK_ATA_SR_DRQ) == 0)
         return -4; //Drive Request should be enabled
 
   }
 
   return 0; 
 
}

/**
** _disk_ide_initialize()
**
** Initializes the IDE module of this driver.
*/
void _disk_ide_initialize(void) {
#if _DEBUG || _DEBUG_DISK
    char dbg[16];
#endif
 
    // Get I/O Ports
    _disk_ide_channels[0].base = (_disk_device.bar0 & 0xFFFFFFFC);
    if (!_disk_ide_channels[0].base) _disk_ide_channels[0].base = 0x1F0;
    _disk_ide_channels[0].ctrl = (_disk_device.bar1 & 0xFFFFFFFC);
    if (!_disk_ide_channels[0].ctrl) _disk_ide_channels[0].ctrl = 0x3F6;
    _disk_ide_channels[1].base = (_disk_device.bar2 & 0xFFFFFFFC);
    if (!_disk_ide_channels[1].base) _disk_ide_channels[1].base = 0x170;
    _disk_ide_channels[1].ctrl = (_disk_device.bar3 & 0xFFFFFFFC);
    if (!_disk_ide_channels[1].ctrl) _disk_ide_channels[1].ctrl = 0x376;
#if _DEBUG || _DEBUG_DISK
    _cvthex(dbg, _disk_ide_channels[0].base);
    __cio_puts("C0B ");__cio_puts(dbg); __cio_putchar('\n');
    _cvthex(dbg, _disk_ide_channels[0].ctrl);
    __cio_puts("C0C ");__cio_puts(dbg); __cio_putchar('\n');
#endif

    // Disable IRQs
    _disk_ide_write_ctrl(0, _DISK_ATA_REG_CTRL_CONTROL, 2);
    _disk_ide_write_ctrl(1, _DISK_ATA_REG_CTRL_CONTROL, 2);

    for (int ch = 0; ch < 2; ch++) {
        for (int dr = 0; dr < 2; dr++) {
            // Select drive
            _disk_ide_write_base(ch, _DISK_ATA_REG_BASE_HDDEVSEL, 0xA0 | (dr << 4)); 
            _sleep1ms(); 
 
            // Identify command
            _disk_ide_write_base(ch, _DISK_ATA_REG_BASE_COMMAND, _DISK_ATA_CMD_IDENTIFY);
            _sleep1ms();
 
            // Ensure this is ATA
            if (_disk_ide_read_base(ch, _DISK_ATA_REG_BASE_STATUS) == 0)
                continue;
            int fail = 0;
            uint8_t status;
            do {
                status = _disk_ide_read_base(ch, _DISK_ATA_REG_BASE_STATUS);
                if ((status & _DISK_ATA_SR_ERR)) {fail = 1; break;} 
            } while ((status & _DISK_ATA_SR_BSY) || !(status & _DISK_ATA_SR_DRQ));
            if (fail)
                continue;

            // Disable IRQs
            _disk_ide_write_ctrl(0, _DISK_ATA_REG_CTRL_CONTROL,  2);
            _disk_ide_write_ctrl(1, _DISK_ATA_REG_CTRL_CONTROL,  2);
 
            // Read identification
            _disk_ide_read_buffer(ch, _DISK_ATA_REG_BASE_DATA, (uint32_t *) _disk_ide_buf, 128);
#if _DEBUG || _DEBUG_DISK
            // Debug identification output
            for (int k = 0; k < 512; k++) {
                _cvthex2(dbg, _disk_ide_buf[k]);
                __cio_puts(dbg);
            }
#endif
            // Save output.
            _disk_ide_devices[_disk_ide_device_count].channel = ch;
#if _DEBUG || _DEBUG_DISK
            __cvtdec(dbg, ch);
            __cio_puts("chan ");__cio_puts(dbg); __cio_putchar('\n');
#endif
            _disk_ide_devices[_disk_ide_device_count].drive = dr;
            _disk_ide_devices[_disk_ide_device_count].capabilities = *((uint16_t *)(_disk_ide_buf + _DISK_ATA_IDENT_CAPABILITIES));
            _disk_ide_devices[_disk_ide_device_count].command_sets = *((uint32_t *)(_disk_ide_buf + _DISK_ATA_IDENT_COMMANDSETS));
 
            // Get size
            if (!(_disk_ide_devices[_disk_ide_device_count].capabilities & 0x200)) {
                // CHS
                _disk_ide_devices[_disk_ide_device_count].heads = *((uint16_t *)(_disk_ide_buf + _DISK_ATA_IDENT_HEADS));
                _disk_ide_devices[_disk_ide_device_count].per_track = *((uint16_t *)(_disk_ide_buf + _DISK_ATA_IDENT_PER_TRACK));
                _disk_ide_devices[_disk_ide_device_count].size = *((uint16_t *)(_disk_ide_buf + _DISK_ATA_IDENT_CYLS)) *
                        _disk_ide_devices[_disk_ide_device_count].heads *               
                        _disk_ide_devices[_disk_ide_device_count].per_track;
            } else if (_disk_ide_devices[_disk_ide_device_count].command_sets & (1 << 26)) {
                // 48-bit LBA
                _disk_ide_devices[_disk_ide_device_count].size = *((uint32_t *)(_disk_ide_buf + _DISK_ATA_IDENT_MAX_LBA_EXT));
                if (*((uint32_t *)(_disk_ide_buf + _DISK_ATA_IDENT_MAX_LBA + 4)) != 0)
                    _disk_ide_devices[_disk_ide_device_count].size = (uint32_t) 0xFFFFFFFC;
            } else {
                // 28-bit LBA
                _disk_ide_devices[_disk_ide_device_count].size = *((uint32_t *)(_disk_ide_buf + _DISK_ATA_IDENT_MAX_LBA));
            }
#if _DEBUG || _DEBUG_DISK
            // Debug model and size
            __cvtdec(dbg, _disk_ide_devices[_disk_ide_device_count].size);
            __cio_puts("Size ");__cio_puts(dbg); __cio_putchar('\n');
            __cio_putchar(_disk_ide_buf[_DISK_ATA_IDENT_MODEL + 1]);
            __delay(500);
#endif

            // Add driver
            driverInterface_t dri = { 0, _disk_ide_device_count, _disk_read_block, _disk_write_block };
            _disk_ide_device_count++;
            int err = _fs_registerDev(dri);
            if (err < 0) {
#if _DEBUG || _DEBUG_DISK
                __cvtdec(dbg, err);
                __cio_puts("Error ");__cio_puts(dbg); __cio_putchar('\n');
#endif
                _kpanic("disk", "Errored on adding disk");
            }
        }
    }
}

/**
** Conducts an ATA read or write operation.
** 
** @param dir   0 if read, 1 if write
** @param drive the drive id to read/write
** @param block the drive block/sector to read/write
** @param buf   Buffer to read from or write to
** 
** @returns     Zero if successful, otherwise negative for error.
*/
unsigned char _disk_ide_ata_io(uint8_t dir, uint8_t drive, uint32_t block, char *buf) {

    static const uint8_t flush[3] = {_DISK_ATA_CMD_CACHE_FLUSH, _DISK_ATA_CMD_CACHE_FLUSH, _DISK_ATA_CMD_CACHE_FLUSH_EXT};
    uint8_t mode;
    uint8_t lba_reg[6];
    uint32_t chan = _disk_ide_devices[drive].channel; 
    uint32_t ddrive = _disk_ide_devices[drive].drive; 
    uint32_t bus = _disk_ide_channels[chan].base; 
    uint32_t words = 256;
    uint8_t top;

    _disk_ide_write_ctrl(chan, _DISK_ATA_REG_CTRL_CONTROL, 0x02);

    if (_disk_ide_devices[drive].capabilities & 0x200)  { 
        // LBA
        mode = 1;
        lba_reg[0] = (block & 0xFF);
        lba_reg[1] = ((block >> 8) & 0xFF);
        lba_reg[2] = ((block >> 16) & 0xFF);
        top = ((block >> 24) & 0xF);
    } else {
        // CHS
        mode = 0;
        uint8_t sect = (block % _disk_ide_devices[drive].per_track) + 1;
        uint16_t cyl = (block + 1 - sect) / (_disk_ide_devices[drive].heads * _disk_ide_devices[drive].per_track);
        lba_reg[0] = sect;
        lba_reg[1] = (cyl >> 0) & 0xFF;
        lba_reg[2] = (cyl >> 8) & 0xFF;
        top = (block + 1 - sect) % (_disk_ide_devices[drive].heads * _disk_ide_devices[drive].per_track) / (_disk_ide_devices[drive].per_track); 
    }

    // Wait for drive to be free
    while (_disk_ide_read_base(chan, _DISK_ATA_REG_BASE_STATUS) & _DISK_ATA_SR_BSY);
    // Get drive with mode
    if (mode == 0) // CHS
        _disk_ide_write_base(chan, _DISK_ATA_REG_BASE_HDDEVSEL, 0xA0 | (ddrive << 4) | top); 
    else
        _disk_ide_write_base(chan, _DISK_ATA_REG_BASE_HDDEVSEL, 0xE0 | (ddrive << 4) | top); 

    // Write to registers
    _disk_ide_write_base(chan, _DISK_ATA_REG_BASE_SECCOUNT0, 1);
    _disk_ide_write_base(chan, _DISK_ATA_REG_BASE_LBA0, lba_reg[0]);
    _disk_ide_write_base(chan, _DISK_ATA_REG_BASE_LBA1, lba_reg[1]);
    _disk_ide_write_base(chan, _DISK_ATA_REG_BASE_LBA2, lba_reg[2]); 

#if _DEBUG || _DEBUG_DISK
    // Debug registers
    char res[16];
    __cvtdec(res, lba_reg[0]);
    __cio_puts("LBA0 ");__cio_puts(res); __cio_putchar('\n');
    __cvtdec(res, lba_reg[1]);
    __cio_puts("LBA1 ");__cio_puts(res); __cio_putchar('\n');
    __cvtdec(res, lba_reg[2]);
    __cio_puts("LBA2 ");__cio_puts(res); __cio_putchar('\n');
    __cvtdec(res, drive);
    __cio_puts("Drive ");__cio_puts(res); __cio_putchar('\n');
    __cvtdec(res, mode);
    __cio_puts("Mode ");__cio_puts(res); __cio_putchar('\n');
    _cvthex(res, bus);
    __cio_puts("Bus ");__cio_puts(res); __cio_putchar('\n');
    __delay(500);
#endif

    // Select mode
    uint8_t cmd;
    if (dir == 1) {
        if (mode == 2) cmd = _DISK_ATA_CMD_WRITE_PIO_EXT;
        else cmd = _DISK_ATA_CMD_WRITE_PIO;
    } else { 
        if (mode == 2) cmd = _DISK_ATA_CMD_READ_PIO_EXT;
        else cmd = _DISK_ATA_CMD_READ_PIO;
    }

    _disk_ide_write_base(chan, _DISK_ATA_REG_BASE_COMMAND, cmd); 

    if (dir == 0) {
        // PIO Read.
        uint8_t err = _disk_ide_poll_wait(chan, 1);
        if (err)
            return err; 
        for (int j = 0; j < words; j++) {
#if _DEBUG || _DEBUG_DISK
            uint16_t in = 
#endif
                ((uint16_t *)buf)[j] = __inw(bus);
#if _DEBUG || _DEBUG_DISK
            // Debug input
            char res[16];
            _cvthex4(res, in);
            __cio_puts(res);
#endif
        }
#if _DEBUG || _DEBUG_DISK
        __cio_putchar('\n');
#endif
        buf += (words*2);
    } else {
        // PIO Write.
        uint8_t err = _disk_ide_poll_wait(chan, 1);
        if (err)
            return err; 
        for (int j = 0; j < words; j++)
            __outw(bus, ((uint16_t *)buf)[j]);
        buf += (words * 2);
        
        _disk_ide_write_base(chan, _DISK_ATA_REG_BASE_COMMAND, flush[mode]);
        err = _disk_ide_poll_wait(chan, 1);
        if (err)
            return err; 
    }
#if _DEBUG || _DEBUG_DISK
    // Pause so we can see I/O output.
    __delay(500);
    __cio_puts("IO Successful\n");
#endif
    return 0;
}


/**
** _disk_dummy_isr(vector, code)
**
** A dummy ISR so we don't fault on hardware interrupts.
**
** @param vector The vector number being notified
** @param code   The code corresponding to this IRQ
*/
static void _disk_dummy_isr(int vector, int code) {
#if _DEBUG || _DEBUG_DISK
    // Debug ISR status
    uint8_t state = _disk_ide_read_base(0, _DISK_ATA_REG_BASE_STATUS); 
    char res[16];
    __cvtdec(res, state);
    __cio_puts("ISR Status ");__cio_puts(res); __cio_putchar('\n');
#endif
}

/**
** _disk_init()
**
** Initialize the disk protocol.
** Requires kfs to be initialized prior.
*/
void _disk_init( void ) {
    _pci_dev_itr_t itr;
    _pci_device_t dev;

    // Install Dummy ISR for the random interrupts.
    __install_isr(46, _disk_dummy_isr);
    
    // Find the PATA controller if any.
    for (_pci_get_devices(&itr); _pci_devices_next(&itr, &dev);) {
        if (dev.class_code == 0x01 && dev.subclass == 0x01) {
            _disk_device = dev;
            _disk_ide_initialize();
            return;
        }
    }
}

/**
** _disk_get_drive_count()
**
** Gets the amount of drives.
**
** @return        The number of drives found, otherwise negative if an error happened
*/
int _disk_get_drive_count( void ) {
    return _disk_ide_device_count;
}

/**
** _disk_get_drive_capacity(drive)
**
** Gets drive's capacity in sectors
**
** @param devId   The drive to measure
**
** @return        The capacity of the drive
*/
int _disk_get_drive_capacity( uint8_t drive ) {
    // Argument checks
    if (drive >= _disk_ide_device_count) 
        return 0;
    return _disk_ide_devices[drive].size;
}

/**
** _disk_read_block(block, buf, drive)
**
** Reads a block/sector from a given disk into a buffer.
** Implements the readBlock procedure of driverInterface_t.
**
** @param block   The block/sector to read from.
** @param buf     The buffer to read into.
** @param drive   The drive to read from
**
** @return        Zero on success, otherwise negative if an error happened
*/
int _disk_read_block( uint32_t block, char* buf, uint8_t drive ) {
#if _DEBUG || _DEBUG_DISK
    // Debug reads.
    char res[16];
    __cio_puts("Read\n");
    __cvtdec(res, block);
    __cio_puts("Block ");__cio_puts(res); __cio_putchar('\n');
    __cvtdec(res, drive);
    __cio_puts("DI ");__cio_puts(res); __cio_putchar('\n');
#endif
    
    // Argument checks
    if (drive >= _disk_ide_device_count) 
        return -1;
 
    if (block + 1 > _disk_ide_devices[drive].size)
        return -1;

    return _disk_ide_ata_io(_DISK_READ, drive, block, buf);
}

/**
** _disk_write_block(block, buf, drive)
**
** Writes to a block/sector of a given disk from a buffer.
** Implements the writeBlock procedure of driverInterface_t.
**
** @param block   The block/sector to write to.
** @param buf     The buffer to write from.
** @param drive   The drive to write to.
**
** @return        Zero on success, otherwise negative if an error happened
*/
int _disk_write_block( uint32_t block, char* buf, uint8_t drive ) {
#if _DEBUG || _DEBUG_DISK
    // Debug writes.
    char res[16];
    __cio_puts("Write\n");
    __cvtdec(res, block);
    __cio_puts("Block ");__cio_puts(res); __cio_putchar('\n');
    __cvtdec(res, drive);
    __cio_puts("DI ");__cio_puts(res); __cio_putchar('\n');
#endif

    // Argument checks
    if (drive >= _disk_ide_device_count) 
        return -1;
 
    if (block + 1 > _disk_ide_devices[drive].size)
        return -1;

    return _disk_ide_ata_io(_DISK_WRITE, drive, block, buf);
}

