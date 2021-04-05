#ifndef TEST_SHELL_H_
#define TEST_SHELL_H_

int32_t testShell(uint32_t arg1, uint32_t arg2) {
    const uint32_t iBufSz = 64;
    const uint32_t oBufSz = 128;
    char iBuf[iBufSz];
    char oBuf[oBufSz];

    int32_t nRead = 0;

    cwrites("Test shell started\n");
    swrites("Test shell started\r\n");

    while (true) {
        swrites("$ ");
        nRead = readLn(CHAN_SIO, iBuf, iBufSz, true);
        if(nRead < 0) {
            cwrites("TEST SHELL: **ERROR** encountered on line read\n");
            continue;
        }

        nRead = strTrim(oBuf, iBuf);
        if(nRead == 0) {
            continue;
        }

        if(strcmp(iBuf, "exit") == 0) {
            exit(0);
        }

    }

    return E_SUCCESS;
}

#endif