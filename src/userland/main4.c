#ifndef MAIN_4_H_
#define MAIN_4_H_

/**
** User main #4:  write, getpid, spawn, sleep, exit
**
** Loops, spawning N copies of userX and sleeping between spawns.
**
** Invoked as:  main4  x  n
**   where x is the ID character
**         n is the iteration count (defaults to 5)
*/

int32_t main4( uint32_t arg1, uint32_t arg2 ) {
    int count = 5;    // default iteration count
    char ch = '4';    // default character to print
    int nap = 30;     // nap time
    char msg2[] = "*4*";

    // process the argument(s)
    ch = (char) arg1;
    if( arg2 > 0 ) {
        count = (int) arg2;
    }

    // announce our presence
    write( CHAN_SIO, &ch, 1 );

    pid_t me = getpid();

    for( int i = 0; i < count ; ++i ) {
        write( CHAN_SIO, &ch, 1 );
        int whom = spawn( userX, PRIO_STD, 'X', ((uint32_t) me << 4) + i );
        if( whom < 0 ) {
            swrites( msg2 );
        } else {
            write( CHAN_SIO, &ch, 1 );
        }
        sleep( SEC_TO_MS(nap) );
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

#endif
