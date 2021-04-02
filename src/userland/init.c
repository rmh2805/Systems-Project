#ifndef INIT_H_
#define INIT_H_

#include "doTests.c"

/**
** Initial process; it starts the other top-level user processes.
**
** Prints a message at startup, then spawns all tests, and prints '!' before 
** transitioning to wait() mode to the SIO, and startup and transition messages 
** to the console.  It also reports each child process it collects via wait() to
** the console along with that child's exit status.
*/

int init( uint32_t arg1, uint32_t arg2 ) {
    pid_t whom;
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
    spawnTests(arg1, arg2);


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
