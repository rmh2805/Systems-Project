#ifndef MAIN_3_H_
#define MAIN_3_H_

/**
** User main #3:  write, sleep, exit
**
** Prints its ID, then loops N times sleeping and printing, then exits.
**
** Invoked as:  main3  x  s
**   where x is the ID character
**         s is the sleep time in seconds
*/

int32_t main3( uint32_t arg1, uint32_t arg2 ) {
    char ch;     // default character to print
    int nap;     // default sleep time

    // process our command-line arguments
    ch = (char) arg1;
    nap = (int) arg2;

    // announce our presence a little differently
    report( ch, getpid() );

    write( CHAN_SIO, &ch, 1 );

    for( int i = 0; i < 30 ; ++i ) {
        sleep( SEC_TO_MS(nap) );
        write( CHAN_SIO, &ch, 1 );
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

#endif
