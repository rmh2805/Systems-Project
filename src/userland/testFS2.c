#ifndef TEST_FS_2_H_
#define TEST_FS_2_H_

#include "common.h"

int testFS2(uint32_t arg1, uint32_t arg2) {
    char buf[100];
    char *wBuf = "123456789012345678901234567890123456789012345678901234567890\n";
    int fp;
    int ret;

    // Open the test file for appending
    fp = fopen("/testLongNames.txt", true);
    if(fp < 0) {
        sprint(buf, "Test FS %d.%d: Failed to open \"/testLongNames.txt\" (exit status %d)\r\n", arg1, arg2, fp);
        swrites(buf);
        return fp;
    }
    sprint(buf, "Test FS %d.%d: Opened \"/testLongNames.txt\" as channel %d\r\n", arg1, arg2, fp);
    swrites(buf);

    // Write to the file
    swrites("\r\nEditing the file by writing 'HELLO WORLD' to it\r\n");
    ret = write(fp, wBuf, strlen(wBuf));
    if(ret < 0) {
        sprint(buf, "TEST FS %d.%d: Failed to write to \"/testLongNames.txt\" (exist status %d)\r\n", arg1, arg2, fp);
        swrites(buf);
        return ret;
    }

    // Close the file after write
    ret = fclose(fp);
    if(ret < 0) {
        sprint(buf, "Test FS %d.%d: Failed to close \"/testLongNames.txt\" (exit status %d)\r\n", arg1, arg2, ret);
        swrites(buf);
        return ret;
    }

    return testFS1(arg1, arg2);
}

#endif
