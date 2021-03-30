/*
** @file queues.h
**
** @author CSCI-452 class of 20205
**
** Queue module declarations
*/

#ifndef QUEUES_H_
#define QUEUES_H_

/*
** General (C and/or assembly) definitions
*/

#ifndef SP_ASM_SRC

/*
** Start of C-only definitions
*/

/*
** Types
*/

/*
** Our queue structure.  The queue is an opaque type, so the outside
** world only sees it as a "thing".  The basic queue_t type is a
** pointer to the internal queue structure.
*/

typedef struct queue_s *queue_t;

/*
** Globals
*/

/*
** Prototypes
*/

/**
** _que_init() - initialize the queue module
**
** Allocates the initial set of qnodes and set of queues.
**
** Dependencies:
**    Cannot be called before kmem is initialized
**    Must be called before any queue manipulation can be done
*/
void _que_init( void );

/**
** _que_alloc() - allocate a queue
**
** Allocates a queue structure and returns it to the caller.
**
** @param order   The ordering function to be used, or NULL
**
** @return a pointer to the allocated queue, or NULL
*/
queue_t _que_alloc( int (*order)(const void *,const void *) );

/**
** _que_free() - return a queue to the free list
**
** Deallocates the supplied queue
**
** @param q   The queue to be put on the free list
*/
void _que_free( queue_t q );

/**
** _que_length() - return the count of elements in a queue
**
** @param q   The queue to be checked
*/
uint_t _que_length( queue_t q );

/**
** _que_enque() - add an element to a queue
**
** @param q     The queue to be manipulated
** @param data  The data to be added
** @param key   The key value to be used when ordering the queue
**
** @return the status of the insertion attempt
*/
status_t _que_enque( queue_t q, void *data, uint32_t key );

/**
** _que_deque() - remove an element from a queue
**
** @param q     The queue to be manipulated
**
** @return the removed element, or NULL
*/
void *_que_deque( queue_t q );

/**
** _que_peek() - peek at the first element in a queue
**
** @param q   The queue to be checked
**
** @return the data pointer from the first node in the queue, or NULL
**         if the queue is empty
*/
void *_que_peek( queue_t q );

/*
** Debugging/tracing routines
*/

/**
** _que_dump(msg,que)
**
** dump the contents of the specified queue to the console
**
** @param msg  Optional message to print
** @param q    Queue to dump
*/
void _que_dump( const char *msg, queue_t q );

#endif

#endif
