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
        if(nRead == 0) {    // Skip empty commands
            continue;
        } else if(strcmp(iBuf, "help") == 0) {  // Print command lists
            swrites("\r\nMain test shell help:\r\n");
            swrites("\thelp: prints this screen\r\n");
            swrites("\tlogout: return to sign in\r\n");
            swrites("\tsetgid [GID]: Set a new GID (defaults to user GID 0)\r\n");
            swrites("\tlist [bank]: List all tests (if bank is specified, all tests in it)\r\n");
            swrites("\ttest <bank> <test>: perform test x from bank n\r\n");


        } else if(strcmp(iBuf, "exit") == 0 || strcmp(iBuf, "logoff") == 0 || 
            strcmp(iBuf, "logout") == 0 || strcmp(iBuf, "`") == 0) {    // Exit
            swrites("Exiting\r\n");
            exit(0);

        } else if(strncmp(iBuf, "list", 4) == 0) {  // list tests
            char* tmp = &iBuf[4];
            while(*tmp == ' ') tmp++;
            listTests(CHAN_SIO, *tmp);

        } else if(strncmp(iBuf, "test", 4) == 0) {  // Test Spawning
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
            swrites("\r\n");

        } else if (strncmp(iBuf, "setgid", 6) == 0) {
            int32_t result = 0;
            gid_t gid = GID_USER;

            nRead = strTrim(oBuf, &iBuf[6]);
            if(nRead > 0) {
                gid = str2int(oBuf, 10);
            }

            result = setgid(gid);

            if(result < 0) {
                sprint(oBuf, "Failed to set GID to %d (exit code %d)\r\n", gid, result);
            } else {
                sprint(oBuf, "Set GID to %d (exit code %d)\r\n", gid, result);
            }

            swrites(oBuf);

        } else {    //Unknown Command
            swrites("Unrecognized command, try \"help\" for a list of commands\r\n");
        }

    }

    return E_FAILURE;
}

#endif