/**
** @file stacks.c
**
** @author  CSCI-452 class of 20205
**
** Stack module implementation
*/

#define SP_KERNEL_SRC

#include "common.h"

#include "stacks.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

// stack management
//
// our "free list" uses the first word in the stack
// as a pointer to the next free stack

static stack_t *_free_stacks;

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

/**
** _stk_init() - initialize the stack module
**
** Sets up the system stack (for use during interrupts)
**
** Dependencies:
**    Cannot be called before kmem is initialized
**    Must be called before interrupt handling has begun
**    Must be called before process creation has begun
*/
void _stk_init( void ) {

    __cio_puts( " Stacks:" );

    // no preallocation here, so the initial free list is empty
    _free_stacks = NULL;

    // allocate the first stack for the OS
    _system_stack = _stk_alloc();
    assert( _system_stack != NULL );

    // set the initial ESP for the OS - it should point to the
    // next-to-last uint32 in the stack, so that when the code
    // at isr_save switches to the system stack and pushes the
    // error code and vector number, ESP is aligned at a multiple
    // of 16 address.
    _system_esp = ((uint32_t *) (_system_stack + 1)) - 2;

    // all done!
    __cio_puts( " done" );
}

/**
** _stk_alloc() - allocate a stack
**
** @return a pointer to the allocated stack, or NULL
*/
stack_t *_stk_alloc( void ) {
    stack_t *new;

    // see if there is an available stack
    if( _free_stacks == NULL ) {

        // none available - create a new one
        new = (stack_t *) _km_page_alloc( STACK_PAGES );

    } else {

        // OK, we know that there is at least one free stack;
        // just take the first one from the list

        new = _free_stacks;

        // unlink it by making its successor the new head of
        // the list.  this is strange, because GCC is weird
        // about doing something like
        //     _free_stacks = (stack_t *) new[0];
        // because 'new' is an array type
        //
        _free_stacks = (stack_t *) ((uint32_t *)new)[0];

        // clear out the fields in this one just to be safe
        __memclr( new, sizeof(stack_t) );

    }

    // pass it back to the caller
    return( new );
}

/**
** _stk_free() - return a stack to the free list
**
** Deallocates the supplied stack
**
** @param stk   The stack to be returned to the free list
*/
void _stk_free( stack_t *stk ) {

    // sanity check!
    if( stk == NULL ) {
        return;
    }

    // just stick this one at the front of the list

    // start by making its first word point to the
    // current head of the free list.  again, we have
    // to work around the "array type" issue here

    ((uint32_t *)stk)[0] = (uint32_t) _free_stacks;

    // now, this one is the new head of the list

    _free_stacks = stk;
}

/*
** Process management/control
*/

/*
** Debugging/tracing routines
*/

/**
** _stk_dump(msg,stk,lim)
**
** Dumps the contents of this stack to the console.  Assumes the stack
** is a multiple of four words in length.
**
** @param msg   An optional message to print before the dump
** @param s     The stack to dump out
** @param lim   Limit on the number of words to dump (0 for all)
*/

// buffer sizes (rounded up a bit)
#define HBUFSZ      48
#define CBUFSZ      24

void _stk_dump( const char *msg, stack_t *stk, uint32_t limit ) {
    int words = sizeof(stack_t) / sizeof(uint32_t);
    int eliding = 0;
    char oldbuf[HBUFSZ], buf[HBUFSZ], cbuf[CBUFSZ];
    uint32_t addr = (uint32_t ) stk;
    uint32_t *sp = (uint32_t *) stk;
    char hexdigits[] = "0123456789ABCDEF";

    // if a limit was specified, dump only that many words

    if( limit > 0 ) {
        words = limit;
        if( (words & 0x3) != 0 ) {
            // round up to a multiple of four
            words = (words & 0xfffffffc) + 4;
        }
        // skip to the new starting point
        sp += (STACK_WORDS - words);
        addr = (uint32_t) sp;
    }

    __cio_puts( "*** stack" );
    if( msg != NULL ) {
        __cio_printf( " (%s):\n", msg );
    } else {
        __cio_puts( ":\n" );
    }

    /**
    ** Output lines begin with the 8-digit address, followed by a hex
    ** interpretation then a character interpretation of four words:
    **
    ** aaaaaaaa*..xxxxxxxx..xxxxxxxx..xxxxxxxx..xxxxxxxx..cccc.cccc.cccc.cccc
    **
    ** Output lines that are identical except for the address are elided;
    ** the next non-identical output line will have a '*' after the 8-digit
    ** address field (where the '*' is in the example above).
    */

    oldbuf[0] = '\0';

    while( words > 0 ) {
        register char *bp = buf;   // start of hex field
        register char *cp = cbuf;  // start of character field
        uint32_t start_addr = addr;

        // iterate through the words for this line

        for( int i = 0; i < 4; ++i ) {
            register uint32_t curr = *sp++;
            register uint32_t data = curr;

            // convert the hex representation

            // two spaces before each entry
            *bp++ = ' ';
            *bp++ = ' ';

            for( int j = 0; j < 8; ++j ) {
                uint32_t value = (data >> 28) & 0xf;
                *bp++ = hexdigits[value];
                data <<= 4;
            }

            // now, convert the character version
            data = curr;

            // one space before each entry
            *cp++ = ' ';

            for( int j = 0; j < 4; ++j ) {
                uint32_t value = (data >> 24) & 0xff;
                *cp++ = (value >= ' ' && value < 0x7f) ? (char) value : '.';
                data <<= 8;
            }
        }
        *bp = '\0';
        *cp = '\0';
        words -= 4;
        addr += 16;

        // if this line looks like the last one, skip it

        if( __strcmp(oldbuf,buf) == 0 ) {
            ++eliding;
            continue;
        }

        // it's different, so print it

        // start with the address
        __cio_printf( "%08x%c", start_addr, eliding ? '*' : ' ' );
        eliding = 0;

        // print the words
        __cio_printf( "%s %s\n", buf, cbuf );

        // remember this line
        __memcpy( (uint8_t *) oldbuf, (uint8_t *) buf, HBUFSZ );
    }
}
