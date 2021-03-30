#ifndef USER_H_H_
#define USER_H_H_

/**
** User H:  write, spawn, sleep
**
** Prints its ID, then spawns 'n' children; exits before they terminate.
**
** Invoked as:  userH  x  n
**   where x is the ID character
**         n is the number of children to spawn
*/

int32_t userH( uint32_t arg1, uint32_t arg2 ) {
    int32_t ret = 0;  // return value
    int count;        // child count
    char ch;          // default character to print
    char buf[128];
    pid_t whom;

    // process our command-line arguments
    ch = (char) arg1;
    count = (int) arg2;

    // announce our presence
    swritech( ch );

    // we spawn user Z and then exit before it can terminate
    // userZ 'Z' 10

    for( int i = 0; i < count; ++i ) {

        // spawn a child
        whom = spawn( userZ, PRIO_STD, 'Z', 10 );

        // our exit status is the number of failed spawn() calls
        if( whom < 0 ) {
            sprint( buf, "User %c spawn() failed, returned %d\n", ch, whom );
            cwrites( buf );
            ret += 1;
        }
    }

    // yield the CPU so that our child(ren) can run
    sleep( 0 );

    // announce our departure
    swritech( ch );

    exit( ret );

    return( 42 );  // shut the compiler up!
}

#endif
