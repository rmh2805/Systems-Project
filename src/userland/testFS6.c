#ifndef TEST_FS_6_H_
#define TEST_FS_6_H_

#include "common.h"

int testFS6(uint32_t arg1, uint32_t arg2) {
    const int maxWidth = 40;
    char outBuf[128];
    char nameBuf[MAX_FILENAME_SIZE + 1];
    int ret, nEntries, off;

    sprint(outBuf, "Test FS %d.%d: Attempting to get all child names from the root node\r\n", arg1, arg2);
    swrites(outBuf);

    nameBuf[MAX_FILENAME_SIZE] = 0; // Set a backstop null
    outBuf[0] = 0;  // Ensure that this is always a valid string
    off = 0;
    for(nEntries = 0; true; nEntries++) {
        int len = 0;

        ret = dirname("/", nameBuf, nEntries);
        if(ret == E_FILE_LIMIT) {
            strcat(outBuf, "\r\n");
            swrites(outBuf);

            sprint(outBuf, "Test FS %d.%d: All %d entries grabbed, exiting\r\n", arg1, arg2, nEntries);
            swrites(outBuf);
            break;
        } else if(ret < 0){
            strcat(outBuf, "\r\n");
            swrites(outBuf);

            sprint(outBuf, "Test FS %d.%d: Error encountered on entry %d, exiting (%d)\r\n", arg1, arg2, nEntries, ret);
            swrites(outBuf);
            cwrites(outBuf);
            return ret;
        }

        len = strlen(nameBuf) + 1;
        if(off + len > maxWidth) {
            strcat(outBuf, "\r\n");
            swrites(outBuf);
            
            strcpy(outBuf, nameBuf);
            off = 0;
        } else {
            strcat(outBuf, " ");
            strcat(outBuf, nameBuf);
        }

        off += len;
    }

    return 0;
}

#endif
