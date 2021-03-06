#ifndef DO_TESTS_H_
#define DO_TESTS_H_



/*
** For the test programs in the baseline system ("user[A-Z]"" and 
** "main[1-9][0-9]*"), command-line arguments follow these rules.  The first two
** entries are fixed:
**
**      arg1 is the "character to print" (identifies the process)
**      arg2 is either an iteration count or a sleep time
** 
** For the multi user test programs ("mUser[1-9][0-9]*") take arguments:
**      arg1:  test number (identifies the parent test)
**      arg2: child number (identifies which child of the parent test process 
**                          this is, where the parent process is 0)
** 
** See the comment at the beginning of each user-code source file for
** information on the argument list that code expects.
*/

/**
 ** spawnTests - spawns all of the top level tests (moved from baseline init)
 ** 
 ** @param arg1 char, test bank selector (0: baseline, 1: multi-user)
 ** @param arg2 char, test selector
 **
 ** @return spawned process ID
 */
int32_t spawnTests(uint32_t arg1, uint32_t arg2) {
    int32_t (*entry)(uint32_t, uint32_t);
    prio_t prio;
    uint32_t ch1;
    uint32_t ch2;
    

    switch(arg1) {
        case '0': // Baseline tests
            if(arg2 >= 'a' && arg2 <= 'z') 
                arg2 = arg2 - 'a' + 'A';

            switch(arg2) {
                case 'A':
                    entry = main1;
                    prio = PRIO_STD;
                    ch1 = 'A';
                    ch2 = 30;
                    break;

                case 'B':
                    entry = main1;
                    prio = PRIO_STD;
                    ch1 = 'B';
                    ch2 = 30;
                    break;

                case 'C':
                    entry = main1;
                    prio = PRIO_STD;
                    ch1 = 'C';
                    ch2 = 30;
                    break;

                case 'D':
                    entry = main2;
                    prio = PRIO_STD;
                    ch1 = 'D';
                    ch2 = 20;
                    break;
                
                case 'E':
                    entry = main2;
                    prio = PRIO_STD;
                    ch1 = 'E';
                    ch2 = 20;
                    break;

                case 'F':
                    entry = main3;
                    prio = PRIO_STD;
                    ch1 = 'F';
                    ch2 = 20;
                    break;

                case 'G':
                    entry = main3;
                    prio = PRIO_STD;
                    ch1 = 'G';
                    ch2 = 10;
                    break;

                case 'H':
                    entry = userH;
                    prio = PRIO_STD;
                    ch1 = 'H';
                    ch2 = 4;
                    break;

                case 'I':
                    entry = userI;
                    prio = PRIO_STD;
                    ch1 = 'I';
                    ch2 = 0;
                    break;

                case 'J':
                    entry = userJ;
                    prio = PRIO_STD;
                    ch1 = 'J';
                    ch2 = 0;
                    break;

                case 'K':
                    entry = main4;
                    prio = PRIO_STD;
                    ch1 = 'K';
                    ch2 = 17;
                    break;

                case 'L':
                    entry = main4;
                    prio = PRIO_STD;
                    ch1 = 'L';
                    ch2 = 31;
                    break;

                case 'M':
                    entry = main5;
                    prio = PRIO_STD;
                    ch1 = 'M';
                    ch2 = 5;
                    break;

                case 'N':
                    entry = main5;
                    prio = PRIO_STD;
                    ch1 = 'N';
                    ch2 = (1 << 8) + 5;
                    break;

                case 'P':
                    entry = userP;
                    prio = PRIO_STD;
                    ch1 = 'P';
                    ch2 = (3 << 8) + 2;
                    break;

                case 'Q':
                    entry = userQ;
                    prio = PRIO_STD;
                    ch1 = 'Q';
                    ch2 = 0;
                    break;

                case 'R':
                    entry = userR;
                    prio = PRIO_STD;
                    ch1 = 'R';
                    ch2 = 10;
                    break;

                case 'S':
                    entry = userS;
                    prio = PRIO_STD;
                    ch1 = 'S';
                    ch2 = 20;
                    break;

                case 'T':
                    entry = main6;
                    prio = PRIO_STD;
                    ch1 = 'T';
                    ch2 = (1 << 8) + 6;
                    break;
                
                case 'U':
                    entry = main6;
                    prio = PRIO_STD;
                    ch1 = 'U';
                    ch2 = 6;
                    break;

                case 'V':
                    entry = userV;
                    prio = PRIO_STD;
                    ch1 = 'V';
                    ch2 = (10 << 8) + 5;
                    break;
                
                default:
                    return E_FAILURE;
            }
            break;
        case '1': // Multi-user tests
            switch(arg2) {
                case '1':
                    entry = mUser1;
                    prio = PRIO_STD;
                    ch1 = 1;
                    ch2 = 0;
                    break;

                case '2':
                    entry = mUser2;
                    prio = PRIO_STD;
                    ch1 = 2;
                    ch2 = 0;
                    break;

                case '3':
                    entry = mUser3;
                    prio = PRIO_STD;
                    ch1 = 3;
                    ch2 = 0;
                    break;
                    
                case '4':
                    entry = mUser4;
                    prio = PRIO_STD;
                    ch1 = 4;
                    ch2 = 0;
                    break;
                
                default:
                    return E_FAILURE;
            }
            break;
        case '2': // File System tests
            switch(arg2) {
                case '1':
                    entry = testFS1;
                    prio = PRIO_STD;
                    ch1 = 1;
                    ch2 = 0;
                    break;
                case '2':
                    entry = testFS2;
                    prio = PRIO_STD;
                    ch1 = 2;
                    ch2 = 0;
                    break;
                case '3':
                    entry = testFS3;
                    prio = PRIO_STD;
                    ch1 = 3;
                    ch2 = 0;
                    break;
                case '4':
                    entry = testFS4;
                    prio = PRIO_STD;
                    ch1 = 4;
                    ch2 = 0;
                    break;
                case '5':
                    entry = testFS5;
                    prio = PRIO_STD;
                    ch1 = 5;
                    ch2 = 0;
                    break;
                case '6':
                    entry = testFS6;
                    prio = PRIO_STD;
                    ch1 = 6;
                    ch2 = 0;
                    break;
                case '7':
                    entry = testFS7;
                    prio = PRIO_STD;
                    ch1 = 7;
                    ch2 = 0;
                    break;
                default:
                    return E_FAILURE;
            }
            break;

        default: // Unknown test banks
            return E_FAILURE;
    } 

    return spawn(entry, prio, ch1, ch2);
}

