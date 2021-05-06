#ifndef TEST_FS_4_H_
#define TEST_FS_4_H_

#include "common.h"

int testFS4(uint32_t arg1, uint32_t arg2) {
    char buf[100];
    int ret;

    ret = fremove("/", "test.txt");
    if(ret < 0) {
        sprint(buf, "Test FS %d.%d: Failed to remove \"/test.txt\" (exit status %d)\r\n", arg1, arg2, ret);
        swrites(buf);
        return ret;
    }

    return 0;
}

#endif
