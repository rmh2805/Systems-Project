/**
** @file users.h
**
** @author CSCI-452 class of 20205
**
** "Userland" configuration information
*/

#ifndef USERS_H_
#define USERS_H_

/*
** General (C and/or assembly) definitions
**
** This section of the header file contains definitions that can be
** used in either C or assembly-language source code.
*/

// delay loop counts

#define DELAY_LONG  100000000
#define DELAY_MED     4500000
#define DELAY_SHORT   2500000

#define DELAY_STD   DELAY_SHORT

// a delay loop

#define DELAY(n)    do { \
        for(int _dlc = 0; _dlc < (DELAY_##n); ++_dlc) continue; \
    } while(0)

#ifndef SP_ASM_SRC

/*
** Start of C-only definitions
*/

//
// System call matrix
//
// System calls in this system:   exit, wait, kill, spawn, read, write,
//  sleep, gettime, getpid, getppid, getprio, setprio
//
// There is also a "bogus" system call which attempts to use an invalid
// system call code; this should be caught by the syscall handler and
// the process should be terminated.
//
// These are the system calls which are used in each of the user-level
// main functions.  Some main functions only invoke certain system calls
// when given particular command-line arguments (e.g., main6).
//
//                        baseline system calls in use
//  fcn   exit wait kill spawn read write sleep time pid ppid gpri spri bogus
// -----  ---- ---- ---- ----- ---- ----- ----- ---- --- ---- ---- ---- -----
// main1    X    .    .    .     .    X     .    .    .    .    .    .    .
// main2    .    .    .    .     .    X     .    .    .    .    .    .    .
// main3    X    .    .    .     .    X     X    .    .    .    .    .    .
// main4    X    .    .    X     .    X     X    .    X    .    .    .    .
// main5    X    .    .    X     .    X     .    .    .    .    .    .    .
// main6    X    X    X    X     .    X     .    .    .    .    .    .    .
//
// userH    X    .    .    X     .    X     X    .    .    .    .    .    .
// userI    X    X    X    X     .    X     X    .    X    .    .    .    .
// userJ    X    .    .    X     .    X     .    .    .    .    .    .    .
// userP    X    .    .    .     .    X     X    X    .    .    .    .    .
// userQ    X    .    .    .     .    X     .    .    .    .    .    .    X
// userR    X    .    .    .     X    X     X    .    .    .    .    .    .
// userS    X    .    .    .     .    X     X    .    .    .    .    .    .
// userT    X    X    .    X     .    X     X    .    X    .    .    .    .
// userU    X    X    .    X     .    X     X    .    X    .    .    .    .
// userV    X    .    .    .     .    X     X    .    .    .    X    X    .
// userW    X    .    .    .     .    X     X    X    X    .    .    .    .
// userX    X    .    .    .     .    X     .    .    .    .    .    .    .
// userY    X    .    .    .     .    X     X    .    .    .    .    .    .
// userZ    X    .    .    .     .    X     X    .    X    X    .    .    .

/*
** User process controls.
**
** The comment field of these definitions contains a brief description
** of the functionality of each user process.
**
** To spawn a specific user process from the initial process, uncomment
** its entry in this list.
*/

//
// Generally, most of these will exit with a status of 0.  If a process
// returns from its main function when it shouldn't (e.g., if it had
// called exit() but continued to run), it will usually return a status
// of 42.
//
#define SPAWN_A
#define SPAWN_B
#define SPAWN_C
#define SPAWN_D
#define SPAWN_E
#define SPAWN_F
#define SPAWN_G
#define SPAWN_H
#define SPAWN_I
#define SPAWN_J
#define SPAWN_K
#define SPAWN_L
#define SPAWN_M
#define SPAWN_N
// no user O
#define SPAWN_P
#define SPAWN_Q
#define SPAWN_R
#define SPAWN_S
#define SPAWN_T
#define SPAWN_U
#define SPAWN_V

//
// Users W-Z are spawned from other processes; they
// should never be spawned directly by init().
//

/*
** Prototypes for externally-visible routines
*/

/**
** init - initial user process
**
** Spawns the other user processes, then loops forever calling wait()
**
** Invoked as:  init
*/
int32_t init( uint32_t arg1, uint32_t arg2 );

/**
** idle - the idle process
**
** Reports itself, then loops forever delaying and printing a character.
**
** Invoked as:  idle
*/
int32_t idle( uint32_t arg1, uint32_t arg2 );

#endif
/* SP_ASM_SRC */

#endif
