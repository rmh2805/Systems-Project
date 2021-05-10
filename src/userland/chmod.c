#ifndef CHMOD_H_
#define CHMOD_H_

#include "common.h"
int chmod(uint32_t arg1, uint32_t arg2) {
    char * permStr = (char*) arg1;
    char * path = (char *) arg2;

    return E_FAILURE;
}

#endif