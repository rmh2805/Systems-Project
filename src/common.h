/**
** @file common.h
**
** @author Warren R. Carithers
** @author CSCI-452 class of 20205
**
** Common definitions for the baseline system.
**
** This header file pulls in the standard header information
** needed by all parts of the system (OS and user levels).
**
** Things which are kernel-specific go in the kdefs.h file;
** things which are user-specific go in the udefs.h file.
** The appropriate 'defs' file is included here based on the
** SP_KERNEL_SRC macro.
*/

#ifndef COMMON_H_
#define COMMON_H_

/*
** General (C and/or assembly) definitions
*/

// NULL pointer value
//
// we define this the traditional way so that
// it's usable from both C and assembly

#define NULL    0

// predefined I/O channels

#define	CHAN_CONS	0
#define	CHAN_SIO	1

// maximum number of processes in the system

#define N_PROCS     25

#ifndef SP_ASM_SRC

/*
** Start of C-only definitions
*/

// halves of various data sizes

#define UI16_UPPER  0xff00
#define UI16_LOWER  0x00ff

#define UI32_UPPER  0xffff0000
#define UI32_LOWER  0x0000ffff

#define UI64_UPPER  0xffffffff00000000LL
#define UI64_LOWER  0x00000000ffffffffLL

// Simple conversion pseudo-functions usable by everyone

// convert seconds to ms

#define	SEC_TO_MS(n)		((n) * 1000)

/*
** Types
*/

// standard integer sized types

typedef char                    int8_t;
typedef unsigned char           uint8_t;
typedef short                   int16_t;
typedef unsigned short          uint16_t;
typedef int                     int32_t;
typedef unsigned int            uint32_t;
typedef long long int           int64_t;
typedef unsigned long long int  uint64_t;

// generic types
typedef unsigned int            uint_t;

// a Boolean type and its values
typedef uint8_t bool_t;

#define true    1
#define false   0

// PID data type
typedef int16_t pid_t;

// Process state and state values
//
// (defined here in case user-level code needs to know)
typedef uint8_t state_t;

enum state_e {
    Unused = 0,    // for easy detection
    New, Ready, Running, Sleeping, Waiting, Blocked, Killed,
    Zombie         // should be the last entry
};

// Process priority type and values
typedef uint8_t prio_t;

enum prio_e {

    // MUST BE FIRST - HIGHEST PRIORITY
    System,          // OS-related processes

    // User-level priorities
    User1, User2,

    // MUST BE LAST - LOWEST PRIORITY
    Deferred         // things that run only when nothing else is ready
};

// delimiters for priority values
#define PRIO_HIGHEST  System
#define PRIO_STD      User1
#define PRIO_LOWEST   Deferred

// number of scheduling categories
#define N_QUEUES   (PRIO_LOWEST + 1)

// Generic "event" type
typedef union event_u {
    uint32_t wakeup;      // wakeup time for sleeping processes
    void *other;          // generic data
} event_t;

// system time
typedef uint32_t time_t;

// status return type
typedef int status_t;

// Error return values (e.g., from system calls)

#define E_SUCCESS       (0)
#define SUCCESS         E_SUCCESS

#define E_FAILURE       (-1)
#define E_NO_QNODES     (-2)
#define E_BAD_SYSCALL   (-3)
#define E_BAD_CHANNEL   (-4)
#define E_NO_DATA       (-5)
#define E_BAD_PARAM     (-6)
#define E_NO_MEMORY     (-7)
#define E_NO_PROCS      (-8)
#define E_NOT_FOUND     (-9)
#define E_NO_CHILDREN   (-10)

/*
** Additional OS-only or user-only things
*/

#ifdef SP_KERNEL_SRC
#include "kdefs.h"
#else
#include "udefs.h"
#endif

// end of SP_ASM_SRC section
#endif

#endif
