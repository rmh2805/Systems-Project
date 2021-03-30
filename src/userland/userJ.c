#ifndef USER_J_H_
#define USER_J_H_

/**
** User function J:  write, getpid, spawn, exit
**
** Reports, tries to spawn lots of children, then exits
**
** Invoked as:  userJ  x  n
**   where x is the ID character
**         n is the number of children to spawn (defaults to 2 * N_PROCS)
*/

int32_t userJ( uint32_t arg1, uint32_t arg2 ) {
    int count = 2 * N_PROCS;   // number of children to spawn
    char ch = 'J';             // default character to print
    char ch2;                  // secondary char to send to 'Y'

    // process the argument(s)
    ch2 = arg1;
    if( arg2 > 0 ) {
        count = (int) arg2;
    }

    // announce our presence
    write( CHAN_SIO, &ch, 1 );

    // set up the command-line arguments:
    // userY char 10

    for( int i = 0; i < count ; ++i ) {
        int32_t whom = spawn( userY, PRIO_STD, ch2 + i, 10  );
        if( whom < 0 ) {
            write( CHAN_SIO, "!j!", 3 );
        } else {
            write( CHAN_SIO, &ch, 1 );
        }
    }

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

#endif
