#ifndef  RAM_DISK_DRIVER_H_
#define  RAM_DISK_DRIVER_H_

#define SP_KERNEL_SRC
#include "common.h"
#include "driverInterface.h"

#define DISK_LOAD_POINT 0x0bc00

void _rd_init();

int _rd_readBlock(uint32_t blockNr, char* buf, uint8_t devId);
int _rd_writeBlock(uint32_t blockNr, char* buf, uint8_t devId);



#endif //RAM_DISK_DRIVER_H_