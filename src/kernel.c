/**
** @file kernel.c
**
** @author Numerous CSCI-452 classes
**
** Miscellaneous OS support routines.
*/

#define SP_KERNEL_SRC

#include "common.h"

#include "kernel.h"
#include "kmem.h"
#include "queues.h"
#include "clock.h"
#include "process.h"
#include "bootstrap.h"
#include "syscalls.h"
#include "cio.h"
#include "sio.h"
#include "scheduler.h"
#include "support.h"

// need init() and idle() addresses
#include "users.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

/*
** PUBLIC GLOBAL VARIABLES
*/

// character buffers, usable throughout the OS
// not guaranteed to retain their contents across an exception return
char b256[256];
char b512[512];

// Other kernel variables that could be defined here:
//
//     system time
//     pointer to the current process
//     information about the initial process
//         pid, PCB pointer
//     information about the idle process (if there is one)
//         pid, PCB pointer
//     information about active processes
//         static array of PCBs, active count, next available PID
//     queue variables
//     OS stack & stack pointer
//

// A separate stack for the OS itself
// (NOTE:  this assumes the OS is not reentrant!)
stack_t *_system_stack;
uint32_t *_system_esp;

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

/**
** _init - system initialization routine
**
** Called by the startup code immediately before returning into the
** first user process.
*/
void _init( void ) {

    /*
    ** BOILERPLATE CODE - taken from basic framework
    **
    ** Initialize interrupt stuff.
    */

    __init_interrupts();  // IDT and PIC initialization

    /*
    ** Console I/O system.
    **
    ** Does not depend on the other kernel modules, so we can
    ** initialize it before we initialize the kernel memory
    ** and queue modules.
    */

    __cio_init( NULL );    // no console callback routine

#ifdef TRACE_CX
    // define a scrolling region in the top 7 lines of the screen
    __cio_setscroll( 0, 7, 99, 99 );
    __cio_puts_at( 0, 6, "================================================================================" );
#endif

    /*
    ** TERM-SPECIFIC CODE STARTS HERE
    */

    /*
    ** Initialize various OS modules
    **
    ** Other modules (clock, SIO, syscall, etc.) are expected to
    ** install their own ISRs in their initialization routines.
    */

    __cio_puts( "System initialization starting.\n" );
    __cio_puts( "-------------------------------\n" );

    __cio_puts( "Modules:" );

    // call the module initialization functions, being
    // careful to follow any module precedence requirements
    //
    // classic order:  kmem; queue; everything else

    _km_init();    // MUST BE FIRST
    _que_init();   // MUST BE SECOND
    _proc_init();
    _stk_init();
    _sys_init();
    _sched_init();
    _clk_init();
    _sio_init();

    __cio_puts( "\nModule initialization complete.\n" );
    __cio_puts( "-------------------------------\n" );
    __delay( 200 );  // about 5 seconds

    /*
    ** Create the initial process
    */

    // set up the command line etc.
    uint32_t args[4];

    args[0] = (uint32_t) init;   // entry point
    args[1] = System;            // priority
    args[2] = args[3] = 0;       // no command-line arguments

    // create it; init is strange, as it is its own parent
    pcb_t *pcb = _proc_create( args, PID_INIT, PID_INIT );
    assert( pcb != NULL );

    // schedule it
    _schedule( pcb );

    // add to the active process table
    _ptable[0] = pcb;
    _active_procs = 1;

    /*
    ** Turn on the SIO receiver (the transmitter will be turned
    ** on/off as characters are being sent)
    */

    _sio_enable( SIO_RX );

    // dispatch the first user process

    _dispatch();

    /*
    ** END OF TERM-SPECIFIC CODE
    **
    ** Finally, report that we're all done.
    */

    __cio_puts( "System initialization complete.\n" );
    __cio_puts( "-------------------------------\n" );
}

#ifdef CONSOLE_SHELL
/**
** _shell - extremely simple shell for handling console input
**
** Called whenever we want to take input from the console and
** act upon it (e.g., for debugging the kernel)
**
** @param ch   The character that should be processed first
*/
void _shell( int ch ) {

    // clear the input buffer
    (void) __cio_getchar();

    __cio_puts( "\nInteractive mode ('x' to exit)\n" );

    // loop until we get an "exit" indicator

    while( 1 ) {

        // are we done?
        if( ch == 'x' || ch == EOT ) {
            break;
        }

        switch( ch ) {

        case '\r': // ignore CR and LF
        case '\n':
            break;

        case 'q':  // dump the queues
            _que_dump( "Sleep queue", _sleeping );
            _que_dump( "Reading queue", _reading );
            _que_dump( "Ready queue 0", _ready[0] );
            _que_dump( "Ready queue 1", _ready[1] );
            _que_dump( "Ready queue 2", _ready[2] );
            _que_dump( "Ready queue 3", _ready[3] );
            break;

        case 'a':  // dump the active table
            _active_dump( "\nActive processes", false );
            break;

        case 'p':  // dump the active table and all PCBs
            _active_dump( "\nActive processes", true );
            break;

        case 'c':  // dump context info for all active PCBs
            _context_dump_all( "\nContext dump" );
            break;

        case 's':  // dump stack info for all active PCBS
            __cio_puts( "\nActive stacks (w/5-sec. delays):\n" );
            for( int i = 0; i < N_PROCS; ++i ) {
                if( _ptable[i]->state != Unused ) {
                    pcb_t *pcb = _ptable[i];
                    __cio_printf( "pid %5d: ", pcb->pid );
                    __cio_printf( "EIP %08x, ", pcb->context->eip );
                    _stk_dump( NULL, pcb->stack, 12 );
                    __delay( 200 );
                }
            }
            break;
     
        default:
            __cio_printf( "shell: unknown request '0x%02x'\n", ch );
            // FALL THROUGH

        case 'h':  // help message
            __cio_puts( "\nCommands:\n" );
            __cio_puts( "   a  -- dump the active table\n" );
            __cio_puts( "   c  -- dump contexts for active processes\n" );
            __cio_puts( "   h  -- this message\n" );
            __cio_puts( "   p  -- dump the active table and all PCBs\n" );
            __cio_puts( "   q  -- dump the queues\n" );
            __cio_puts( "   s  -- dump stacks for active processes\n" );
            __cio_puts( "   x  -- exit\n" );
            break;
        }

        __cio_puts( "\n? " );
        ch = __cio_getchar();
    }

    __cio_puts( "\nLeaving interactive mode\n\n" );

    // clear the input buffer
    (void) __cio_getchar();
}
#endif
