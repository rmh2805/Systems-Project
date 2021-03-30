/**
** @file syscalls.c
**
** @author CSCI-452 class of 20205
**
** System call implementations
*/

#define SP_KERNEL_SRC

#include "common.h"

#include "x86arch.h"
#include "x86pic.h"
#include "uart.h"

#include "support.h"
#include "bootstrap.h"

#include "syscalls.h"
#include "scheduler.h"
#include "process.h"
#include "stacks.h"
#include "clock.h"
#include "cio.h"
#include "sio.h"

// copied from ulib.h
extern void exit_helper( void );

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

// the system call jump table
//
// initialized by _sys_init() to ensure that the code::function mappings
// are correct even if the code values should happen to change

static void (*_syscalls[N_SYSCALLS])( uint32_t args[] );

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** PRIVATE FUNCTIONS
*/

/**
** Name:  _sys_isr
**
** System call ISR
**
** @param vector    Vector number for the clock interrupt
** @param code      Error code (0 for this interrupt)
*/
static void _sys_isr( int vector, int code ) {

    // if there is no current process, we're in deep trouble
    assert( _current != NULL );

    // much less likely to occur, but still potentially problematic
    assert2( _current->context != NULL );

    // retrieve the arguments to the system call
    // (even if they aren't needed)
    uint32_t args[4];
    args[0] = ARG( _current, 1 );
    args[1] = ARG( _current, 2 );
    args[2] = ARG( _current, 3 );
    args[3] = ARG( _current, 4 );

    // retrieve the code
    uint32_t syscode = REG( _current, eax );

    // validate the code
    if( syscode >= N_SYSCALLS ) {
        // uh-oh....
        __sprint( b256, "PID %d bad syscall %d", _current->pid, syscode );
        WARNING( b256 );
        // force a call to exit()
        syscode = SYS_exit;
        args[0] = E_BAD_SYSCALL;
    }

    // handle the system call
    _syscalls[syscode]( args );

    // tell the PIC we're done
    __outb( PIC_PRI_CMD_PORT, PIC_EOI );
}

/**
** Second-level syscall handlers
**
** All have this prototype:
**
**    static void _sys_NAME( uint32_t args[4] );
**
** Values being returned to the user are placed into the EAX
** field in the context save area for that process.
*/

/**
** _sys_exit - terminate the calling process
**
** implements:
**    void exit( int32_t status );
*/
static void _sys_exit( uint32_t args[4] ) {
    int32_t status = (int32_t) args[0];
    
    // record the termination status
    _current->exit_status = status;

    // perform exit processing for this process
    _force_exit( _current, status );
    
    // this process is done, so we need to pick another one
    _dispatch();
}

/**
** _sys_read - read into a buffer from a stream
**
** implements:
**    int32_t read( int chan, void *buffer, uint32_t length );
*/
static void _sys_read( uint32_t args[4] ) {
    int n = 0;
    uint32_t chan = args[0];
    char *buf = (char *) args[1];
    uint32_t length = args[2];

    // try to get the next character(s)
    switch( chan ) {
    case CHAN_CONS:
        // console input is non-blocking
        if( __cio_input_queue() < 1 ) {
            RET(_current) = E_NO_DATA;
            return;
        }
        // at least one character
        n = __cio_gets( buf, length );
        break;

    case CHAN_SIO:
        // this may block the process; if so,
        // _sio_reads() will dispatch a new one
        n = _sio_reads( buf, length );
        break;

    default:
        // bad channel code
        RET(_current) = E_BAD_CHANNEL;
        return;
    }

    // if there was data, return the byte count to the process;
    // otherwise, block the process until data is available
    if( n > 0 ) {

        RET(_current) = n;

    } else {

        // mark it as blocked
        _current->state = Blocked;

        // put it on the SIO input queue
        assert( _que_enque(_reading,_current,0) == E_SUCCESS );

        // select a new current process
        _dispatch();
    }
}

/**
** _sys_write - write from a buffer to a stream
**
** implements:
**    int32_t write( int chan, const void *buffer, uint32_t length );
*/
static void _sys_write( uint32_t args[4] ) {
    uint32_t chan = args[0];
    char *buf = (char *) args[1];
    uint32_t length = args[2];

    // this is almost insanely simple, but it does separate the
    // low-level device access fromm the higher-level syscall implementation

    switch( chan ) {
    case CHAN_CONS:
        __cio_write( buf, length );
        RET(_current) = length;
        break;

    case CHAN_SIO:
        _sio_write( buf, length );
        RET(_current) = length;
        break;

    default:
        RET(_current) = E_BAD_CHANNEL;
        break;
    }
}

/**
** _sys_getpid - retrieve the PID of this process
**
** implements:
**    pid_t getpid( void );
*/
static void _sys_getpid( uint32_t args[4] ) {
    RET(_current) = _current->pid;
}

/**
** _sys_getppid - retrieve the PID of the parent of this process
**
** implements:
**    pid_t getppid( void );
*/
static void _sys_getppid( uint32_t args[4] ) {
    RET(_current) = _current->ppid;
}

