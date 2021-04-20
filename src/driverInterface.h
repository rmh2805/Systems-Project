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

typedef struct driverInterface_s {
    int (* readBlock)(uint32_t blockNr, char* buf, uint8_t devId);
    int (* writeBlock)(uint32_t blockNr, char* buf, uint8_t devId);
} driverInterface_t;

#endif
