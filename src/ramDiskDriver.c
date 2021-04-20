#include "ramDiskDriver.h"

void _rd_init();

int _rd_readBlock(uint32_t blockNr, char* buf, uint8_t devId);
int _rd_writeBlock(uint32_t blockNr, char* buf, uint8_t devId);