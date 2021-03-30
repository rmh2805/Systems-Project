#ifndef USER_R_H_
#define USER_R_H_

/**
** User function R:  write, sleep, read, exit
**
** Reports itself, then loops forever reading and printing SIO characters
**
** Invoked as:  userR  x  s
**   where x is the ID character (defaults to '1')
**         s is the initial delay time (defaults to 10)
*/

int32_t userR( uint32_t arg1, uint32_t arg2 ) {
    char ch = 'r';    // default character to print
    int delay = 10;   // initial delay
    char buf[128];
    char b2[8];

    // process the argument(s)
    ch = (int) arg1;
    if( arg2 > 0 ) {
        delay = (int) arg2;
    }

    // announce our presence
    b2[0] = ch;
    b2[1] = '\0';  // just in case!
    b2[2] = '\0';
    write( CHAN_SIO, b2, 1 );

    sleep( SEC_TO_MS(delay) );

    for(;;) {
        int32_t n = read( CHAN_SIO, &b2[1], 1 );
        if( n != 1 ) {
            sprint( buf, "User %c, read returned %d\n", ch, n );
            cwrites( buf );
            if( n == -1 ) {
                // wait a bit
                sleep( SEC_TO_MS(1) );
            }
        } else {
	    write( CHAN_SIO, b2, 2 );
        }
    }

    sprint( buf, "User %c exiting!?!?!?\n", ch );
    cwrites( buf );
    exit( 1 );

    return( 42 );  // shut the compiler up!

}

#endif
