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
    
    if(bank == '0' || bank == 0) {
        sprint(oBuf, "Baseline tests (bank 0):\r\n");
        write(chan, oBuf, strlen(oBuf));
        sprint(oBuf, "\tA, B, C: Print out name a finite nr of times w/ delays\r\n");
        write(chan, oBuf, strlen(oBuf));
    }

}
#endif
