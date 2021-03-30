/**
** @file kdefs.h
**
** @author Numerous CSCI-452 classes
**
** Kernel-only definitions for the baseline system.
**
*/

#ifndef KDEFS_H_
#define KDEFS_H_

// The OS needs the standard system headers

#include "cio.h"
#include "kmem.h"
#include "support.h"
#include "kernel.h"

// The OS also needs the kernel library.

#include "klib.h"

#ifndef SP_ASM_SRC

/*
** Start of C-only definitions
*/

// bit patterns for modulus checking of (e.g.) sizes and addresses

#define MOD4_BITS        0x3
#define MOD4_MASK        0xfffffffc

#define MOD16_BITS       0xf
#define MOD16_MASK       0xfffffff0

// Debugging and sanity-checking macros

// Warning messages to the console

#define WARNING(m)  do { \
        __cio_printf( "WARN %s (%s @ %s): ", __func__, __FILE__, __LINE__ ); \
        __cio_puts( m ); \
        __cio_putchar( '\n' ); \
    } while(0)

// Panic messages to the console

#define PANIC(n,x)  do { \
        __sprint( b512, "ASSERT %s (%s @ %s), %d: ", \
                  __func__, __FILE__, __LINE__, n ); \
        _kpanic( b512, # x ); \
    } while(0)

// Always-active assertions

#define assert(x)   if( !(x) ) { PANIC(0,x); }

// Conditional assertions are categorized by the "sanity level"
// being used in this compilation; each only triggers a fault
// if the sanity level is at or above a specific value.  This
// allows selective enabling/disabling of debugging checks.
//
// The sanity level is set during compilation with the CPP macro
// "SANITY".  A sanity level of 0 disables these assertions.

#ifndef SANITY
// default sanity check level: check everything!
#define SANITY  9999
#endif

// only provide these macros if the sanity check level is positive

#if SANITY > 0

#define assert1(x)  if( SANITY >= 1 && !(x) ) { PANIC(1,x); }
#define assert2(x)  if( SANITY >= 2 && !(x) ) { PANIC(2,x); }
#define assert3(x)  if( SANITY >= 3 && !(x) ) { PANIC(3,x); }
#define assert4(x)  if( SANITY >= 4 && !(x) ) { PANIC(4,x); }

#else

#define assert1(x)  // do nothing
#define assert2(x)  // do nothing
#define assert3(x)  // do nothing
#define assert4(x)  // do nothing

#endif

#endif

#endif
