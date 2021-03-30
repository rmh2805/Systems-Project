/**
** @file process.c
**
** @author  CSCI-452 class of 20205
**
** Process module implementation
*/

#define SP_KERNEL_SRC

#include "common.h"

#include <x86arch.h>
#include "bootstrap.h"

#include "process.h"
#include "scheduler.h"
#include "stacks.h"
#include "cio.h"

// also need the exit_helper function entry point
void exit_helper( void );

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

// PCB management
static pcb_t *_free_pcbs;

/*
** PUBLIC GLOBAL VARIABLES
*/

// next available PID
pid_t _next_pid;

// active process count
uint32_t _active_procs;

// table of active processes
pcb_t *_ptable[N_PROCS];

/*
** PRIVATE FUNCTIONS
*/

/**
** _new_pcbs() - allocate a slice and carve it into PCBs
**
** @param critical  Should we panic on allocation failure?
**
** @return true on success, else false
*/
static bool_t _new_pcbs( bool_t critical ) {
    pcb_t *slice;

    // start by carving off a slice of memory
    slice = (pcb_t *) _km_slice_alloc();

    // if this is a critical allocation and slice is NULL, we're done
    assert( !critical || slice != NULL );

    // whew - not critical; NULL slice is still a problem, though
    if( slice == NULL ) {
        return( false );
    }

    // clear out the allocated space
    __memclr( slice, SLICE_SIZE );

    for( int i = 0; i < (SLICE_SIZE / sizeof(pcb_t)); ++i ) {
        _pcb_free( slice + i );
    }

    // all done!
    return( true );
}

/*
** PUBLIC FUNCTIONS
*/

/*
** PCB manipulation
*/

/**
** _pcb_alloc() - allocate a PCB
**
** Allocates a PCB structure and returns it to the caller.
**
** @return a pointer to the allocated PCB, or NULL
*/
pcb_t *_pcb_alloc( void ) {
    pcb_t *new;

    // see if there is an available PCB
    if( _free_pcbs == NULL ) {

        // no - see if we can create some
        if( !_new_pcbs(false) ) {
            // no!  let's just leave quietly
            return( NULL );
        }
    }

    // OK, we know that there is at least one free PCB;
    // just take the first one from the list

    new = _free_pcbs;
    _free_pcbs = (pcb_t *) new->context;

    // clear out the fields in this one just to be safe
    __memclr( new, sizeof(pcb_t) );

    // pass it back to the caller
    return( new );
}

/**
** _pcb_free() - return a PCB to the free list
**
** Deallocates the supplied PCB
**
** @param pcb   The PCB to be put on the free list
*/
void _pcb_free( pcb_t *pcb ) {

    // sanity check!
    if( pcb == NULL ) {
        return;
    }

    // mark it as unused (just in case)
    pcb->state = Unused;

    // stick it at the front of the list
    pcb->context = (context_t *) _free_pcbs;
    _free_pcbs = pcb;
}

/**
** _pcb_find_pid(pid)
**
** Locate the PCB for the specified PID
**
** @param pid   The PID to look for
**
** @return A pointer to the relevant PCB, or NULL
*/
pcb_t *_pcb_find_pid( pid_t pid ) {

    // very straightforward - look through the process table
    // for a non-NULL entry that has the specified PID

    for( int i = 0; i < N_PROCS; ++i ) {
        if( _ptable[i] != NULL && _ptable[i]->pid == pid ) {
            // found it!
            return( _ptable[i] );
        }
    }

    // no such PID in the system
    return( NULL );
}

/**
** _pcb_cleanup(pcb)
**
** Reclaim a process' data structures
**
** @param pcb   The PCB to reclaim
*/
void _pcb_cleanup( pcb_t *pcb ) {

    // sanity check:  avoid deallocating a NULL pointer
    if( pcb == NULL ) {
        // should this be an error?
        return;
    }

    // clear the entry in the process table
    for( int i = 0; i < N_PROCS; ++i ) {
        if( _ptable[i] == pcb ) {
            _ptable[i] = NULL;
            break;
        }
    }

    // release the stack
    if( pcb->stack != NULL ) {
        _stk_free( pcb->stack );
    }

    // release the PCB
    _pcb_free( pcb );
}

/*
** Process management/control
*/

