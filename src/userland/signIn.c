#ifndef  SIGN_IN_H_
#define  SIGN_IN_H_

#include "doTests.c"

int32_t signIn(uint32_t arg1, uint32_t arg2) {
    const uint32_t nIBuf = 16;

    char iBuf[nIBuf];
    char oBuf[nIBuf + 5]; //room for a full iBuf, zero, and 2 newlines
    char inCh;

    cwrites("Sign in shell started\n");

    if(getuid() != UID_ROOT) {
        cwrites("SIGN IN: **ERROR** sign in shell started as non-root user, exiting\n");
        exit(E_NO_PERMISSION);
    }
    
    swrites("Enter your (decimal) uid: ");

    uint32_t i = 0;
    while (i < nIBuf) {
        int32_t nRead = read(CHAN_SIO, &inCh, 1); //get the next char

        if(nRead == 0) { // try again on a null read
            continue;
        } else if (inCh == '\b' && i >= 1) {
            swrites("[bsp]");
            --i;
        } else if(inCh == '\n') {
            iBuf[i] = 0;
            break;
        } else if (inCh >= '0' && inCh <= '9') {
            swritech(inCh);
            iBuf[i] = inCh;
            i++;
        } 
    }

    if(i == nIBuf) {
        iBuf[nIBuf] = 0;
        swrites("\r\n Overflow on input buffer, taking current input.\r\n");
    }

    uid_t uid = (uid_t) str2int(iBuf, 10);
    sprint(oBuf, "\r\n%d\r\n", uid);
    swrites(oBuf);

    // Change to the gathered UID and spawn test generator

    setuid(uid);

    return spawn(spawnTests, PRIO_HIGHEST, 0, 0);
}

#endif