/**
** _sys_gettime - retrieve the current system time
**
** implements:
**    time_t gettime( void );
*/
static void _sys_gettime( uint32_t args[4] ) {
    RET(_current) = _system_time;
}

/**
** _sys_getprio - retrieve the priority for this process
**
** implements:
**    prio_t getprio( void );
*/
static void _sys_getprio( uint32_t args[4] ) {
    RET(_current) = _current->priority;
}

/**
** _sys_setprio - set the priovity for this process
**
** implements:
**    prio_t setprio( prio_t new );
*/
static void _sys_setprio( uint32_t args[4] ) {
    
    if( args[0] > PRIO_LOWEST ) {
        RET(_current) = E_BAD_PARAM;
    } else {
        RET(_current) = _current->priority;
        _current->priority = args[0];
    }
}

/**
** _sys_kill - terminate a process with extreme prejudice
**
** implements:
**    int32_t kill( pid_t victim );
*/
static void _sys_kill( uint32_t args[4] ) {
    pid_t pid = (pid_t) args[0];
    
    // POTENTIAL DANGER:  What if we try kill(init) or kill(idle)?
    // Might want to guard for that here!

    // kill(0) is a request to kill the calling process
    if( pid == 0 ) {
        pid = _current->pid;
    }
    
    // locate the victim
    pcb_t *pcb = _pcb_find_pid( pid );
    if( pcb == NULL ) {
        RET(_current) = E_NOT_FOUND;
        return;
    }
    
    // how we process the victim depends on its current state:
    switch( pcb->state ) {
    
        // for the first three of these states, the process is on
        // a queue somewhere; just mark it as 'Killed', and when it
        // comes off that queue via _schedule() or _dispatch() we
        // will clean it up

    case Ready:    // FALL THROUGH
    case Blocked:  // FALL THROUGH
    case Sleeping:
        pcb->state = Killed;
        // FALL THROUGH

        // for Killed, it's already been marked as 'Killed', so we
        // don't need to re-mark it
    case Killed:
        RET(_current) = E_SUCCESS;
        break;
    
        // we have met the enemy, and he is us!
    case Running:  // current process
        _force_exit( _current, Killed );
        _dispatch();
        break;
    
        // much like 'Running', except that it's not the current
        // process, so we don't have to dispatch another one
    case Waiting:
        _force_exit( pcb, Killed );
        break;
    
        // you can't kill something if it's already dead
    case Zombie:
        RET(_current) = E_NOT_FOUND;
        break;
        
    default:
        // this is a really bad potential problem - we have
        // a bogus process state, so we report that
        __sprint( b256, "*** kill(): victim %d, unknown state %d\n",
            pcb->pid, pcb->state );
        __cio_puts( b256 );

        // after reporting it, we give up
        PANIC( 0, _sys_kill );
    }
}

/**
** _sys_sleep - put the current process to sleep for some length of time
**
** implements:
**    void sleep( uint32_t msec );
*/
static void _sys_sleep( uint32_t args[4] ) {
    uint32_t ticks = MS_TO_TICKS( args[0] );

    if( ticks == 0 ) {

        // handle the case where the process is just yielding
        _schedule( _current );

    } else {

        // process is actually going to sleep - calculate wakeup time
        _current->event.wakeup = _system_time + ticks;
        _current->state = Sleeping;

        // add to the sleep queue
        if( _que_enque(_sleeping,_current,_current->event.wakeup) !=
                E_SUCCESS ) {
            // something went wrong!
            WARNING( "cannot enque(_sleeping,_current)" );
            _schedule( _current );
        }
    }

    // either way, need a new "current" process
    _dispatch();
}

/**
** _sys_spawn - create a new process
**
** implements:
**    pid_t spawn( int (*entry)(uint32_t,uint32_t),
**                 prio_t prio, uint32_t arg1, uint32_t arg2 );
*/
static void _sys_spawn( uint32_t args[4] ) {
    
    // is there room for one more process in the system?
    if( _active_procs >= N_PROCS ) {
        RET(_current) = E_NO_PROCS;
        return;
    }

    // verify that there is an entry point
    if( args[0] == NULL ) {
        RET(_current) = E_BAD_PARAM;
        return;
    }

    // and that the priority is legal
    if( args[1] > PRIO_LOWEST ) {
        RET(_current) = E_BAD_PARAM;
        return;
    }

    // create the process
    pcb_t *pcb = _proc_create( args, _next_pid++, _current->pid );
    if( pcb == NULL ) {
        RET(_current) = E_NO_MEMORY;
        return;
    }

    // the parent gets the PID of the child as its return value
    RET(_current) = pcb->pid;  // parent
    
    // schedule the child
    _schedule( pcb );
    
    // add the child to the "active process" table
    ++_active_procs;

    // find an empty process table slot
    int i;
    for( i = 0; i < N_PROCS; ++i ) {
        if( _ptable[i] == NULL ) {
            break;
        }
    }
    
    // if we didn't find one, we have a serious problem
    assert( i < N_PROCS );
    
    // add this to the table
    _ptable[i] = pcb;
}

