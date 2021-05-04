#ifndef  SIGN_IN_H_
#define  SIGN_IN_H_

#include "doTests.c"

#include "common.h"

int getShadowField(const char* lineBuf, char* outBuf, int outBufLen) {
    for(int i = 0; i < outBufLen; i++) {
        outBuf[i] = lineBuf[i];
        if(lineBuf[i] == ':' || !(lineBuf[i])) {
            return i;
        }
    }
    
    return outBufLen;
}

int getShadowLine(const char* nameBuf, char * dataBuf, int dataBufLen) {
    int fp, ret;
    char tmpBuf[MAX_UNAME_SIZE];
    
    fp = fopen("/.shadow", false);
    if(fp < 0) {
        cwrites("SIGN IN: **ERROR** sign in failed to open shadow file\n");
        return fp;
    }

    // Try to match name against each line in the shadow buffer
    bool_t matched = false;
    for(ret = fReadLn(fp, dataBuf, dataBufLen); ret >= 0; ret = fReadLn(fp, dataBuf, dataBufLen)) {
        if(ret == 0) continue;
        
        // Extract the name from the data
        int nameLen = getShadowField(dataBuf, tmpBuf, MAX_UNAME_SIZE);
        if(nameLen < MAX_UNAME_SIZE) {
            tmpBuf[nameLen] = 0;
        }

        char* dataPtr = dataBuf + nameLen;
        if(!(*dataPtr)) {
            break;
        }
        dataPtr++;

        // Compare the grabbed name to the provided name
        if(strncmp(tmpBuf, nameBuf, MAX_UNAME_SIZE) == 0) {
            matched = true;
            
            int i;
            for(i = 0; i + nameLen < dataBufLen && dataPtr[i]; i++) {
                dataBuf[i] = dataPtr[i];
            }
            dataBuf[i] = 0;
            break;
        }
    }

    ret = fclose(fp);
    if(ret < 0) {
        cwrites("SIGN IN: **ERROR** sign in failed to close shadow file\n");
        return ret;
    }

    return (matched) ? 0 : 1;
}

int32_t signIn(uint32_t arg1, uint32_t arg2) {
    const uint32_t nIBuf = 32;

    char iBuf[nIBuf];
    char nameBuf[MAX_UNAME_SIZE];
    
    int32_t result = 0;
    uid_t nUID = 0;
    
    //==================<Notify Start and San Check>==================//
    swrites("\r\n========================================\r\n");

    if(getuid() != UID_ROOT) {
        cwrites("SIGN IN: **ERROR** sign in shell started as non-root user, exiting\n");
        exit(E_NO_PERMISSION);
    }
    
    sleep(100); //delay to wait out initial serial prints 
    
    //==================<Grab the next line for UID>==================//

    while (true) {
        swrites("Enter your username: ");

        // Grab the next line of input
        result = readLn(CHAN_SIO, iBuf, nIBuf, true);

        // Return failure on failure to read
        if(result < 0) {
            swrites("SIGN IN: **ERROR** Failed to read user name\r\n");
            return result;
        }

        // Trim username string
        result = strTrim(iBuf, iBuf);
        if(result == 0) {
            swrites("SIGN IN: **ERROR** Blank username detected\r\n");
            continue;
        }
        
        // Copy username into username buffer and convert to lower case
        strncpy(nameBuf, iBuf, MAX_UNAME_SIZE);
        strLower(nameBuf, nameBuf);

        // Compare username with file
        result = getShadowLine(nameBuf, iBuf, nIBuf);
        if(result < 0) {
            swrites("SIGN IN: **ERROR** Failed to compare with shadow file\r\n");
            return result;
        } 
        
        if(result == 1) {
            swrites("SIGN IN: **ERROR** Unrecognized username\r\n");
            continue;
        }

        // Get uID from file
        int result = getShadowField(iBuf, nameBuf, MAX_UNAME_SIZE);
        if(result == 0) {
            swrites("SIGN IN: **ERROR** Malformed shadow file (undefined uid)\r\n");
            return E_FAILURE;
        }

        int i;
        for(i = 0; i < result; i++) {
            if((nameBuf[i] < '0' || nameBuf[i] > '9') && nameBuf[i]) {
                break;
            }
        }
        if(i != result) {
            swrites("SIGN IN: **ERROR** Malformed shadow file (non-integer uid)\r\n");
            return E_FAILURE;
        }

        int id = str2int(nameBuf, 10);
        if(id != (nUID = id)) {
            swrites("SIGN IN: **ERROR** Malformed shadow file (overflowed uid)\r\n");
            return E_FAILURE;
        }

        break;
    }
    
    //===============<Set UID and Spawn Test Generator>===============//
    swrites("\r\n");
    setuid(nUID);

    return spawn(testShell, PRIO_STD, 0, 0);
}

#endif
