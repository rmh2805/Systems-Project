#ifndef TEST_FS_1_H_
#define TEST_FS_1_H_

#include "common.h"

int testFS1(uint32_t arg1, uint32_t arg2) {
    char buf[128];
    char oBuf[128];
    int fp;
    int ret;
    unsigned lineLen;

    // Open the test file
    fp = fopen("/testLongNames.txt", false);
    if(fp < 0) {
        sprint(buf, "Test FS %d.%d: Failed to open \"/testLongNames.txt\" (exit status %d)\r\n", arg1, arg2, fp);
        swrites(buf);
        return fp;
    }
    sprint(buf, "Test FS %d.%d: Opened \"/testLongNames.txt\" as chanel %d\r\n", arg1, arg2, fp);
    swrites(buf);

    // Read the contents of the first line
    sprint(buf, "Test FS %d.%d: Reading the first line char by char\r\n", arg1, arg2);
    swrites(buf);

    lineLen = 0;
    for(int i = 0; i < 126; i++) {
        buf[i] = 0;

        char ch = 0;
        ret = read(fp, &ch, 1);
        if(ret < 0) {
            swrites(buf);
            swrites("\r\n");
            sprint(buf, "*ERROR* Test FS %d.%d: failed to read char %d (%d)\r\n", arg1, arg2, i, ret);
            cwrites(buf);
            swrites(buf + 8);
            return ret;
        } else if(ch == '\n') {
            buf[i] = '\r';
            buf[i+1] = '\n';
            buf[i+2] = 0;
            break;
        }

        sprint(oBuf, "Test FS %d.%d: got %d ('%c')\r\n", arg1, arg2, (int) ch, ch);
        swrites(oBuf);
        
        buf[i] = ch;
        lineLen++;
        
    }
    swrites(buf);

    // Read the contents of the remaining lines

    // Close the file
    ret = fclose(fp);
    if(ret < 0) {
        sprint(buf, "Test FS %d.%d: Failed to close \"/testLongNames.txt\" (exit status %d)\r\n", arg1, arg2, ret);
        swrites(buf);
        return ret;
    }

    return (0);
}

#endif