/**
** _sys_wait - wait for a child process to terminate
**
** implements:
**    pid_t wait( int32_t *status );
*/
static void _sys_wait( uint32_t args[4] ) {
    int children = 0;
    int i;
    
    // see if this process has any children, and if so,
    // whether one of them has terminated
    for( i = 0; i < N_PROCS; ++i ) {
        if( _ptable[i] != NULL && _ptable[i]->ppid == _current->pid ) {
            ++children;
            if( _ptable[i]->state == Zombie ) {
                break;
            }
        }
    }       

    // case 1:  no children

    if( children < 1 ) {
        // return the bad news
        RET(_current) = E_NO_PROCS;
        return;
    }
    
    // case 2:  children, but none are zombies

    if( i >= N_PROCS ) {
        // block this process until one of them terminates
        _current->state = Waiting;
        _dispatch();
        return;
    }
    
    // case 3:  bingo!
    
    // return the zombie's PID
    RET(_current) = _ptable[i]->pid;
    
    // see if the parent wants the termination status
    int32_t *ptr = (int32_t *) (args[0]);
    if( ptr != NULL ) {
        // yes - return it
        // *****************************************************
        // Potential VM issue here!  This code assigns the exit
        // status into a variable in the parent's address space.  
        // This works in the baseline because we aren't using
        // any type of memory protection.  If address space
        // separation is implemented, this code will very likely
        // STOP WORKING, and will need to be fixed.
        // *****************************************************
        *ptr = _ptable[i]->exit_status;
    }
    
    // clean up the zombie now
    _pcb_cleanup( _ptable[i] );
    
    return;
}

/*
** PUBLIC FUNCTIONS
*/

/**
** Name:  _sys_init
**
** Syscall module initialization routine
**
** Dependencies:
**    Must be called after _sio_init()
*/
void _sys_init( void ) {

    __cio_puts( " Syscall:" );

    /*
    ** Set up the syscall jump table.  We do this here
    ** to ensure that the association between syscall
    ** code and function address is correct even if the
    ** codes change.
    */

    _syscalls[ SYS_exit ]     = _sys_exit;
    _syscalls[ SYS_read ]     = _sys_read;
    _syscalls[ SYS_write ]    = _sys_write;
    _syscalls[ SYS_getpid ]   = _sys_getpid;
    _syscalls[ SYS_getppid ]  = _sys_getppid;
    _syscalls[ SYS_gettime ]  = _sys_gettime;
    _syscalls[ SYS_getprio ]  = _sys_getprio;
    _syscalls[ SYS_setprio ]  = _sys_setprio;
    _syscalls[ SYS_kill ]     = _sys_kill;
    _syscalls[ SYS_sleep ]    = _sys_sleep;
    _syscalls[ SYS_spawn ]    = _sys_spawn;
    _syscalls[ SYS_wait ]     = _sys_wait;

    // install the second-stage ISR
    __install_isr( INT_VEC_SYSCALL, _sys_isr );

    // all done
    __cio_puts( " done" );
}

/**
** Name:  _force_exit
**
** Do the real work for exit() and some kill() calls
**
** @param victim   Pointer to the PCB for the exiting process
** @param state    Termination status for the process
*/
void _force_exit( pcb_t *victim, int32_t status ) {
    pid_t us = victim->pid;

    // reparent all the children of this process so that
    // when they terminate init() will collect them
    for( int i = 0; i < N_PROCS; ++i ) {
        // if (A) this is an active process, and
        //    (B) it's in a "really active" state, and
        //    (C) it's a child of this process,
        // hand it off to 'init'
        if( _ptable[i] != NULL
            && _ptable[i]->state >= Ready
            && _ptable[i]->ppid == us) {
            _ptable[i]->ppid = PID_INIT;
        }
    }

    // locate this process' parent
    pcb_t *parent = _pcb_find_pid( victim->ppid );

    // every process has a parent, even if it's 'init'
    assert( parent != NULL );
    
    if( parent->state != Waiting ) {
    
        // if the parent isn't currently waiting, turn
        // the exiting process into a zombie
        victim->state = Zombie;

        // leave it alone and unchanged for now
        return;
    }
        
    // OK, we know that the parent is currently waiting.  Waiting
    // processes, like Zombie processes, are not on an actual queue;
    // instead, they exist solely in the process table, with their
    // state indicating their condition.

    // Give the parent this child's PID
    RET(parent) = victim->pid;

    // if the parent wants it, also return this child's status
    int32_t *ptr = (int32_t *) ARG( parent, 1 );
    if( ptr != NULL ) {
        // *****************************************************
        // Potential VM issue here!  This code assigns the exit
        // status into a variable in the parent's address space.  
        // This works in the baseline because we aren't using
        // any type of memory protection.  If address space
        // separation is implemented, this code will very likely
        // STOP WORKING, and will need to be fixed.
        // *****************************************************
        *ptr = status;
    }

    // switch the parent back on to process the info we gave it
    _schedule( parent );

    // clean up this process
    _pcb_cleanup( victim );
}
