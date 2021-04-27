#include "ramDiskDriver.h"

void _rd_init() {
    __cio_puts( " RamDisk:" );
    if(_fs_register((driverInterface_t) {0, 0, _rd_readBlock, _rd_writeBlock}) < 0) {
        __cio_puts( " FAILURE" );
    }

    
    __cio_puts( " done" );
}

void blockCpy(char* src, char* dst) {
    for(unsigned int i = 0; i < 512; i++) {
        *dst++ = *src++;
    }
}

int _rd_readBlock(uint32_t blockNr, char* buf, uint8_t devId) {
    if(devId != 0) {
        return E_BAD_CHANNEL;   // Only one ramdisk, device 0
    }
    char* diskPtr = 0x0bc00 + blockNr * BLOCK_SIZE; // Calculate disk offset

    blockCpy(diskPtr, buf);
    return E_SUCCESS;
}

int _rd_writeBlock(uint32_t blockNr, char* buf, uint8_t devId) {
    if(devId != 0) {
        return E_BAD_CHANNEL;   // Only one ramdisk, device 0
    }
    char* diskPtr = 0x0bc00 + blockNr * BLOCK_SIZE; // Calculate disk offset

    blockCpy(buf, diskPtr);
    return E_SUCCESS;
}