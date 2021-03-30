#ifndef MAIN_6_H_
#define MAIN_6_H_

/**
** User main main6:  write, spawn, sleep, wait, getpid, kill
**
** Reports, then loops spawing userW, sleeps, then waits for or kills
** all its children.
**
** Invoked as:  main6  x  b
**   where x is the ID character
**         b is wait/kill boolean << 8 | child count
*/

#ifndef MAX_CHILDREN
#define MAX_CHILDREN    50
#endif

int32_t main6( uint32_t arg1, uint32_t arg2 ) {
    int count = 3;    // default child count
    char ch = '6';    // default character to print
    int nap = 8;      // nap time
    char what = 1;    // wait or kill?  default is wait
    char buf[128];
    pid_t children[MAX_CHILDREN];
    int nkids = 0;

    // process the argument(s)
    ch = (char) arg1;
    what = (int) arg2 >> 8;
    count = (int) arg2 & 0xff;

    // secondary output (for indicating errors)
    char ch2[] = "*?*";
    ch2[1] = ch;

    // announce our presence
    write( CHAN_SIO, &ch, 1 );

    // set up the argument vector
    // userW 'W' (10 << 8 + 5)

    for( int i = 0; i < count; ++i ) {
        pid_t whom = spawn( userW, PRIO_STD, 'W', (10 << 8) + 5 );
        if( whom < 0 ) {
            swrites( ch2 );
        } else {
            children[nkids++] = whom;
        }
    }

    // let the children start
    sleep( SEC_TO_MS(nap) );

    // collect exit status information

    int n = 0;

    do {
        pid_t this;
        int32_t status;

        // are we waiting for or killing it?
        if( what ) {
            this = wait( &status );
        } else {
            this = kill( n++ );
        }

        // what was the result?
        if( this < 0 ) {
            // uh-oh - something went wrong
            // "no children" means we're all done
            if( this != E_NO_CHILDREN ) {
                if( what ) {
                    sprint( buf, "User %c: wait() status %d\n", ch, this );
                } else {
                    sprint( buf, "User %c: kill() status %d\n", ch, this );
                }
                cwrites( buf );
            }
            // regardless, we're outta here
            break;
        } else {
            int i;
            for( i = 0; i < nkids; ++i ) {
                if( children[i] == this ) {
                    break;
                }
            }
            if( i < nkids ) {
                sprint( buf, "User %c: child %d (%d) status %d\n",
                        ch, i, this, status );
            } else {
                sprint( buf, "User %c: child PID %d term, NOT FOUND\n",
                        ch, this );
            }
        }

        sprint( buf, "User %c: child %d status %d\n", ch, this, status );
        cwrites( buf );

    } while( n < nkids );

    exit( 0 );

    return( 42 );  // shut the compiler up!
}

#endif
