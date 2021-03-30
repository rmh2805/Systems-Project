#ifndef INIT_H_
#define INIT_H_

/**
** Initial process; it starts the other top-level user processes.
**
** Prints a message at startup, '+' after each user process is spawned,
** and '!' before transitioning to wait() mode to the SIO, and
** startup and transition messages to the console.  It also reports
** each child process it collects via wait() to the console along
** with that child's exit status.
*/

/*
** For the test programs in the baseline system, command-line arguments
** follow these rules.  The first two entries are fixed:
**
**      arg1 is the "character to print" (identifies the process)
**      arg2 is either an iteration count or a sleep time
**
** See the comment at the beginning of each user-code source file for
** information on the argument list that code expects.
*/

int init( uint32_t arg1, uint32_t arg2 ) {
    pid_t whom;
    char ch = '+';
    static int invoked = 0;
    char buf[128];

    if( invoked > 0 ) {
        cwrites( "Init RESTARTED???\n" );
        for(;;);
    }

    cwrites( "Init started\n" );
    ++invoked;

    // home up, clear
    swritech( '\x1a' );
    // wait a bit
    DELAY(STD);

    // a bit of Dante to set the mood
    swrites( "\n\nSpem relinquunt qui huc intrasti!\n\n\r" );

    // start by spawning the idle process
    whom = spawn( idle, PRIO_LOWEST, '.', 0 );
    if( whom < 0 ) {
        cwrites( "init, spawn() of idle failed!!!\n" );
    }

    /*
    ** Start all the other users
    */

    // set up for users A, B, and C initially
#ifdef SPAWN_A
    whom = spawn( main1, PRIO_STD, 'A', 30 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user A failed\n" );
    }
    swritech( ch );
#endif

#ifdef SPAWN_B
    whom = spawn( main1, PRIO_STD, 'B', 30 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user B failed\n" );
    }
    swritech( ch );
#endif

#ifdef SPAWN_C
    // "main1 C 30"
    whom = spawn( main1, PRIO_STD, 'C', 30 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user C failed\n" );
    }
    swritech( ch );
#endif

    // Users D and E are like A-C, but uses main2 instead

#ifdef SPAWN_D
    // "main2 D 20"
    whom = spawn( main2, PRIO_STD, 'D', 20 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user D failed\n" );
    }
    swritech( ch );
#endif

#ifdef SPAWN_E
    // "main2 E 20"
    whom = spawn( main2, PRIO_STD, 'E', 20 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user E failed\n" );
    }
    swritech( ch );
#endif

    // F and G behave the same way: report, sleep, exit
    // F sleeps for 20 seconds; G sleeps for 10 seconds

#ifdef SPAWN_F
    // "main3 F 20"
    whom = spawn( main3, PRIO_STD, 'F', 20 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user F failed\n" );
    }
    swritech( ch );
#endif

#ifdef SPAWN_G
    // "main3 G 10"
    whom = spawn( main3, PRIO_STD, 'G', 10 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user G failed\n" );
    }
    swritech( ch );
#endif

    // User H tests reparenting of orphaned children
    
#ifdef SPAWN_H
    // "userH H 4"
    whom = spawn( userH, PRIO_STD, 'H', 4 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user H failed\n" );
    }
    swritech( ch );
#endif

    // User I spawns several children, kills one, and waits for all
    
#ifdef SPAWN_I
    // "userI I"
    whom = spawn( userI, PRIO_STD, 'I', 0 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user I failed\n" );
    }
    swritech( ch );
#endif

    // User J tries to spawn 2 * N_PROCS children

#ifdef SPAWN_J
    // "userJ J"
    whom = spawn( userJ, PRIO_STD, 'J', 0 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user J failed\n" );
    }
    swritech( ch );
#endif

    // Users K and L iterate spawning copies of userX and sleeping
    // for varying amounts of time.

#ifdef SPAWN_K
    // "main4 K 17"
    whom = spawn( main4, PRIO_STD, 'K', 17 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user K failed\n" );
    }
    swritech( ch );
#endif

#ifdef SPAWN_L
    // "main4 L 31"
    whom = spawn( main4, PRIO_STD, 'L', 31 );
    if( whom < 0 ) {
        cwrites( "init, spawn() user L failed\n" );
    }
    swritech( ch );
#endif

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

    // Users W through Z are spawned elsewhere

    swrites( "!\r\n\n" );

    /*
    ** At this point, we go into an infinite loop waiting
    ** for our children (direct, or inherited) to exit.
    */

    cwrites( "init() transitioning to wait() mode\n" );

    for(;;) {
        int32_t status;
        pid_t whom = wait( &status );

        if( whom == E_NO_CHILDREN ) {
            cwrites( "INIT: wait() says 'no children'???\n" );
            continue;
        } else {
            sprint( buf, "INIT: pid %d exited, status %d\n", whom, status );
            cwrites( buf );
        }
    }

    /*
    ** SHOULD NEVER REACH HERE
    */

    cwrites( "*** INIT IS EXITING???\n" );
    exit( 1 );

    return( 0 );  // shut the compiler up!
}

#endif
