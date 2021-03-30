/*
** @file scheduler.c
**
** @author CSCI-452 class of 20205
**
** Scheduler implementation
*/

#define    SP_KERNEL_SRC

#include "common.h"
#include "syscalls.h"

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

// the ready queue:  a MLQ with four levels
queue_t _ready[N_QUEUES];

// the current user process
pcb_t *_current;

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

/**
** _sched_init() - initialize the scheduler module
**
** Allocates the ready queues and resets the "current process" pointer
**
** Dependencies:
**    Cannot be called before queues are initialized
**    Must be called before any process scheduling can be done
*/
void _sched_init( void ) {

    __cio_puts( " Sched:" );
    
    // allocate the ready queues
    for( int i = 0; i < N_QUEUES; ++i ) {
        _ready[i] = _que_alloc( NULL );
        // at this point, allocation failure is terminal
        assert( _ready[i] != NULL );
    }
    
    // reset the "current process" pointer
    _current = NULL;
    
    __cio_puts( " done" );
}

/**
** _schedule() - add a process to the ready queue
**
** Enques the supplied process according to its priority value
**
** @param pcb   The process to be scheduled
*/
void _schedule( pcb_t *pcb ) {

    // can't enque nothing
    assert1( pcb != NULL );

    // if this process is a delayed 'kill', take care of it
    if( pcb->state == Killed ) {
        _force_exit( pcb, Killed );
        return;
    }

    // bad priority value causes a fault
    assert1( pcb->priority <= PRIO_LOWEST );
    
    // mark the process as ready to execute
    pcb->state = Ready;

    // add it to the appropriate queue
    int status = _que_enque( _ready[pcb->priority], pcb, 0 );

    // failure is not an option!
    assert( status == E_SUCCESS );
}

/**
** _dispatch() - select a new "current" process
**
** Selects the highest-priority process available
*/
void _dispatch( void ) {
    pcb_t *new;
    int n;
    
    // we need to loop until we find a process
    // that's eligible to execute

    while( 1 ) {

        // find a ready queue that has an available process
        for( n = 0; n < N_QUEUES; ++n ) {
            if( _que_length(_ready[n]) > 0 ) {
                break;
            }
        }
    
        // this should never happen - if nothing else, the
        // idle process should be on the "Deferred" queue
        assert( n < N_QUEUES );
    
        // OK, we found a queue; pull the first process from it
        new = _que_deque( _ready[n] );

        // failure to deque means something serious has gone wrong
        assert( new != NULL );

        // if this process is a delayed 'kill', we need to take care
        // of it; otherwise, we've found our new current process

        if( new->state == Ready ) {
            // this is the new current process
            break;
        }

        // this is something of a sanity check - processes on the
        // ready queue are usually in the Ready state.  however, if
        // another process used kill() on this process, it may be on
        // the ready queue in the Killed state.

        if( new->state != Killed ) {
            // this is a serious problem - we have a process that
            // was on the ready queue but wasn't in either the Ready
            // state or the Killed state!
            __sprint( b256, "*** _dispatch(): found PID %d, state %d\n",
                     new->pid, new->state );
            __cio_puts( b256 );
            PANIC( 0, _dispatch );
        }

        // we found a process, but it has been killed - clean it up,
        // then loop and pick another process

        _force_exit( new, Killed );
    }
    
    // make this the current process
    _current = new;

    // set its state and remaining quantum
    new->state = Running;
    new->ticks = new->quantum;
}
