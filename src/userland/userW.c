#ifndef USER_W_H_
#define USER_W_H_

/**
** User function W:  write, sleep, exit, getpid, gettime
**
** Reports its presence, then iterates 'n' times printing identifying
** information and sleeping, before exiting.
**
** Invoked as:  userW  x  n
**   where x is the ID character (defaults to '1')
**         n is the iteration count << 8 + the sleep time
*/

int32_t userW( uint32_t arg1, uint32_t arg2 ) {
    int n;
    int count = 20;   // default iteration count
    char ch = 'w';    // default character to print
    int nap = 3;      // nap length
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
    report( ch, getpid() );

    uint32_t time = gettime();

    sprint( buf, " %c[%u] ", ch, time );

    for( int i = 0; i < count ; ++i ) {
        swrites( buf );
        sleep( SEC_TO_MS(nap) );
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

#endif
