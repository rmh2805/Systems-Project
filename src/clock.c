/**
** @file clock.c
**
** @author CSCI-452 class of 20205
**
** Clock module implementation
*/

#define	SP_KERNEL_SRC

#include "x86arch.h"
#include "x86pic.h"
#include "x86pit.h"

#include "common.h"

#include "clock.h"
#include "process.h"
#include "queues.h"
#include "scheduler.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

// pinwheel control variables
static uint32_t _pinwheel;   // pinwheel counter
static uint32_t _pindex;     // index into pinwheel string

/*
** PUBLIC GLOBAL VARIABLES
*/

// current system time
time_t _system_time;

// we own the sleep queue
queue_t _sleeping;

/*
** PRIVATE FUNCTIONS
*/

/**
** Name:  _cmp_wakeup
**
** Ordering function for the sleep queue
**
** @param v1    First key value to examine
** @param v2    Second key value to examine
**
** @return Relationship between the key values:
**      < 0   v1 < v2
**      = 0   v1 == v2
**      > 0   v1 > v2
*/
static int _cmp_wakeup( const void *v1, const void *v2 ) {

    // QUESTION:  does it make more sense to put this function
    // somewhere else with a name like "_cmp_uint32", which would
    // make it more generic and thus usable for other ordered
    // queues which sort in ascending order by uint32_t values?

    // we just do direct comparisons between the void* values,
    // because pointers are essentially unsigned integers in IA32

	if( v1 < v2 )
        return( -1 );
	else if( v1 == v2 )
        return( 0 );
	else
        return( 1 );
}

/**
** Name:  _clk_isr
**
** The ISR for the clock
**
** @param vector    Vector number for the clock interrupt
** @param code      Error code (0 for this interrupt)
*/
static void _clk_isr( int vector, int code ) {

	// spin the pinwheel
	
    ++_pinwheel;
    if( _pinwheel == (CLOCK_FREQUENCY / 10) ) {
        _pinwheel = 0;
        ++_pindex;
        __cio_putchar_at( 0, 0, "|/-\\"[ _pindex & 3 ] );
    }

#if defined(STATUS)
    // Periodically, dump the queue lengths and the SIO status (along
    // with the SIO buffers, if non-empty).
    //
    // Define the symbol STATUS with a value equal to the desired
    // reporting frequency, in seconds.

    if( (_system_time % SEC_TO_TICKS(STATUS)) == 0 ) {
        __cio_printf_at( 3, 0,
            "%3d procs:  sl/%d wt/%d rd/%d zo/%d  r %d %d %d %d    ",
                _active_procs,
                _que_length(_sleeping), _que_length(_waiting),
                _que_length(_reading), _que_length(_zombie),
                _que_length(_ready[0]), _que_length(_ready[1]),
                _que_length(_ready[2]), _que_length(_ready[3])
        );
        _sio_dump( true );
        // _active_dump( "Ptbl", false );
    }
#endif

    // time marches on!
	++_system_time;
	
    // wake up an sleeping processes whose time has come
    //
    // we give them preference over the current process
    // (when it is scheduled again)

    // peek at the first element on the sleep queue
	pcb_t *tmp = _que_peek( _sleeping );

    // loop as long as there is something on the queue AND
    // it's (past?) time for that process to wake up
	while( tmp != NULL && tmp->event.wakeup <= _system_time ) {
		tmp = _que_deque( _sleeping );
		_schedule( tmp );
		tmp = _que_peek( _sleeping );
	}
	
    // check the current process to see if its time slice has expired
	_current->ticks -= 1;

	if( _current->ticks < 1 ) {
        // yes!  put it back on the ready queue
		_schedule( _current );
        // pick a new "current" process
		_dispatch();
	}
	
    // tell the PIC we're done
	__outb( PIC_PRI_CMD_PORT, PIC_EOI );
}

/*
** PUBLIC FUNCTIONS
*/

/**
** Name:  _clk_init
**
** Initializes the clock module
**
*/
void _clk_init( void ) {

	__cio_puts( " Clock:" );

    // start the pinwheel
    _pinwheel = (CLOCK_FREQUENCY / 10) - 1;
    _pindex = 0;

    // return to the dawn of time
	_system_time = 0;
	
	// configure the clock
    uint32_t divisor = TIMER_FREQUENCY / CLOCK_FREQUENCY;
    __outb( TIMER_CONTROL_PORT, TIMER_0_LOAD | TIMER_0_SQUARE );
    __outb( TIMER_0_PORT, divisor & 0xff );        // LSB of divisor
    __outb( TIMER_0_PORT, (divisor >> 8) & 0xff ); // MSB of divisor
	
    // create the sleep queue
	_sleeping = _que_alloc( _cmp_wakeup );
	assert( _sleeping != NULL );
	
    // register the second-stage ISR
	__install_isr( INT_VEC_TIMER, _clk_isr );
	
    // report that we're all set
	__cio_puts( " done" );
}
