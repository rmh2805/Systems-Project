#ifndef AP_H_
#define AP_H_

#include "common.h"
int32_t ap(uint32_t arg1, uint32_t arg2) {
    char * path = (char*) arg1;
    char lineBuf[128];
    int fp, ret;

    strTrim(path, path);

    // Open the file
    fp = fopen(path, true);

    // If the file doesn't exist, try to create one
    if(fp == E_NO_CHILDREN) {
        strcpy(lineBuf, path);
        char nBuf[MAX_FILENAME_SIZE];
        
        int i;
        for(i = strlen(lineBuf); i > 0 && lineBuf[i] != '/'; i--);
        if(lineBuf[i] == '/') {
            lineBuf[i] = 0;
            i += 1;
        }

        strncpy(nBuf, &(lineBuf[i]), MAX_FILENAME_SIZE);

        ret = fcreate((i == 0) ? "" : lineBuf, nBuf, true);
        if(ret < 0) {
            sprint(lineBuf, "*ERROR* in ap: Failed to open file %s (name %s) (%d)\r\n", path, nBuf, fp);
            swrites(lineBuf);
            return E_FAILURE;
        }

        fp = fopen(path, true);
    } 
    
    // Fail on failure to open the file
    if(fp < 0) {
        sprint(lineBuf, "*ERROR* in ap: Failed to open file %s (%d)\r\n", path, fp);
        swrites(lineBuf);
        return E_FAILURE;
    }

    // Get the string to append
    swrites("Enter the line to append:\r\n");
    readLn(CHAN_SIO, lineBuf, 126, true);
    strcat(lineBuf, "\r\n");

    // Append the string to file
    ret = write(fp, lineBuf, strlen(lineBuf));
    if(ret < 0) {
        sprint(lineBuf, "*ERROR* in ap: Failed to write out to file %s (%d)", path, ret);
        swrites(lineBuf);
        return E_FAILURE;
    }

    // Close the file and return
    fclose(fp);
    return E_SUCCESS;

}

#endif