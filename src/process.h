/*
** @file process.h
**
** @author CSCI-452 class of 20205
**
** Process module declarations
*/

#ifndef PROCESS_H_
#define PROCESS_H_

/*
** General (C and/or assembly) definitions
*/

#ifndef SP_ASM_SRC

/*
** Start of C-only definitions
*/

#include "common.h"

#include "stacks.h"
#include "queues.h"

// PID for the init() process
#define PID_INIT     1

// PID for the idle() process
#define PID_IDLE     2

// REG(pcb,x) -- access a specific register in a process context

#define REG(pcb,x)  ((pcb)->context->x)

// RET(pcb) -- access return value register in a process context

#define RET(pcb)    ((pcb)->context->eax)

// ARG(pcb,n) -- access argument #n from the indicated process
//
// ARG(pcb,0) --> return address
// ARG(pcb,1) --> first parameter
// ARG(pcb,2) --> second parameter
// etc.
//
// ASSUMES THE STANDARD 32-BIT ABI, WITH PARAMETERS PUSHED ONTO THE
// STACK.  IF THE PARAMETER PASSING MECHANISM CHANGES, SO MUST THIS!

#define ARG(pcb,n)  ( ( (uint32_t *) (((pcb)->context) + 1) ) [(n)] )

/*
** Types
*/

// process states are defined in common.h so that
// they're available in userland

// process context structure
//
// NOTE:  the order of data members here depends on the
// register save code in isr_stubs.S!!!!

typedef struct context {
    uint32_t ss;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t vector;
    uint32_t code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} context_t;

// the process control block
//
// fields are ordered by size to avoid padding
//
// ideally, its size should divide evenly into 1024 bytes;
// currently, 32 bytes

typedef struct pcb_s {
    // four-byte values

    // Start with these eight bytes, for easy access in assembly
    context_t *context;     // pointer to context save area on stack
    stack_t *stack;         // pointer to process stack

    int32_t exit_status;    // termination status, for parent's use
    event_t event;          // what this process is waiting for

    // two-byte values
    pid_t pid;              // unique PID for this process
    pid_t ppid;             // PID of the parent

    // one-byte values
    state_t state;          // current state (see common.h)
    prio_t priority;        // process priority (MLQ queue level)

    uint8_t quantum;        // quantum for this process
    uint8_t ticks;          // ticks remaining in current slice

    // filler, to round us up to 32 bytes
    // adjust this as fields are added/removed/changed
    uint8_t filler[8];

} pcb_t;

/*
** Globals
*/

// next available PID
extern pid_t _next_pid;

// active process count
extern uint32_t _active_procs;

// table of active processes
extern pcb_t *_ptable[N_PROCS];

/*
** Prototypes
*/

/*
** PCB manipulation
*/

/**
** _pcb_alloc() - allocate a PCB
**
** @return pointer to a "clean" PCB, or NULL
*/
pcb_t *_pcb_alloc( void );

/**
** _pcb_free() - free a PCB
**
** @param p   The PCB to be returned to the free list
*/
void _pcb_free( pcb_t *p );

/**
** _pcb_find_pid(pid)
**
** Locate the PCB for the specified PID
**
** @param pid   The PID to look for
**
** @return A pointer to the relevant PCB, or NULL
*/
pcb_t *_pcb_find_pid( pid_t pid );

/**
** _pcb_cleanup(pcb)
**
** Reclaim a process' data structures
**
** @param pcb   The PCB to reclaim
*/
void _pcb_cleanup( pcb_t *pcb );

/*
** Process management/control
*/

/**
** _proc_init() - initialize the process module
**
** Allocates the initial set of PCBs, does whatever else is
** needed to make it possible to create processes
**
** Dependencies:
**    Cannot be called before kmem is initialized
**    Must be called before any process creation can be done
*/
void _proc_init( void );

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
pcb_t *_proc_create( uint32_t args[4], pid_t pid, pid_t ppid );

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
void _pcb_dump( const char *msg, register pcb_t *p );

/**
** _context_dump(msg,context)
**
** Dumps the contents of this process context to the console
**
** @param msg   An optional message to print before the dump
** @param c     The context to dump out
*/
void _context_dump( const char *msg, register context_t *c );

/**
** _context_dump_all(msg)
**
** dump the process context for all active processes
**
** @param msg  Optional message to print
*/
void _context_dump_all( const char *msg );

/**
** _active_dump(msg,all)
**
** dump the contents of the "active processes" table
**
** @param msg  Optional message to print
** @param all  Dump all or only part of the relevant data
*/
void _active_dump( const char *msg, bool_t all );

#endif

#endif
