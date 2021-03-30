#ifndef USER_I_H_
#define USER_I_H_

#ifndef MAX_CHILDREN
#define MAX_CHILDREN    50
#endif

/**
** User function I:  write, spawn, sleep, getpid, kill, getpid
**
** Reports, then loops spawing userW, sleeps, kills two children, then
** loops checking the status of all its children
**
** Invoked as:  userI [ x ]
**   where x is the ID character (defaults to 'i')
*/

int32_t userI( uint32_t arg1, uint32_t arg2 ) {
    int count = 5;    // default child count
    char ch = 'i';    // default character to print
    int nap = 5;      // nap time
    char buf[128];
    pid_t children[MAX_CHILDREN];
    int nkids = 0;

    // process the argument(s)
    ch = (char) arg1;
    if( arg2 > 0 ) {
        count = (int) arg2;
    }

    // secondary output (for indicating errors)
    char ch2[] = "*?*";
    ch2[1] = ch;

    // announce our presence
    write( CHAN_SIO, &ch, 1 );

    // set up the argument vector
    // we run:  userW 10 5

    for( int i = 0; i < count; ++i ) {
        pid_t whom = spawn( userW, PRIO_STD, 10, 5 );
        if( whom < 0 ) {
            swrites( ch2 );
        } else {
            swritech( ch );
            children[nkids++] = whom;
        }
    }

    // let the children start
    sleep( SEC_TO_MS(nap) );

    // kill two of them
    int32_t status = kill( children[1] );
    if( status ) {
        sprint( buf, "User %c: kill(%d) status %d\n", ch, children[1], status );
        cwrites( buf );
        children[1] = -42;
    }
    status = kill( children[3] );
    if( status ) {
        sprint( buf, "User %c: kill(%d) status %d\n", ch, children[3], status );
        cwrites( buf );
        children[3] = -42;
    }

    // collect child information
    while( 1 ) {
        pid_t n = wait( NULL );
        if( n == E_NO_PROCS ) {
            // no more children
            break;
        }
        for( int i = 0; i < count; ++i ) {
            if( children[i] == n ) {
                sprint( buf, "User %c: child %d (%d)\n", ch, i, children[i] );
            }
        }
        sleep( SEC_TO_MS(nap) );
    };

    // let init() clean up after us!

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

#endif
