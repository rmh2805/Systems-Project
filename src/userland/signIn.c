#ifndef  SIGN_IN_H_
#define  SIGN_IN_H_

#include "doTests.c"

int32_t signIn(uint32_t arg1, uint32_t arg2) {
    const uint32_t nIBuf = 16;
    const uint32_t nOBuf = 64;

    char iBuf[nIBuf];
    char oBuf[nOBuf];

    cwrites("Sign in shell started\n");

    if(getuid() != UID_ROOT) {
        cwrites("SIGN IN: **ERROR** sign in shell started as non-root user, exiting\n");
        exit(E_NO_PERMISSION);
    }
    
    swrites("Enter your (decimal) uid: ");

    int32_t result = readLn(CHAN_SIO, iBuf, nIBuf, true);
    
    
    
    
    //uid_t uid = (uid_t) str2int(iBuf, 10);
    sprint(oBuf, "\r\n%s\r\n", iBuf);
    swrites(oBuf);
    sprint(oBuf, "%d\r\n", result);
    swrites(oBuf);

    // Change to the gathered UID and spawn test generator

    //setuid(uid);

    return E_FAILURE; //spawn(spawnTests, PRIO_HIGHEST, 0, 0);
}

#endif
