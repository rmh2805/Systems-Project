#ifndef TEST_FS_7_H_
#define TEST_FS_7_H_

#include "common.h"

int testFS7(uint32_t arg1, uint32_t arg2) {
    const int maxWidth = 40;
    char buf[128];
    char nameBuf[MAX_FILENAME_SIZE + 1];
    int ret, nEntries, off;

    sprint(buf, "Test FS %d.%d: Attempting to create 2 dirs and move a file between them.\r\n", arg1, arg2);
    swrites(buf);

    // Create test file
    ret = fcreate("/", "test7.txt", true);
    if(ret < 0) {
        sprint(buf, "Test FS %d.%d: Failed to create \"test7.txt\" (exit status %d)\r\n", arg1, arg2, ret);
        swrites(buf);
        return ret;
    }
    sprint(buf, "Test FS %d.%d: File \"test7.txt\" created successfully.\r\n", arg1, arg2);
    swrites(buf);

    // Create target dir
    ret = fcreate("/", "testFS7direc", false);
    if(ret < 0) {
        sprint(buf, "Test FS %d.%d: Failed to create \"testFS7direc\" (exit status %d)\r\n", arg1, arg2, ret);
        swrites(buf);
        return ret;
    }
    sprint(buf, "Test FS %d.%d: Directory \"testFS7direc\" created successfully.\r\n", arg1, arg2);
    swrites(buf);

    // Move the file from root to the directory
    ret = fmove("/", "test7.txt", "/testFS7direc", "t7copy.txt");
    if(ret < 0) {
        sprint(buf, "Test FS %d.%d: Failed to move \"test7.txt\" into /testFS7direc (exit status %d)\r\n", arg1, arg2, ret);
        swrites(buf);
        return ret;
    }
    sprint(buf, "Test FS %d.%d: \"test7.txt\" moved to \"testFS7direc\\t7copy.txt\" successfully.\r\n", arg1, arg2);
    swrites(buf);

    sprint(buf, "Test FS %d.%d: Printing contents of \"testFS7direc\".\r\n", arg1, arg2);
    swrites(buf);

    // Show the file inside the directory
    nameBuf[MAX_FILENAME_SIZE] = 0; // Set a backstop null
    buf[0] = 0;  // Ensure that this is always a valid string
    off = 0;
    for(nEntries = 0; true; nEntries++) {
        int len = 0;

        ret = dirname("/testFS7direc", nameBuf, nEntries);
        if(ret == E_FILE_LIMIT) {
            strcat(buf, "\r\n");
            swrites(buf);

            sprint(buf, "Test FS %d.%d: All %d entries grabbed, exiting\r\n", arg1, arg2, nEntries);
            swrites(buf);
            break;
        } else if(ret < 0){
            strcat(buf, "\r\n");
            swrites(buf);

            sprint(buf, "Test FS %d.%d: Error encountered on entry %d, exiting (%d)\r\n", arg1, arg2, nEntries, ret);
            swrites(buf);
            cwrites(buf);
            return ret;
        }

        len = strlen(nameBuf) + 1;
        if(off + len > maxWidth) {
            strcat(buf, "\r\n");
            swrites(buf);
            
            strcpy(buf, nameBuf);
            off = 0;
        } else {
            strcat(buf, " ");
            strcat(buf, nameBuf);
        }

        off += len;
    }

    return 0;
}

#endif
