#ifndef IDLE_H_
#define IDLE_H_

/**
** Idle process:  write, getpid, gettime, exit
**
** Reports itself, then loops forever delaying and printing a character.
**
** Invoked as:  idle
*/

int idle( uint32_t arg1, uint32_t arg2 ) {
    pid_t me;
    time_t now;
    char buf[128];
    char ch = '.';

    // ignore the command-line arguments
    (void) arg1;
    (void) arg2;
    
    // get some data about our starting point
    me = getpid();
    now = gettime();

    // report our presence on the console
    sprint( buf, "Idle [%d] started at %d\n", me, (int32_t) now );
    cwrites( buf );

    write( CHAN_SIO, &ch, 1 );

    // idle() should never block - it must always be available
    // for dispatching when we need to pick a new current process

    for(;;) {
        DELAY(LONG);
        write( CHAN_SIO, &ch, 1 );
    }

    // we should never reach this point!

    now = gettime();
    sprint( buf, "+++ Idle done at %d !?!?!\n", now );
    cwrites( buf );

    exit( 1 );

    return( 42 );  // shut the compiler up!
}

#endif