void listTests(int chan, char bank) {
    char oBuf[96];
    bool_t didPrint = false;
    
    if(bank == '0' || bank == 0) {
        didPrint = true;

        sprint(oBuf, "\tBaseline tests (bank 0):\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\tA, B, C: Print out name a finite nr of times w/ delays\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t   D, E: Like A-C, but also checks write's return\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t   F, G: Report, sleep (10 and 20 secs) and exit\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      H: Test orphan reparenting\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      I: Test killing children\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      J: Test overflow proc table\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t   K, L: Iterate spawning user X and sleeping\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t   M, N: Test various syscalls through children\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      P: Iterates reporting time and sleeping\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      Q: Tests a bad syscall\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      R: Echo SIO forever (NO EXIT!!!)\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      S: Sleep forever (NO EXIT!!!)\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t   T, U: Spawn user W copies and then wait or kill them\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      V: Play with process priority\r\n");
        write(chan, oBuf, strlen(oBuf));
    }

    if(bank == '1' || bank == 0) {
        didPrint = true;

        sprint(oBuf, "\tMulti User tests (bank 1):\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      1: Print own uid and gid\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      2: Test modifying own uid and gid\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      3: Test uid/gid inheretence\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      4: Test restrictions on kill\r\n");
        write(chan, oBuf, strlen(oBuf));
    }
    if(bank == '2' || bank == 0) {
        didPrint = true;

        sprint(oBuf, "\tFile System tests (bank 2):\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      1: File Read Test\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      2: File Write Test\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      3: File Create Test\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      4: File Remove Test\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      5: Inode get test\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      6: getdir tests\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      7: File move tests\r\n");
        write(chan, oBuf, strlen(oBuf));
    }

    if(!didPrint || bank == 0) {
        sprint(oBuf, "\tValid Banks:\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      0: Baseline tests\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      1: Multi-User tests\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\t\t      2: File System tests\r\n");
        write(chan, oBuf, strlen(oBuf));
    }
}
#endif