/**
** _proc_init() - initialize the PCB module
**
** Allocates the initial set of PCBs, does whatever else is
** needed to make it possible to create processes
**
** Dependencies:
**    Cannot be called before kmem is initialized
**    Must be called before any process creation can be done
*/
void _proc_init( void ) {

    __cio_puts( " Process:" );

    // allocate an initial slice of PCBs
    _free_pcbs = NULL;
    _new_pcbs( true );

    // reset the "active" variables
    _active_procs = 0;
    for( int i = 0; i < N_PROCS; ++i ) {
        _ptable[i] = NULL;
    }

    // first process is init, PID 1; it's created by system initialization

    // second process is idle, PID 2; it's spawned by init()
    _next_pid = 2;

    // all done!
    __cio_puts( " done" );
}

/**
** _proc_create - create a new process
**
** Performs most of the actions required to set up a new process, but
** does not add the process to the system process table
**
** Called from _sys_spawn() in the syscall module, and from _init()
** during system initialization.
**
** @param args   Entry point, priority, and command-line arguments
** @param pid    PID for new process
** @param ppid   PID of parent process
**
** @return Pointer to the new process' PCB, or NULL if memory could
**         not be allocated for the PCB or the stack
*/
pcb_t *_proc_create( uint32_t args[4], pid_t pid, pid_t ppid ) {

    // allocate the necessary data structures
    pcb_t *pcb = _pcb_alloc();
    if( pcb == NULL ) {
        return( NULL );
    }

    stack_t *stack = _stk_alloc();
    if( stack == NULL ) {
        _pcb_free( pcb );
        return( NULL );
    }

    // fill in most of the PCB
    pcb->stack    = stack;      // user runtime stack
    pcb->pid      = pid;        // unique PID
    pcb->ppid     = ppid;       // parent's PID
    pcb->state    = New;        // initial state
    pcb->priority = args[1];    // process priority
    pcb->quantum  = Q_STD;      // allotted time slice

    /*
    ** Set up the initial process stack
    **
    ** We reserve two longwords at the bottom of the stack as scratch
    ** space.  Above that, we simulate a call from exit_helper() with 
    ** two command-line arguments by pushing the arguments and then
    ** a "return address" which is the entry point to exit_helper().
    ** Above that, we place a context_t area that is initialized with
    ** the standard initial register contents.
    **
    ** The stack will then contain the following:
    **
    **     esp ->   context        <-- context save area
    **                ...          <-- context save area
    **              context        <-- context save area
    **              exit_helper    <-- return address
    **              arg 1          <-- command-line arguments
    **              arg 2
    **              0              <-- scratch space
    **              0
    */

    /*
    ** Stack alignment is controlled by the SysV ABI i386 supplement,
    ** version 1.2 (June 23, 2016), which states in section 2.2.2:
    **
    **   "The end of the input argument area shall be aligned on a 16
    **   (32 or 64, if __m256 or __m512 is passed on stack) byte boundary.
    **   In other words, the value (%esp + 4) is always a multiple of 16
    **   (32 or 64) when control is transferred to the function entry
    **   point. The stack pointer, %esp, always points to the end of the
    **   latest allocated stack frame."
    **
    ** Isn't technical documentation fun?  Ultimatly, this means that
    ** the first parameter should be on the stack at an address that is
    ** a multiple of 16.  Because our stacks are multiples of the page
    ** size, we just need to ensure that "arg1" is 16 bytes back from 
    ** the beginning of whatever follows the stack in memory.
    */
    
    // create a pointer to the last longword in the stack
    uint32_t *sp = ((uint32_t *)(stack + 1)) - 1;

    // fill in the two scratch longwords
    *sp-- = 0;
    *sp-- = 0;  // stack alignment
    
    // next, fill in the two parameters
    *sp-- = args[3];
    *sp-- = args[2];   // this entry is aligned

    // add the "return address"
    *sp = (uint32_t) exit_helper;

    // OK, now we need to add the context save area
    pcb->context = ((context_t *) sp) - 1;

    // fill in the critical registers
    pcb->context->ss = GDT_STACK;
    pcb->context->gs = GDT_DATA;
    pcb->context->fs = GDT_DATA;
    pcb->context->es = GDT_DATA;
    pcb->context->ds = GDT_DATA;
    pcb->context->cs = GDT_CODE;

    // EIP will be the entry point of the user main() function
    pcb->context->eip = args[0];

    // don't forget the flags!
    pcb->context->eflags = DEFAULT_EFLAGS;

    return( pcb );
}

/*
** Debugging/tracing routines
*/

