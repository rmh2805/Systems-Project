/**
** @file queues.c
**
** @author  CSCI-452 class of 20205
**
** Queue module implementation
*/

#define SP_KERNEL_SRC

#include "common.h"

#include "queues.h"

/*
** PRIVATE DEFINITIONS
*/

// shorthand form of queue length call
#define QLEN(q)    ((q)->length)
// alternate version that actually invokes the function
// #define QLEN(q)    _que_length(q)

/*
** PRIVATE DATA TYPES
*/

/*
** Queue organization
** ------------------
** Our queues are self-ordering, generic queues.  A queue can contain
** any type of data.  This is accomplished through the use of intermediate
** nodes called qnodes, which contain a void* data member, allowing them
** to point to any type of integral data (integers, pointers, etc.).
** The qnode list is doubly-linked for ease of traversal.
**
** Each queue has associated with it a comparison function, which may be
** NULL.  Insertions into a Queue are handled according to this function.
** If the function pointer is NULL, the queue is FIFO, and the insertion
** is always done at the end of the queue.  Otherwise, the insertion is
** ordered according to the results from the comparison function.
**
** Neither of these types are visible to the rest of the system.  The
** queue_t type is a pointer to the queue_s struct.
*/

// queue nodes
typedef struct qn_s {
    struct qn_s *prev;   // link to previous node
    struct qn_s *next;   // link to next node
    uint_t key;          // key to whatever's in this entry
    void *data;          // what's in this entry
} qnode_t;

// the queue itself is a pointer to this structure
struct queue_s {
    qnode_t *head;       // first element
    qnode_t *tail;       // last element
    uint_t length;       // current occupancy count
    int (*order)( const void *, const void * ); // how to compare entries
};

/*
** PRIVATE GLOBAL VARIABLES
*/

/*
** The list of free qnodes.
**
** Organized as a singly-linked list using the 'next' pointer
** in the qnode structure.
*/
static qnode_t *_free_qnodes;

/*
** The list of free queues.
**
** Organized as a singly-linked list using the 'data' pointer
** in the queue_s structure.
*/
static queue_t _free_queues;

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** PRIVATE FUNCTIONS
*/

// forward references
static void _qn_free( qnode_t * );
void _que_free( queue_t );

/**
** _new_qnodes() - allocate a slice and carve it into qnodes
**
** @param critical  Should we panic on allocation failure?
**
** @return true on success, else false
*/
static bool_t _new_qnodes( bool_t critical ) {
    qnode_t *slice;

    // start by carving off a slice of memory
    slice = (qnode_t *) _km_slice_alloc();

    // if this is a critical allocation and slice is NULL, we're done
    assert( !critical || slice != NULL );

    // whew - not critical; NULL slice is still a problem, though
    if( slice == NULL ) {
        return( false );
    }

    // clear out the allocated space
    __memclr( slice, SLICE_SIZE );

    // free the qnodes!
    for( int i = 0; i < (SLICE_SIZE / sizeof(qnode_t)); ++i ) {
        // N.B.: if the slice size is not an integral multiple of
        // the qnode size, this will leave a small internal fragment
        // at the end of the allocated block of memory
        _qn_free( slice + i );
    }

    // all done!
    return( true );
}

/**
** _new_queues() - allocate a slice and carve it into queues
**
** @param critical  Should we panic on allocation failure?
**
** @return true on success, else false
*/
static bool_t _new_queues( bool_t critical ) {
    struct queue_s *slice;

    // start by carving off a slice of memory
    slice = (struct queue_s *) _km_slice_alloc();

    // if this is a critical allocation and slice is NULL, we're done
    assert( !critical || slice != NULL );

    // whew - not critical; NULL slice is still a problem, though
    if( slice == NULL ) {
        return( false );
    }

    // clear out the allocated space
    __memclr( slice, SLICE_SIZE );

    // 
    for( int i = 0; i < (SLICE_SIZE / sizeof(struct queue_s)); ++i ) {
        _que_free( slice + i );
    }

    // all done!
    return( true );
}

/**
** _qn_alloc() - allocate a qnode
**
** Removes the first qnode from the free list.
**
** @return A pointer to the allocated node, or NULL
*/
static qnode_t *_qn_alloc( void ) {
    qnode_t *new;

    // see if there is an available node
    if( _free_qnodes == NULL ) {

        // no - see if we can create some
        if( !_new_qnodes(false) ) {
            // no!  let's just leave quietly
            return( NULL );
        }
    }

    // OK, we know that there is at least one free qnode;
    // just take the first one from the list

    new = _free_qnodes;
    _free_qnodes = new->next;

    // clear out the fields in this one just to be safe
    new->prev = new->next = new->data = NULL;
    new->key = 0;

    // pass it back to the caller
    return( new );
}

/**
** _qn_free() - return a qnode to the free list
**
** Deallocates the supplied qnode
**
** @param qn   The qnode to be put on the free list
*/
static void _qn_free( qnode_t *qn ) {

    // just stick this one at the front of the list
    qn->next = _free_qnodes;
    _free_qnodes = qn;
}

/*
** PUBLIC FUNCTIONS
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
void _que_init( void ) {

    __cio_puts( " Queue:" );

    // start with the qnodes
    _free_qnodes = NULL;
    _new_qnodes( true );

    // next, the queues
    _free_queues = NULL;
    _new_queues( true );

    // all done!
    __cio_puts( " done" );
}

/**
** _que_alloc() - allocate a queue
**
** Allocates a queue structure and returns it to the caller.
**
** @param order   The ordering function to be used, or NULL
**
** @return a pointer to the allocated queue, or NULL
*/
queue_t _que_alloc( int (*order)(const void *,const void *) ) {
    queue_t new;

    // see if there is an available node
    if( _free_queues == NULL ) {

        // no - see if we can create some
        if( !_new_queues(false) ) {
            // no!  let's just leave quietly
            return( NULL );
        }
    }

    // OK, we know that there is at least one free qnode;
    // just take the first one from the list

    new = _free_queues;
    _free_queues = (queue_t) new->head;

    // clear out the fields in this one just to be safe
    new->head = new->tail = NULL;
    new->length = 0;
    new->order = order;

    // pass it back to the caller
    return( new );
}

