#ifndef TEST_SHELL_H_
#define TEST_SHELL_H_

int32_t testShell(uint32_t arg1, uint32_t arg2) {
    const uint32_t iBufSz = 64;
    const uint32_t oBufSz = 128;
    char iBuf[iBufSz];
    char oBuf[oBufSz];

    int32_t nRead = 0;

    swrites("Test shell started\r\nTry help for a list of commands");

    while (true) {
        swrites("\r\n$ ");
        nRead = readLn(CHAN_SIO, iBuf, iBufSz, true);
        if(nRead < 0) {
            cwrites("TEST SHELL: **ERROR** encountered on line read\n");
            continue;
        }

        nRead = strTrim(iBuf, iBuf);
        if(nRead == 0) {
            continue;
        }

        if(strcmp(iBuf, "help") == 0) {
            swrites("\r\nMain test shell help:\r\n");
            swrites("\thelp: prints this screen\r\n");
            swrites("\tlogout: return to sign in\r\n");

        }

        if(strcmp(iBuf, "exit") == 0 || strcmp(iBuf, "logoff") == 0 || 
            strcmp(iBuf, "logout") == 0 || strcmp(iBuf, "`") == 0) {
            swrites("Exiting\r\n");
            exit(0);
        }

    }

    return E_FAILURE;
}

#endif