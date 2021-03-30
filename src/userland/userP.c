#ifndef USER_P_H_
#define USER_P_H_

/**
** User function P:  write, gettime, sleep
**
** Reports itself, then loops reporting the current time
**
** Invoked as:  userP  x  n
**   where x is the ID character (defaults to 'p')
**         n is the iteration count << 8 | sleep time
*/

int32_t userP( uint32_t arg1, uint32_t arg2 ) {
    int n;
    int count = 3;    // default iteration count
    char ch = 'p';    // default character to print
    int nap = 2;      // nap time
    char buf[128];

    // process the argument(s)
    ch = (char) arg1;
    n = (int) arg2 >> 8;
    if( n > 0 ) {
        count = n;
    }
    n = (int) arg2 & 0xff;
    if( n > 0 ) {
        nap = n;
    }

    // announce our presence
    time_t now = gettime();
    sprint( buf, "User %c running, start at %d\n", ch, now );
    cwrites( buf );

    write( CHAN_SIO, &ch, 1 );

    for( int i = 0; i < count; ++i ) {
        sleep( SEC_TO_MS(nap) );
        now = gettime();
        sprint( buf, "User %c reporting time %d\n", ch, now );
        cwrites( buf );
        write( CHAN_SIO, &ch, 1 );
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

#endif
