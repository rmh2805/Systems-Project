#ifndef USER_S_H_
#define USER_S_H_

/**
** User function S:  write, sleep, exit
**
** Reports itself, then loops forever, sleeping on each iteration
**
** Invoked as:  userS  x  s
**   where x is the ID character
**         s is the sleep time (defaults to 20)
*/

int32_t userS( uint32_t arg1, uint32_t arg2 ) {
    char ch = 's';    // default character to print
    int nap = 20;     // nap time
    char buf[128];

    // process the argument(s)
    ch = (char) arg1;
    if( arg2 > 0 ) {
        nap = (int) arg2;
    }

    // announce our presence
    write( CHAN_SIO, &ch, 1 );

    for(;;) {
        sleep( SEC_TO_MS(nap) );
        write( CHAN_SIO, &ch, 1 );
    }

    sprint( buf, "User %c exiting!?!?!?\n", ch );
    cwrites( buf );
    exit( 1 );

    return( 42 );  // shut the compiler up!
}

#endif
