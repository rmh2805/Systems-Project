#ifndef USER_Q_H_
#define USER_Q_H_

/**
** User function Q:  write, bogus, exit
**
** Reports itself, then tries to execute a bogus system call
**
** Invoked as:  userQ  x
**   where x is the ID character (defaults to '1')
*/

int32_t userQ( uint32_t arg1, uint32_t arg2 ) {
    char ch = 'q';    // default character to print
    char buf[128];

    // process the command-line argument
    ch = (char) arg1;

    // announce our presence
    write( CHAN_SIO, &ch, 1 );

    // try something weird
    bogus();

    // should not have come back here!
    sprint( buf, "User %c returned from bogus syscall!?!?!\n", ch );
    cwrites( buf );

    exit( 1 );

    return( 42 );  // shut the compiler up!
}

#endif
