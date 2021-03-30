#ifndef USER_Y_H_
#define USER_Y_H_

/**
** User function Y:  write, sleep, exit
**
** Reports its PID, then iterates N times printing 'Yx' and
** sleeping for one second, then exits.
**
** Invoked as:  userY  x  n
**   where x is the ID character
**         n is the iteration count (defaults to 10)
*/

int32_t userY( uint32_t arg1, uint32_t arg2 ) {
    int count = 10;   // default iteration count
    char ch = 'Y';    // default character to print
    char ch2;         // secondary character
    char buf[128];

    // process the argument(s)
    ch2 = arg1;
    if( arg2 > 0 ) {
        count = (int) arg2;
    }

    sprint( buf, " %c%c] ", ch, ch2 );

    for( int i = 0; i < count ; ++i ) {
        swrites( buf );
        DELAY(STD);
        sleep( SEC_TO_MS(1) );
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

#endif
