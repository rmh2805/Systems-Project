#ifndef TEST_FS_1_H_
#define TEST_FS_1_H_

#include "common.h"

int testFS1(uint32_t arg1, uint32_t arg2) {
    char buf[128];

    int fp = fopen("/testLongNames.txt", false);
    if(fp < 0) {
        sprint(buf, "Test FS %d.%d: Failed to open \"testLongNames.txt\" (exit status %d)\r\n", arg1, arg2, fp);
        swrites(buf);
        return fp;
    }
    


    return (0);
}

#endif
