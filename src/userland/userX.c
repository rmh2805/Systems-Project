#ifndef USER_X_H_
#define USER_X_H_

/**
** User function X:  write, exit
**
** Prints its PID at start and exit, iterates printing its character
** N times, and exits with a status equal to its PID.
**
** Invoked as:  userX  x  n
**   where x is the ID character
**         n is a value to be used when printing our character
*/

int32_t userX( uint32_t arg1, uint32_t arg2 ) {
    int count = 20;   // iteration count
    char ch = 'x';    // default character to print
    int status;
    int value = 17;   // default value
    char buf[128];

    // process the argument(s)
    ch = (char) arg1;
    value = (int) arg2;

    // announce our presence
    report( ch, status=getpid() );

    sprint( buf, " %c[%d] ", ch, value );

    for( int i = 0; i < count ; ++i ) {
        swrites( buf );
        DELAY(STD);
    }

    exit( status );

    return( 42 );  // shut the compiler up!
}

#endif
