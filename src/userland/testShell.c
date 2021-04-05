#ifndef TEST_SHELL_H_
#define TEST_SHELL_H_

int32_t testShell(uint32_t arg1, uint32_t arg2) {
    const uint32_t iBufSz = 64;
    const uint32_t oBufSz = 128;
    char iBuf[iBufSz];
    char oBuf[oBufSz];

    int32_t nRead = 0;

    while (true) {
        nRead = readLn(CHAN_SIO, iBuf, iBufSz, true);
        if(nRead < 0) {
            cwrites("TEST SHELL: **ERROR** encountered on line read\n");
            continue;
        }

        int32_t trimLen = strTrim(oBuf, iBuf);

        swrites("Trimmed: \"");
        swrites(oBuf);
        sprint(oBuf, "\"\r\nInput:   \"%s\"\r\n", iBuf);
        swrites(oBuf);
        sprint(oBuf, "Trimmed Length: %d\r\n", trimLen);
        swrites(oBuf);
        sprint(oBuf, "Source Length: %d\r\n", strlen(iBuf));
        swrites(oBuf);
        swrites("\r\n");
    }

    return E_SUCCESS;
}

#endif