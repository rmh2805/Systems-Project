#ifndef TEST_FS_3_H_
#define TEST_FS_3_H_

#include "common.h"

int testFS3(uint32_t arg1, uint32_t arg2) {
    char buf[100];
    char wBuf[26] = "HELLO WORLD IN A NEW FILE\n";
    int fp;
    int ret;

    // Create a file named test.txt
    ret = fcreate("/", "testtesttest.txt", true);
    if(ret < 0) {
        sprint(buf, "Test FS %d.%d: Failed to create \"/test.txt\" (exit status %d)\r\n", arg1, arg2, ret);
        swrites(buf);
        return ret;
    }

    // Open the test file for appending
    fp = fopen("/testtesttest.txt", true);
    if(fp < 0) {
        sprint(buf, "Test FS %d.%d: Failed to open \"/test.txt\" (exit status %d)\r\n", arg1, arg2, fp);
        swrites(buf);
        return fp;
    }
    sprint(buf, "Test FS %d.%d: Opened \"/test.txt\" as channel %d\r\n", arg1, arg2, fp);
    swrites(buf);

    // Write to the file
    swrites("\r\nEditing the new file by writing to it\r\n");
    ret = write(fp, wBuf, 26);
    if(ret < 0) {
        sprint(buf, "TEST FS %d.%d: Failed to write to \"/test.txt\" (exit status %d)\r\n", arg1, arg2, fp);
        swrites(buf);
        return ret;
    }

    // Close the file after write
    ret = fclose(fp);
    if(ret < 0) {
        sprint(buf, "Test FS %d.%d: Failed to close \"/test.txt\" (exit status %d)\r\n", arg1, arg2, ret);
        swrites(buf);
        return ret;
    }
    sprint(buf, "Test FS %d.%d: Closed \"/test.txt\"\r\n", arg1, arg2, fp);
    swrites(buf);
    

    // Open file again to read! 
    fp = fopen("/testtesttest.txt", false);
    if(fp < 0) {
        sprint(buf, "Test FS %d.%d: Failed to open \"/test.txt\" for reading (exit status %d)\r\n", arg1, arg2, fp);
        swrites(buf);
        return fp;
    }

    // Reading the new file! 
    for(ret = fReadLn(fp, buf, 100); ret >= 0; ret = fReadLn(fp, buf, 100)) {
        sprint(wBuf, "(%03d) %s\r\n", ret, buf);
        swrites(wBuf);
    }

    // Close the file after write
    ret = fclose(fp);
    if(ret < 0) {
        sprint(buf, "Test FS %d.%d: Failed to close \"/test.txt\" (exit status %d)\r\n", arg1, arg2, ret);
        swrites(buf);
        return ret;
    }
    
    return 0;
}

#endif
