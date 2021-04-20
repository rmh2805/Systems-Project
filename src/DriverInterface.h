/**
** @file DriverInterface.h
**
** @author CSCI-452 class of 20205
**
** Defines a common interface for disk drivers
*/

#ifndef DRIVER_INTERFACE_H_
#define DRIVER_INTERFACE_H_

#include "common.h"

/*
** General (C and/or assembly) definitions
**
** This section of the header file contains definitions that can be
** used in either C or assembly-language source code.
*/

#ifndef SP_ASM_SRC

/*
** Start of C-only definitions
**
** Anything that should not be visible to something other than
** the C compiler should be put here.
*/

typedef struct devInterface_s {
    int (* readBlock)(uint32_t blockNr, char* buf, uint8_t devId);
    int (* writeBlock)(uint32_t blockNr, char* buf, uint8_t devId);
} devInterface_t;

#endif
/* SP_ASM_SRC */

#endif
