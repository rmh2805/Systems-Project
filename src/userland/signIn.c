#ifndef  SIGN_IN_H_
#define  SIGN_IN_H_

#include "doTests.c"

int32_t signIn(uint32_t arg1, uint32_t arg2) {
    const uint32_t nIBuf = 32;

    char iBuf[nIBuf];
    
    int32_t result = 0;
    uid_t nUID = 0;
    
    //==================<Notify Start and San Check>==================//
    cwrites("Sign in shell started\n");

    if(getuid() != UID_ROOT) {
        cwrites("SIGN IN: **ERROR** sign in shell started as non-root user, exiting\n");
        exit(E_NO_PERMISSION);
    }
    
    sleep(100); //delay to wait out initial serial prints 
    
    //==================<Grab the next line for UID>==================//
    while (true) {
        swrites("Enter your (decimal) uid: ");

        result = readLn(CHAN_SIO, iBuf, nIBuf, true);
        
        if(result == 0) { //Default to user root
            strcpy(iBuf, "0");
            result = 1;
        }
        
        if(result < 0) {
            cwrites("SIGN IN: **ERROR** sign in shell failed to read uid, exiting\n");
            exit(E_NO_DATA);
        }
        
        uint32_t i = 0;
        for(; i < result; i++) {
            if(iBuf[i] < '0' || iBuf[i] > '9') {
                break;
            }
        }
        
        if(i < result) {
            swrites("SIGN IN: **ERROR** non-decimal UID entered\r\n");
           continue;
        }
        
        //Check that UID is in bounds
        i = str2int(iBuf, 10);
        if(i != (uid_t) i) {
            swrites("SIGN IN: **ERROR** overflowed UID entered\r\n");
            continue;
        }
        
        nUID = i;
        break;
    }
    
    //===============<Set UID and Spawn Test Generator>===============//
    setuid(nUID);

    return spawn(spawnTests, PRIO_HIGHEST, 0, 0);
}

#endif
