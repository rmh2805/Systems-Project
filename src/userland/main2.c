#ifndef MAIN_2_H_
#define MAIN_2_H_

/**
** User main #2:  write
**
** Prints its ID, then loops N times delaying and printing, then returns
** without calling exit().  Verifies the return byte count from each call
** to write().
**
** Invoked as:  main2  x  n
**   where x is the ID character
**         n is the iteration count
*/

int32_t main2( uint32_t arg1, uint32_t arg2 ) {
    int n;
    int count;   // default iteration count
    char ch;     // default character to print
    char buf[128];

    // process the command-line arguments
    ch = (char) arg1;
    count = (int) arg2;

    // announce our presence
    n = swritech( ch );
    if( n != 1 ) {
        sprint( buf, "User %c, write #1 returned %d\n", ch, n );
        cwrites( buf );
    }

    // iterate and print the required number of other characters
    for( int i = 0; i < count; ++i ) {
        DELAY(STD);
        n = swritech( ch );
        if( n != 1 ) {
            sprint( buf, "User %c, write #2 returned %d\n", ch, n );
            cwrites( buf );
        }
    }

    // all done!
    return( 0 );
}

#endif