/**
** _que_free() - return a queue to the free list
**
** Deallocates the supplied queue
**
** @param q   The queue to be put on the free list
*/
void _que_free( queue_t q ) {

    // sanity check!
    assert1( q != NULL );

    // just stick this one at the front of the list
    q->head = (qnode_t *) _free_queues;
    _free_queues = q;
}

/**
** _que_length() - return the count of elements in a queue
**
** @param q   The queue to be checked
*/
uint_t _que_length( queue_t q ) {

    // sanity check!
    assert1( q != NULL );

    // this one's easy
    return( q->length );
}

/**
** _que_enque() - add an element to a queue
**
** @param q     The queue to be manipulated
** @param data  The data to be added
** @param key   The key value to be used when ordering the queue
**
** @return the status of the insertion attempt
*/
status_t _que_enque( queue_t q, void *data, uint_t key ) {

    // sanity check!
    assert1( q != NULL );

    // need to use a qnode
    qnode_t *qn = _qn_alloc();
    if( qn == NULL ) {
        return( E_NO_QNODES );
    }

    // fill in the node
    qn->data = data;
    qn->key = key;

    /*
    ** Insert the data.
    **
    ** The simplest case is insertion into an empty queue.
    */

    if( QLEN(q) == 0 ) {
        // first, last, and only element
        q->head = q->tail = qn;
        q->length = 1;
        return( E_SUCCESS );
    }

    // next simplest is an un-ordered queue

    if( q->order == NULL ) {
        // just add at the end
        qn->prev = q->tail;     // predecessor is (old) last node
        q->tail->next = qn;     // new is successor to (old) last node
        q->tail = qn;           // new is now the last node
        q->length += 1;         // one more in the list
        return( E_SUCCESS );
    }

    /*
    ** Insertion into a non-empty, ordered list.
    **
    ** Start by traversing the list looking for the node
    ** that will come after the node we're inserting.
    */

    qnode_t *curr = q->head;

    while( curr != NULL && q->order((void *)key,(void *)(curr->key)) >= 0 ) {
        curr = curr->next;
    }

    /*
    ** We now know the successor of the node we're inserting.
    */

    qn->next = curr;    // correct even if curr is NULL

    if( curr == NULL ) {

        // if curr is NULL, we're adding at the end
        qn->prev = q->tail;     // predecessor is (old) last node
        q->tail->next = qn;     // new is successor to (old) last node
        q->tail = qn;           // new is now the last node

    } else {

        // adding before the end; set the predecessor pointer
        qn->prev = curr->prev;

        // if curr is the first node, this is the new head node
        if( curr->prev == NULL ) {
            // first!
            q->head = qn;
        } else {
            // adding to the middle of the list
            qn->prev->next = qn;
        }

        // finally, point our successor back to us
        curr->prev = qn;
    }

    q->length += 1;

    return( E_SUCCESS );
}

/**
** _que_deque() - remove an element from a queue
**
** @param q     The queue to be manipulated
**
** @return the removed element, or NULL
*/
void *_que_deque( queue_t q ) {

    // sanity check!
    assert1( q != NULL );

    // can't remove anything from an empty queue
    if( QLEN(q) == 0 ) {
        return( NULL );
    }

    // OK, we have something to return; take it from the queue
    qnode_t *qn = q->head;

    // save the data value
    void *data = qn->data;

    // unlink the qnode from the list
    q->head = qn->next;
    q->length -= 1;

    // was this the only node in the list?
    if( q->head != NULL ) {
        // no - unlink from the successor
        q->head->prev = NULL;
    } else {
        // yes, so now there's no last element
        q->tail = NULL;
    }

    // return the qnode for later re-use
    _qn_free( qn );

    // send the result back to the caller
    return( data );
}

/**
** _que_peek() - peek at the first element in a queue
**
** @param q   The queue to be checked
**
** @return the data pointer from the first node in the queue, or NULL
**         if the queue is empty
*/
void *_que_peek( queue_t q ) {

    // sanity check!
    assert1( q != NULL );

    if( QLEN(q) == 0 ) {
        return( NULL );
    }

    return( q->head->data );
}

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
void _que_dump( const char *msg, queue_t q ) {

    // report on this queue
    __cio_printf( "%s: ", msg );
    if( q == NULL ) {
        __cio_puts( "NULL???\n" );
        return;
    }

    // first, the basic data
    __cio_printf( "head %08x tail %08x %d items",
                  (uint32_t) q->head, (uint32_t) q->tail, q->length );

    // next, how the queue is ordered
    if( q->order ) {
        __cio_printf( " order %08x\n", (uint32_t) q->order );
    } else {
        __cio_puts( " FIFO\n" );
    }

    // if there are members in the queue, dump the first five data pointers
    if( q->length > 0 ) {
        __cio_puts( " data: " );
        qnode_t *tmp;
        int i = 0;
        for( tmp = q->head; i < 5 && tmp != NULL; ++i, tmp = tmp->next ) {
            __cio_printf( " [%08x]", (uint32_t) tmp->data );
        }

        if( tmp != NULL ) {
            __cio_puts( " ..." );
        }

        __cio_putchar( '\n' );
    }
}
