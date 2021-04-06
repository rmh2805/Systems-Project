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
 ** @param arg1 Which test bank (0: baseline, 1: multi-user)
 ** @param arg2 Test identifier
 **
 ** @return spawned process ID
 */
int32_t spawnTests(uint32_t arg1, uint32_t arg2) {
    pid_t whom;

    char ch = '+';

    int32_t (*entry)(uint32_t, uint32_t);
    prio_t prio;
    uint32_t ch1;
    uint32_t ch2;
    

    switch(arg1) {
        case 0:
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

            }
            break;
        case 1:
            break;
        default:
            return E_FAILURE;
    }

    // Users M and N spawn copies of userW and userZ

#ifdef SPAWN_M
    // "main5 M 5"
    whom = spawn( main5, PRIO_STD, 'M', 5 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user M failed\n" );
    }
    swritech( ch );
#endif

#ifdef SPAWN_N
    // "main5 N (1 << 8) | 5"
    whom = spawn( main5, PRIO_STD, 'N', (1 << 8) + 5 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user N failed\n" );
    }
    swritech( ch );
#endif

    // There is no user O

    // User P iterates, reporting system time and sleeping

#ifdef SPAWN_P
    // "userP P 3<<8 + 2"
    whom = spawn( userP, PRIO_STD, 'P', (3 << 8) + 2 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user P failed\n" );
    }
    swritech( ch );
#endif

    // User Q tries to execute a bad system call

#ifdef SPAWN_Q
    // "userQ Q"
    whom = spawn( userQ, PRIO_STD, 'Q', 0 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user Q failed\n" );
    }
    swritech( ch );
#endif

    // User R reads from the SIO one character at a time, forever

#ifdef SPAWN_R
    // "userR 10"
    whom = spawn( userR, PRIO_STD, 'R', 10 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user R failed\n" );
    }
    swritech( ch );
#endif

    // User S loops forever, sleeping on each iteration

#ifdef SPAWN_S
    // "userS 20"
    whom = spawn( userS, PRIO_STD, 'S', 20 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user S failed\n" );
    }
    swritech( ch );
#endif

    // Users T and U run main6(); they spawn copies of userW,
    // then wait for them all or kill them all

#ifdef SPAWN_T
    // User T:  wait for any child each time
    // "main6 T 1 << 8 + 6"
    whom = spawn( main6, PRIO_STD, 'T', (1 << 8) + 6 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user T failed\n" );
    }
    swritech( ch );
#endif

#ifdef SPAWN_U
    // User U:  kill all children
    // "main6 U 6"
    whom = spawn( main6, PRIO_STD, 'U', 6 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user U failed\n" );
    }
    swritech( ch );
#endif

    // User V plays with its process priority a lot

#ifdef SPAWN_V
    // User V:  get and set priority
    // "userV V 10 << 8 + 5"
    whom = spawn( userV, PRIO_HIGHEST, 'V', (10 << 8) + 5 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user V failed\n" );
    }
    swritech( ch );
#endif

    // M User 1 performs tests on the getters for gid and uid

#ifdef SPAWN_M_1
    // M User 1: get and display uid and gid
    whom = spawn( mUser1, PRIO_HIGHEST, 1, 0 );
    if( whom < 0 ) {
        cwrites( "init, spawn() M User 1 failed\n" );
    }
    swritech( ch );

#endif

    // M User 2 plays with basic setting of uid and gid, both legal and illegal

#ifdef SPAWN_M_2
    // M User 2: Test changing gid and uid
    whom = spawn( mUser2, PRIO_HIGHEST, 2, 0 );
    if( whom < 0 ) {
        cwrites( "init, spawn() M User 2 failed\n" );
    }
    swritech( ch );

#endif

    // M User 3 tests inheretence of uid and gid by child processes

#ifdef SPAWN_M_3
    // M User 3: test inhereting gid and uid
    whom = spawn( mUser3, PRIO_HIGHEST, 3, 0 );
    if( whom < 0 ) {
        cwrites( "init, spawn() M User 3 failed\n" );
    }
    swritech( ch );

#endif

    return E_SUCCESS;
}

#endif