/**
** _pcb_dump(msg,pcb)
**
** Dumps the contents of this PCB to the console
**
** @param msg   An optional message to print before the dump
** @param p     The PCB to dump out
*/
void _pcb_dump( const char *msg, register pcb_t *p ) {

    // first, the message (if there is one)
    if( msg ) {
        __cio_printf( "%s ", msg );
    }

    // the pointer
    __cio_printf( "@ %08x: ", (uint32_t) p );

    // if it's NULL, why did you bother calling me?
    if( p == NULL ) {
        __cio_puts( " NULL???\n" );
        return;
    }

    // now, the contents
    __cio_printf( " pids %d/%d state %d prio %d",
                  p->pid, p->ppid, p->state, p->priority );

    __cio_printf( "\n ticks %d/%d xit %d",
                  p->ticks, p->quantum, p->exit_status );

    // for the event, just print the 32 bits in hex
    __cio_printf( " event %08x", (uint32_t) p->event.other );
    __cio_printf( "\n context %08x stack %08x",
                  (uint32_t) p->context, (uint32_t) p->stack );

    // and the filler (just to be sure)
    __cio_puts( " fill: " );
    for( int i = 0; i < sizeof(p->filler); ++i ) {
        __cio_printf( "%02x", p->filler[i] );
    }
    __cio_putchar( '\n' );
}

/**
** _context_dump(msg,context)
**
** Dumps the contents of this process context to the console
**
** @param msg   An optional message to print before the dump
** @param c     The context to dump out
*/
void _context_dump( const char *msg, register context_t *c ) {

    // first, the message (if there is one)
    if( msg ) {
        __cio_printf( "%s ", msg );
    }

    // the pointer
    __cio_printf( "@ %08x: ", (uint32_t) c );

    // if it's NULL, why did you bother calling me?
    if( c == NULL ) {
        __cio_puts( " NULL???\n" );
        return;
    }

    // now, the contents
    __cio_printf( "ss %04x gs %04x fs %04x es %04x ds %04x cs %04x\n",
                  c->ss & 0xff, c->gs & 0xff, c->fs & 0xff,
                  c->es & 0xff, c->ds & 0xff, c->cs & 0xff );
    __cio_printf( "  edi %08x esi %08x ebp %08x esp %08x\n",
                             c->edi, c->esi, c->ebp, c->esp );
    __cio_printf( "  ebx %08x edx %08x ecx %08x eax %08x\n",
                  c->ebx, c->edx, c->ecx, c->eax );
    __cio_printf( "  vec %08x cod %08x eip %08x eflags %08x\n",
                  c->vector, c->code, c->eip, c->eflags );
}

/**
** _context_dump_all(msg)
**
** dump the process context for all active processes
**
** @param msg  Optional message to print
*/
void _context_dump_all( const char *msg ) {

    if( msg != NULL ) {
        __cio_printf( "%s: ", msg );
    }

    __cio_printf( "%d active processes\n", _active_procs );

    if( _active_procs < 1 ) {
        return;
    }

    int n = 0;
    for( int i = 0; i < N_PROCS; ++i ) {
        pcb_t *pcb = _ptable[i];
        if( pcb->state != Unused ) {
            ++n;
            __cio_printf( "%2d[%2d]: ", n, i );
            _context_dump( NULL, pcb->context );
        }
    }
}

/**
** _active_dump(msg,all)
**
** dump the contents of the "active processes" table
**
** @param msg  Optional message to print
** @param all  Dump all or only part of the relevant data
*/
void _active_dump( const char *msg, bool_t all ) {

    if( msg ) {
        __cio_printf( "%s: ", msg );
    }

    int used = 0;
    int empty = 0;

    for( int i = 0; i < N_PROCS; ++i ) {
        register pcb_t *pcb = _ptable[i];
        if( pcb->state == Unused ) {

            // an empty slot
            ++empty;

        } else {

            // a non-empty slot
            ++used;

            // if not dumping everything, add commas if needed
            if( !all && used ) {
                __cio_putchar( ',' );
            }

            // things that are always printed
            __cio_printf( " #%d: %d/%d %d", i, pcb->pid, pcb->ppid,
                          pcb->state );
            // do we want more info?
            if( all ) {
                __cio_printf( " stk %08x EIP %08x\n",
                      (uint32_t) pcb->stack, pcb->context->eip );
            }
        }
    }

    // only need this if we're doing one-line output
    if( !all ) {
        __cio_putchar( '\n' );
    }

    // sanity check - make sure we saw the correct number of table slots
    if( (used + empty) != N_PROCS ) {
        __cio_printf( "Table size %d, used %d + empty %d = %d???\n",
                      N_PROCS, used, empty, used + empty );
    }
}
