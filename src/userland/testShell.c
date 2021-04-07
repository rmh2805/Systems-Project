#ifndef TEST_SHELL_H_
#define TEST_SHELL_H_

int32_t testShell(uint32_t arg1, uint32_t arg2) {
    const uint32_t iBufSz = 64;
    const uint32_t oBufSz = 128;
    char iBuf[iBufSz];
    char oBuf[oBufSz];

    int32_t nRead = 0;


    swrites("Test shell started\r\nTry help for a list of commands\r\n\n");

    while (true) {
        swrites("$ ");
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
            swrites("\tlist [n]: List all tests (if n is specified, all tests in that bank)\r\n");
            swrites("\ttest n x: perform test x from bank n\r\n");

            continue;
        }

        if(strcmp(iBuf, "exit") == 0 || strcmp(iBuf, "logoff") == 0 || 
            strcmp(iBuf, "logout") == 0 || strcmp(iBuf, "`") == 0) {
            swrites("Exiting\r\n");
            exit(0);

            continue;
        }

        if(nRead < 4) {
            swrites("Unrecognized command, try \"help\" for a list of commands\r\n");
            continue;
        }

        if(strncmp(iBuf, "list", 4) == 0) {
            if(nRead == 4) {
                listTests(CHAN_SIO, 0);
            }

            char* tmp = &iBuf[4];
            while(*tmp == ' ') tmp++;
            listTests(CHAN_SIO, *tmp);
        } else if(strncmp(iBuf, "test", 4) == 0) {
            // Gather test parameters (bank and selector)
            char* tmp = &iBuf[4];

            while(*tmp == ' ' && *tmp) tmp++;
            if(*tmp == 0) {
                swrites("Usage: test <bank> <test>\r\n");
                continue;
            }
            char ch1 = *tmp++;

            while(*tmp == ' ' && *tmp) tmp++;
            if(*tmp == 0) {
                swrites("Usage: test <bank> <test>\r\n");
                continue;
            }

            pid_t whom, test = spawnTests(ch1, *tmp);

            if(test < 0) {
                sprint(oBuf, "SHELL: **ERROR** Failure to spawn test (spawn returned %d)\r\n", whom);
                cwrites(oBuf);
                swrites(&oBuf[7]);
                continue;
            }

            sprint(oBuf, "SHELL: Spawned test %c.%c (pid %d)\r\n", 
                (uint32_t) ch1, (uint32_t) *tmp, whom);
            cwrites(oBuf);

            status_t status;
            do {    // Wait for test process to return
                whom = wait(&status);
            } while (whom != test);

            sprint(oBuf, "SHELL: Test process (pid %d) returned status %d\r\n", whom, status);
            cwrites(oBuf);

        } else {
            swrites("Unrecognized command, try \"help\" for a list of commands\r\n");
        }

    }

    return E_FAILURE;
}

#endif