#ifndef TEST_SHELL_H_
#define TEST_SHELL_H_

#include "common.h"
int32_t testShell(uint32_t arg1, uint32_t arg2) {
    const uint32_t iBufSz = 64;
    const uint32_t oBufSz = 128;
    char iBuf[iBufSz];
    char oBuf[oBufSz];

    int32_t nRead = 0;
    int ret;


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
            swrites("Main test shell help:\r\n");
            swrites("\thelp: prints this screen\r\n");
            swrites("\tlist [bank]: List all tests (if bank is specified, all tests in it)\r\n");
            swrites("\ttest <bank> <test>: perform test x from bank n\r\n");

            swrites("\r\n\tlogout: return to sign in\r\n");
            swrites("\tcls: clear the screen (print several lines to console)\r\n");

            swrites("\r\n\tsetgid [GID]: Set a new GID (defaults to user GID 0)\r\n");
            swrites("\tchown <uid> <gid> <path>: Set a new group and user owner for path\r\n");
            swrites("\tchmod <permStr> <path>: Set new permissions for path\r\n");
            
            swrites("\r\n\tcat <file path>: Cat the file contents out to the console\r\n");
            swrites("\tls <file path>: Print the contents and permissions of the subdirectory\r\n");
            swrites("\tap <file path>: Append a line to a file\r\n");
            swrites("\trm <file path>: Remove a directory entry\r\n");
            swrites("\tcd <file path>: Change the working directory\r\n");


        } else if(strcmp(iBuf, "exit") == 0 || strcmp(iBuf, "logoff") == 0 || 
            strcmp(iBuf, "logout") == 0 || strcmp(iBuf, "`") == 0) {    // Exit
            swrites("Exiting\r\n");
            exit(0);

        } else if (strcmp(iBuf, "cls") == 0){   // Clear the screen
            for(int i = 0; i < 80; i++) {
                swrites("\r\n");
            }
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
                sprint(oBuf, "SHELL: **ERROR** Failure to spawn test (spawn returned %d)\r\n", test);
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

        } else if (strncmp(iBuf, "chown", 5) == 0) {
            nRead = strTrim(oBuf, &iBuf[6]);
            char modBuf[32];
            char * dataPtr = oBuf;

            // Get the UID
            for(int i = 0; oBuf[i] && oBuf[i] != ' '; i++) {
                modBuf[i] = oBuf[i];
                modBuf[i + 1] = 0;
                dataPtr++;
            }
            while(*dataPtr && *dataPtr != ' ') {
                dataPtr++;
            }
            dataPtr++;
            
            
            nRead = strTrim(oBuf, modBuf);
            if(nRead <= 0) {
                swrites("SHELL: No uid specified for chown\r\n");
            }
            uid_t uid = str2int(modBuf, 10);

            // Get the GID
            for(int i = 0; *dataPtr && *dataPtr != ' '; i++) {
                modBuf[i] = *dataPtr;
                modBuf[i + 1] = 0;
                dataPtr++;
            }
            
            nRead = strTrim(oBuf, modBuf);
            if(nRead <= 0) {
                swrites("SHELL: No gid specified for chown\r\n");
            }
            gid_t gid = str2int(modBuf, 10);

            // Get the path
            strTrim(oBuf, dataPtr);
            
            fchown(oBuf, uid, gid);


        } else if (strncmp(iBuf, "chmod", 5) == 0) {
            nRead = strTrim(oBuf, &iBuf[6]);
            char modBuf[32];
            char * dataPtr = oBuf;
            for(int i = 0; oBuf[i] && oBuf[i] != ' '; i++) {
                modBuf[i] = oBuf[i];
                modBuf[i + 1] = 0;
                dataPtr++;
            }
            
            strTrim(oBuf, dataPtr);

            chmod((uint32_t) modBuf, (uint32_t) oBuf);

        } else if(strncmp(iBuf, "cat", 3) == 0) {
            strTrim(oBuf, iBuf + 3);

            ret = cat((uint32_t)&oBuf, 0);
            if(ret < 0) {
                sprint(iBuf, "Failed to cat file \"%s\"\r\n", oBuf);
                swrites(iBuf);
            }


        } else if(strncmp(iBuf, "ls", 2) == 0) {
            strTrim(oBuf, iBuf + 2);

            ret = ls((uint32_t)&oBuf, 0);
            if(ret < 0) {
                sprint(iBuf, "Failed to ls directory \"%s\"\r\n", oBuf);
                swrites(iBuf);
            }


        } else if(strncmp(iBuf, "ap", 2) == 0) {
            strTrim(oBuf, iBuf + 2);

            ret = ap((uint32_t)&oBuf, 0);
            if(ret < 0) {
                sprint(iBuf, "Failed to append line to file \"%s\"\r\n", oBuf);
                swrites(iBuf);
            }
        } else if(strncmp(iBuf, "rm", 2) == 0) {
            strTrim(oBuf, iBuf + 2);
            
            // Separate the path and the file name
            char nBuf[MAX_FILENAME_SIZE + 1];
            nBuf[MAX_FILENAME_SIZE] = 0;
            
            int i;
            for(i = strlen(oBuf); i > 0 && oBuf[i] != '/'; i--);
            if(oBuf[i] == '/') {
                oBuf[i] = 0;
                i += 1;
            }
            
            strncpy(nBuf, &(oBuf[i]), MAX_FILENAME_SIZE);

            ret = fremove((i == 0) ? "" : oBuf, nBuf);
            if(ret < 0) {
                sprint(iBuf, "Failed to remove file \"%s\" (%s)\r\n", nBuf, oBuf);
                swrites(iBuf);
            }
        } else if (strncmp(iBuf, "cd", 2) == 0) {
            strTrim(iBuf, iBuf + 2);
            
            ret = setDir(iBuf);
            if(ret < 0) {
                sprint(oBuf, "Failed to change working directory to \"%s\"\r\n", iBuf);
                swrites(oBuf);
            }
        } else {    //Unknown Command
            sprint(oBuf, "Uncrecognized command \"%s\", try \"help\"", iBuf);
            swrites(oBuf);
        }
        
        swrites("\r\n");

    }

    return E_FAILURE;
}

#endif
