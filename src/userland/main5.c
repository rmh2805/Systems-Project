#ifndef MAIN_5_H_
#define MAIN_5_H_

/**
** User main #5:  write, spawn, exit
**
** Iterates spawning copies of userW (and possibly userZ), reporting
** their PIDs as it goes.
**
** Invoked as:  main5  x  n
**   where x is the ID character
**         n is the w&z boolean << 8 | the iteration count
*/

int32_t main5( uint32_t arg1, uint32_t arg2 ) {
    int count = 5;  // default iteration count
    char ch = '5';  // default character to print
    int alsoZ = 0;  // also do userZ?
    char msg2[] = "*5*";

    // process the argument(s)
    ch = (char) arg1;
    alsoZ = (int) arg2 >> 8;
    if( ((int) arg2 & 0xff) > 0 ) {
        count = (int) arg2 & 0xff;
    }

    // announce our presence
    write( CHAN_SIO, &ch, 1 );

    // set up the argument vector(s)
    // userW W (15 << 8 + 5)
    // userZ Z 15

    for( int i = 0; i < count; ++i ) {
        write( CHAN_SIO, &ch, 1 );
        pid_t whom = spawn( userW, PRIO_STD, 'W', (15 << 8) + 5 );
        if( whom < 1 ) {
            swrites( msg2 );
        }
        if( alsoZ ) {
            whom = spawn( userZ, PRIO_STD, 'Z', 15 );
            if( whom < 1 ) {
                swrites( msg2 );
            }
        }
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

#endif
