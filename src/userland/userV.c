#ifndef USER_V_H_
#define USER_V_H_

/**
** User function V:  write, getprio, setprio, sleep
**
** Reports itself, then loops reporting the current time
**
** Invoked as:  userV  x  n
**   where x is the ID character (defaults to 'v')
**         n is the iteration count << 8 | sleep time
*/

int32_t userV( uint32_t arg1, uint32_t arg2 ) {
    int n;
    int count = 3;    // default iteration count
    char ch = 'v';    // default character to print
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
        prio_t p = getprio();
        sprint( buf, " %c[%d] ", ch, (int) p );
        swrites( buf );
        if( p == User1 ) {
            p = setprio( User2 );
        } else {
            p = setprio( User1 );
        }
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

#endif
