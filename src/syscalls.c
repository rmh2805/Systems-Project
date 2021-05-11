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

#include "fs.h"
#include "kfs.h"

// copied from ulib.h
extern void exit_helper( void );

/*
** PRIVATE DEFINITIONS
*/
static int _sys_seekFile( const char* path, inode_id_t * currentDir);

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
        if(chan >= 2 + MAX_OPEN_FILES) { // Channel beyond potential for files
            RET(_current) = E_BAD_CHANNEL;
            return;
        }

        // File channel
        fd_t * fd = &_current->files[chan - 2];
        if(fd->inode_id.devID == 0 && fd->inode_id.idx == 0) {    // Blank fd_t
            RET(_current) = E_BAD_CHANNEL;  // Can't read from a blank fd
            return;
        }

        // Check that you have read permissions
        bool_t canRead;
        int ret = _fs_getPermission(fd->inode_id, _current->uid, _current->gid, &canRead, NULL, NULL);
        if(ret < 0) {
            __cio_printf("*ERROR* in _sys_read: Failed to read file %d.%d's permissions (%d)\n", fd->inode_id.devID, fd->inode_id.idx, ret);
            RET(_current) = E_NO_PERMISSION;
            return;
        } else if(!canRead) {
            __cio_printf("*ERROR* in _sys_read: No read permission on file %d.%d\n", fd->inode_id.devID, fd->inode_id.idx);
            RET(_current) = E_NO_PERMISSION;
            return;
        }

        n = _fs_read(fd, buf, length); // Read from file
    }

    // if there was data (or EOF), return the byte count to the process;
    // otherwise, block the process until data is available
    if( n > 0 || n == E_EOF) {

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
    int ret = 0;

    // this is almost insanely simple, but it does separate the
    // low-level device access fromm the higher-level syscall implementation

    switch( chan ) {
    case CHAN_CONS:
        __cio_write( buf, length );
        ret = length;
        break;

    case CHAN_SIO:
        ret = _sio_write( buf, length );
        break;

    default:
        if(chan >= 2 + MAX_OPEN_FILES) { // Channel beyond potential for files
            RET(_current) = E_BAD_CHANNEL;
            return;
        }

        // File channel
        fd_t * fd = &_current->files[chan - 2];
        if(fd->inode_id.devID == 0 && fd->inode_id.idx == 0) {    // Blank fd_t
            RET(_current) = E_BAD_CHANNEL;  // Can't write to a blank fd
            return;
        }
        
        // Check that you have read permissions
        bool_t canWrite;
        ret = _fs_getPermission(fd->inode_id, _current->uid, _current->gid, NULL, &canWrite, NULL);
        if(ret < 0) {
            __cio_printf("*ERROR* in _sys_write: Failed to read file %d.%d's permissions (%d)\n", fd->inode_id.devID, fd->inode_id.idx, ret);
            RET(_current) = E_NO_PERMISSION;
            return;
        } else if(!canWrite) {
            __cio_printf("*ERROR* in _sys_read: No write permission on file %d.%d\n", fd->inode_id.devID, fd->inode_id.idx);
            RET(_current) = E_NO_PERMISSION;
            return;
        }

        ret = _fs_write(fd, buf, length); // write to file
    }

    RET(_current) = ret; // Return proper status
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
    
    // Can only kill your own procs (unless root or sudo)
    if(!(_current->uid == UID_ROOT || _current->gid == GID_SUDO) && _current->uid != pcb->uid) {
        RET(_current) = E_NO_PERMISSION;
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
    pcb_t *pcb = _proc_create( args, _next_pid++, _current->pid, 
                                _current->uid, _current->gid, _current->wDir);
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

/**
** _sys_getuid - retrieves the uid of this process
** 
** implements:
**    uid_t getuid( void );
*/
static void _sys_getuid ( uint32_t args[4] ) {
    RET(_current) = _current->uid;
}

/**
** _sys_getgid - retrieves the gid of the current process
** 
** implements:
**    gid_t getgid( void );
*/
static void _sys_getgid ( uint32_t args[4] ) {
    RET(_current) = _current->gid;
}

/**
** _sys_setuid - attempts to modify the uid of the current process
** 
** implements:
**    uint32_t setuid( uid_t uid );
*/
static void _sys_setuid ( uint32_t args[4] ) {
    uid_t uid = args[0];
    
    if (_current->uid == uid) { // Report success for same user
        RET(_current) = E_SUCCESS;
    } else if (_current->uid != UID_ROOT && _current->gid != GID_SUDO) { // Return no permissions if non-root user & not sudoing
        RET(_current) = E_NO_PERMISSION;
    } else { // Otherwise update uid, set default gid, and return success
        _current->uid = uid;
        _current->gid = GID_USER;
        RET(_current) = E_SUCCESS;
    }
}

/**
 * _sys_fGetLn - Helper to grab a file's next line
 * 
 * @param fd The file descriptor to read from
 * @param buf The buffer to read into
 * @param bufLen The length of the buffer
 * 
 * @return The number of bytes read (<0 on error, E_EOF on EOF)
 */
static int _sys_fGetLn(fd_t* fd, char * buf, int bufLen) {
    int ret, nRead;

    for(nRead = 0; nRead < bufLen - 1;) {
        ret = _fs_read(fd, buf + nRead, 1);
        if(ret == 0 || ret == E_EOF) {
            if(nRead > 0) {
                break;
            }

            buf[nRead] = 0;
            return E_EOF;
        } else if (ret < 0) {
            __cio_printf("*ERROR* in _sys_fGetLn: Failed to read from file (%d)\n", ret);
            return E_FAILURE;
        }

        switch(buf[nRead]) {
            case '\b':
            case '\r':
            case 0:
                continue;
            case '\n':
            default:
                break;
        }
        if(buf[nRead] == '\n') {
            break;
        }

        nRead++;
    }
    buf[nRead] = 0;
    return nRead;
}

/**
** _sys_setgid - attempts to modify the gid of the current process
** 
** implements:
**    uint32_t setgid( gid_t gid );
*/
static void _sys_setgid ( uint32_t args[4] ) {
    const int bufSize = 128;

    gid_t gid = args[0];
    gid_t fGid;
    int ret;

    char buf[bufSize];
    fd_t fd = {{0, 0}, 0};

    // If this is the user's or the open gid perform the change and return success
    if (gid == GID_USER || gid == GID_OPEN) {
        _current->gid = gid;
        RET(_current) = E_SUCCESS;
        return;
    } 

    // Create an FD for the groups file
    ret = _sys_seekFile("/.groups", &(fd.inode_id));
    if(ret < 0) {
        __cio_printf("*ERROR* in _sys_setgid: Failed to seek group file (%d)\n", ret);
        RET(_current) = E_NOT_FOUND;
        return;
    }
    
    // Read the file line by line to look for a matching entry
    while(true) {
        char * dataPtr = buf;

        ret = _sys_fGetLn(&fd, buf, bufSize);
        if(ret == E_EOF) {
            break;
        } else if (ret < 0) {
            __cio_printf("*ERROR* in _sys_setgid: Failed to read line from group file (%d)\n", ret);
            RET(_current) = E_FAILURE;
            return;
        }

        //Skip the name to get to the gid
        while(*(dataPtr) != ':' && *(dataPtr)) {
            dataPtr++;
        }
        dataPtr++;

        // Decode the referenced gid
        fGid = 0;
        while(*(dataPtr) != ':' && *(dataPtr)) {
            fGid = 10 * fGid + *(dataPtr) - '0';
            dataPtr++;
        }
        dataPtr++;

        // Try next line if wrong GID
        if(fGid != gid) {
            continue;
        }

        // Try to match current uid to the group's user list
        bool_t matching = (_current->uid == UID_ROOT); // Skip matching root user
        while(!matching && *dataPtr) {
            uid_t fUid = 0;
            while(*(dataPtr) != ':' && *(dataPtr)) {
                fUid = 10 * fUid + *(dataPtr) - '0';
                dataPtr++;
            }
            dataPtr++;

            if(fUid == _current->uid) {
                matching = true;
            }
        }
        if(matching) {  // Update gid on success
            _current->gid = gid;
            RET(_current) = E_SUCCESS;
        } else {        // Fail if not on the list
            RET(_current) = E_NO_PERMISSION;
        }
        return;
    }

    // Didn't find the group in the entire file. Exit.
    RET(_current) = E_EOF;
    return;
}

// File system traversal helpers
static int _sys_getNextName(const char * path, char * nextName, int* nameLen) {
    *nameLen = 0;
    int i = 0;

    while(path[i] && path[i] != '/') {
        if (*nameLen < 12) {
            nextName[i] = path[i];
            *nameLen += 1;
        }
        i++;
    }
    
    for(int j = i; j < MAX_FILENAME_SIZE; j++) { // 0 fill the rest of nextName
        nextName[j] = 0;
    }
    
    return i; // Return the string after the read name
}

static int _sys_seekFile(const char* path, inode_id_t * currentDir) {
    int i = 0;

    // Get starting inode (either working directory or root directory)
    *currentDir = _current->wDir;
    if(path[0] == '/') {
        *currentDir = (inode_id_t){0, 1};
        i++;
        if(!(path[i])) {
            return E_SUCCESS;
        }
    }
    
    char nextName[12];
    while(path[i] != 0) {
        if (path[i] == '/') {     //If targeting slash, advance
            i++;
            if(!path[i]) {
                return E_SUCCESS;
            }
        }
        
        // Grab the name of the next entry
        int nameLen = 0;
        nameLen = _sys_getNextName(&(path[i]), nextName, &nameLen);

        if(nameLen == 0) {  // Entry names must have length > 0
            __cio_printf("*ERROR* in _sys_seekfile(): 0 length fs entry name in path \"%s\"\n", path);
            return E_BAD_PARAM;
        }
        

        i += nameLen;
        // Grab nextName from currentDir's entries
        int ret = _fs_getSubDir(*currentDir, nextName, currentDir);
        if(ret < 0) {
            __cio_printf("*ERROR* in _sys_seekfile(): Unable to get subDir \"%s\" in %d.%d\n", nextName, currentDir->devID, currentDir->idx);
            return ret;
        }
    }

    // If we made our way through the path successfully, return success (currentDir already set)
    return E_SUCCESS;
}

/**
 ** _sys_fopen - attempts to open a file and store its FD in the PCBs block
 **
 ** implements: 
 **    int32_t fopen(char * path, bool_t append);
 */
static void _sys_fopen( uint32_t args[4]) {
    char * path = (char*) args[0]; // Get path given to user
    bool_t append = (bool_t) args[1];
    uint32_t fdIdx;

    // Check if process has available files
    for (fdIdx = 0; fdIdx < MAX_OPEN_FILES; fdIdx++) {
        if(_current->files[fdIdx].inode_id.devID == 0 && 
            _current->files[fdIdx].inode_id.idx == 0) {
            break;
        } else if (fdIdx == MAX_OPEN_FILES - 1) {
            __cio_printf("*ERROR* in _sys_fopen: Out of file pointers\n");
            RET(_current) = E_FILE_LIMIT; // ERROR NO FILES AVAILABLE
            return;
        }       
    }

    // Seek the file itself
    inode_id_t currentDir;
    int result = _sys_seekFile(path, &currentDir);
    if(result < 0) {
        __cio_printf("*ERROR* in _sys_fopen: failed to seek target inode \"%s\"\n", path);
        RET(_current) = E_NO_CHILDREN;
        return;
    }

    // Load the referenced inode
    inode_t tgt;
    result = _fs_getInode(currentDir, &tgt);
    if(result < 0) {
        __cio_printf("*ERROR* in _sys_fopen: failed to retrieve target inode \"%s\"\n", path);
        RET(_current) = E_NO_CHILDREN;
        return;
    }

    // Check that this is an inode
    if(tgt.nodeType != INODE_FILE_TYPE) {
        __cio_printf("*ERROR* in _sys_fopen: Specified inode is not a file\n");
        RET(_current) = E_BAD_PARAM;
        return;
    }

    // Check that we have either read or write permissions on this
    bool_t canRead, canWrite;
    _fs_nodePermission(&tgt, _current->uid, _current->gid, &canRead, &canWrite, NULL);
    if(!canRead && !canWrite) {
        __cio_printf("*ERROR* in _sys_fopen: No rw permissions on this file\n");
        RET(_current) = E_NO_PERMISSION;
        return;
    }

    // Setup the file descriptor with this file and return the file type
    _current->files[fdIdx].inode_id = currentDir;
    _current->files[fdIdx].offset = (append) ? tgt.nBytes : 0;
    
    RET(_current) = fdIdx + 2; // Add channel (2) How do I return this? 
}

/**
 ** _sys_fclose - attempts to close an opened file
 **
 ** implements: 
 **    int32_t fclose(uint32_t chanNr);
 */
static void _sys_fclose (uint32_t args[4]) {
    uint32_t fdIdx;

    if(args[0] < 2 || args[0] >= 2 + MAX_OPEN_FILES) {
        RET(_current) = E_FILE_LIMIT; // Fail if not in closable file range (either SIO, CIO, or unused)
        return;
    }

    fdIdx = args[0] - 2;

    if(_current->files[fdIdx].inode_id.devID == 0 && 
            _current->files[fdIdx].inode_id.idx == 0) {
        RET(_current) = E_BAD_CHANNEL;   // Fail on null file
        return;
    }

    // NULL out the closed file and return success
    _current->files[fdIdx].inode_id.devID = 0;
    _current->files[fdIdx].inode_id.idx = 0;

    RET(_current) = E_SUCCESS;
}

/**
 ** _sys_fcreate - attempts to create a file/dir after the provided path
 **
 ** implements: 
 **    int32_t fcreate(char * path, char * name, bool_t isFile);
 */
static void _sys_fcreate  (uint32_t args[4]) {
    char * path = (char*) args[0];
    char * name = (char*) args[1];
    int result;
    bool_t isFile = (bool_t) args[2];
    inode_id_t newID, currentDir; 
    inode_t newNode, pNode;

    // Get Current Directory
    result = _sys_seekFile(path, &currentDir);
    if(result < 0) {
        RET(_current) = E_NO_CHILDREN;
        return;
    }

    // Get the parent inode
    result = _fs_getInode(currentDir, &pNode);
    if(result < 0){
        RET(_current) = E_NO_CHILDREN;
        return;
    }

    // Fail if parent node is not a directory
    if(pNode.nodeType != INODE_DIR_TYPE) {
        __cio_printf("*ERROR* in _sys_fcreate: path \"%s\" leads to non-directory\n", path);
        RET(_current) = E_BAD_PARAM;
        return;
    }

    // Check for name overlap
    for(uint32_t i = 0; i < pNode.nBytes; i++) {
        data_u temp;
        bool_t matched;

        result = _fs_getNodeEnt(&pNode, i, &temp);
        if(result < 0) { 
            RET(_current) = E_BAD_PARAM;
            return;
        }

        matched = true;
        for(uint32_t j = 0; j < MAX_FILENAME_SIZE; j++) {
            if(temp.dir.name[j] == 0 && name[j] == 0) {
                break;
            }

            if(temp.dir.name[j] != name[j]) {
                matched = false;
                break;
            }
        }
        if (matched) {
            __cio_printf("ERROR in _sys_fcreate: %s already exists in DIR\n", name);
            RET(_current) = E_BAD_PARAM;
            return;
        }
    }

    // Fail if no write permission in this directory
    bool_t canWrite;
    _fs_nodePermission(&pNode, _current->uid, _current->gid, NULL, &canWrite, NULL);
    if(!canWrite) {
        __cio_printf("*ERROR* in _sys_fcreate: Cannot create entries in \"%s\"\n", path);
        RET(_current) = E_NO_PERMISSION;
        return;
    }

    // Find next free inode
    result = _fs_allocNode(currentDir.devID, &newID);
    if(result < 0) {
        RET(_current) = E_NO_DATA;
        return;
    }

    // Get the inode via the index
    result = _fs_getInode(newID, &newNode);
    if(result < 0) {
        RET(_current) = E_NOT_FOUND;
        return;
    }    
    
    // Setup default inode values
    newNode.id = newID;
    newNode.uid = _current->uid;
    newNode.gid = _current->gid;
    newNode.permissions = DEFAULT_PERMISSIONS;     // Default to open access
    newNode.nRefs = 0; // No references yet
    newNode.nBlocks = 0;
    newNode.nBytes = 0;
    newNode.nodeType = (isFile) ? INODE_FILE_TYPE : INODE_DIR_TYPE;
    
    // Write the inode
    result = _fs_setInode(newNode);
    if(result < 0) {
        RET(_current) = result;
        return;
    }

    // Update parent Inode
    result = _fs_addDirEnt(currentDir, name, newID);
    if(result < 0) {
        RET(_current) = E_FAILURE;
        return;
    }

    RET(_current) = E_SUCCESS;
}

/**
 ** _sys_fremove - attempts to remove the file/dir at the end of the path
 **
 ** implements: 
 **    int32_t fremove(char * path, char * name);
 */
static void _sys_fremove (uint32_t args[4]) {
    char * path = (char*) args[0];
    char * name = (char*) args[1];
    
    // Get Current Directory
    inode_id_t targetDirID;
    int result = _sys_seekFile(path, &targetDirID);
    if(result < 0) {
        RET(_current) = E_NOT_FOUND;
        return;
    }

    // Find the child node
    inode_id_t childId;
    result = _fs_getSubDir(targetDirID, name, &childId);
    if(result < 0) {
        __cio_printf("*ERROR* in _sys_fremove: Could not grab node id for \"%s/%s\"\n", path, name);
        RET(_current) = E_NOT_FOUND;
        return; 
    }

    // Read the child node in
    inode_t child;
    result = _fs_getInode(childId, &child);
    if(result < 0) {
        __cio_printf("*ERROR* in _sys_fremove: Could not grab for \"%s/%s\"\n", path, name);
        RET(_current) = E_NOT_FOUND;
        return;
    }

    // Actually check node priveleges
    bool_t canMeta;
    result = _fs_nodePermission(&child, _current->uid, _current->gid, NULL, NULL, &canMeta);
    if(!canMeta) {
        __cio_printf("*ERROR* in _sys_fremove: Do not have meta priveleges for \"%s/%s\"\n", path, name);
        RET(_current) = E_NO_PERMISSION;
        return; 
    }

    // Check that the child is not a directory with referands
    if(child.nodeType == INODE_DIR_TYPE && child.nBytes != 0 && child.nRefs == 1) {
        __cio_printf("*ERROR* in _sys_fremove: Cannot remove non-empty directory \"%s/%s\"\n", path, name);
        RET(_current) = E_FAILURE;
        return; 
    }

    // Remove the entry from the parent
    result = _fs_rmDirEnt(targetDirID, name);
    if(result < 0) {
        __cio_printf("*ERROR*in _sys_fremove: Cannot remove entry from directory (fremove)\n");
        RET(_current) = E_FAILURE;
        return; 
    }

    RET(_current) = E_SUCCESS;
    return;
}

/**
 ** _sys_fmove - attempts to remove the file/dir at the end of the path
                 and move it to the new location!
 **
 ** implements: 
 **    int32_t fmove(char * sPath, char * sName, char * dPath, char* dName);
 */
static void _sys_fmove (uint32_t args[4]) {
    char * sPath = (char *)args[0];
    char * sName = (char *)args[1];
    char * dPath = (char *)args[2];
    char * dName = (char *)args[3];
    inode_id_t sourceDir;
    inode_id_t destDir;
    inode_id_t copyTarg;
    inode_t sourceNode;
    inode_t destNode;

    int result = _sys_seekFile(sPath, &sourceDir);
    if(result < 0) {
        // Failed to seek file on disk
        __cio_printf("*ERROR* in _sys_fmove: Failed to seek file path %s (%d)\n", sPath, result);
        RET(_current) = E_BAD_PARAM;
        return;
    }

    result = _sys_seekFile(dPath, &destDir);
    if(result < 0) {
        // Failed to seek file on disk
        __cio_printf("*ERROR* in _sys_fmove: Failed to seek file path %s (%d)\n", dPath, result);
        RET(_current) = E_BAD_PARAM;
        return;
    }

    // Read the source node from the disk
    result = _fs_getInode(sourceDir, &sourceNode);
    if(result < 0) {
        __cio_printf("*ERROR* in _sys_fmove: Failed to grab inode %d.%d (%d)\n", sourceDir.devID, sourceDir.idx, result);
        RET(_current) = E_FAILURE;
        return;
    }

    // Check that the source is a directory inode
    if(sourceNode.nodeType != INODE_DIR_TYPE) {
        __cio_printf("*ERROR* in _sys_fmove: Node at \"%s\" is not a directory\n", sPath);
        RET(_current) = E_NO_CHILDREN;
        return;
    }

    // Check that we have write priveleges in source node
    bool_t permitted;
    _fs_nodePermission(&sourceNode, _current->uid, _current->gid, NULL, &permitted, NULL);
    if(!permitted) {
        __cio_printf("*ERROR* in _sys_fmove: Cannot write to node \"%s\"\n", sPath);
        RET(_current) = E_NO_PERMISSION;
        return;
    }

    // Read the dest node from the disk
    result = _fs_getInode(destDir, &destNode);
    if(result < 0) {
        __cio_printf("*ERROR* in _sys_fmove: Failed to grab inode %d.%d (%d)\n", destDir.devID, destDir.idx, result);
        RET(_current) = E_FAILURE;
        return;
    }

    // Check that the dest is a directory inode
    if(destNode.nodeType != INODE_DIR_TYPE) {
        __cio_printf("*ERROR* in _sys_fmove: Node at \"%s\" is not a directory\n", dPath);
        RET(_current) = E_NO_CHILDREN;
        return;
    }

    // Check that we can write to dest
    _fs_nodePermission(&destNode, _current->uid, _current->gid, NULL, &permitted, NULL);
    if(!permitted) {
        __cio_printf("*ERROR* in _sys_fmove: Cannot write to node \"%s\"\n", dPath);
        RET(_current) = E_NO_PERMISSION;
        return;
    }

    // Get inode_id_t for source file and add it to the dest dir
    result = _fs_getSubDir(sourceDir, sName, &copyTarg); 
    if(result < 0) {
        __cio_printf("*ERROR* Unable to get \"%s\" in \"%s\"\n", sName, sourceDir);
        RET(_current) = result;
        return;
    }

    //Check that we have meta permissions for the file in question
    result = _fs_getPermission(copyTarg, _current->uid, _current->gid, NULL, &permitted, NULL);
    if(result < 0) {
        __cio_printf("*ERROR* in _sys_fmove: Could not get permissions for node \"%s/%s\"\n", sPath, sName);
        RET(_current) = E_NO_PERMISSION;
        return;
    } else if(!permitted) {
        __cio_printf("*ERROR* in _sys_fmove: No meta priveleges on node \"%s/%s\"\n", sPath, sName);
        RET(_current) = E_NO_PERMISSION;
        return;
    }
    

    // Add the new entry to the destination first
    result = _fs_addDirEnt(destDir, dName, copyTarg);
    if(result < 0) {
        __cio_printf("*ERROR* Unable to add \"%s\" to \"%s\"\n", dName, destDir);
        RET(_current) = result;
        return;
    }
    
    // Remove the file from source
    _sys_fremove(args);
    if(result < 0) {
        __cio_printf("*ERROR* Unable to remove \"%s\" from \"%s\"\n", sName, sourceDir);
        RET(_current) = E_FAILURE;
        return;
    }
}

/**
 ** _sys_getinode - attempts to retrieve the inode at the end of path
 **
 ** implements: 
 **    int32_t getinode(char * path, inode_t * inode);
 */
static void _sys_getinode (uint32_t args[4]) {
    char* path = (char *) args[0];
    inode_t * inode = (inode_t *) args[1];
    inode_id_t currentDir;

    // Seek the node in the directory tree
    int result = _sys_seekFile(path, &currentDir);
    if(result < 0) {
        // Failed to seek file on disk
        __cio_printf("*ERROR* in _sys_getinode: Failed to seek file path %s (%d)\n", path, result);
        RET(_current) = E_BAD_PARAM;
        return;
    }

    // Read the node from the disk
    result = _fs_getInode(currentDir, inode);
    if(result < 0) {
        __cio_printf("*ERROR* in _sys_getinode: Failed to grab inode %d.%d (%d)\n", currentDir.devID, currentDir.idx, result);
        RET(_current) = E_FAILURE;
        return;
    }

    // Clear out everything past gid and return success
    for(int idx = (void *)(&(inode->lock)) - (void*)(inode); idx < sizeof(inode_t); idx++) {
        ((char*)(inode))[idx] = 0;
    }

    RET(_current) = E_SUCCESS;
    return;
}

/**
 ** _sys_dirname - attempts to retrieve a subdirectory name after end of path
 **
 ** implements: 
 **    int32_t dirname(char * path, char* buf, uint32_t subDirNr);
 */
static void _sys_dirname (uint32_t args[4]) {
    /*
    ** - Grab the dir at end of path from disk
    **      - san check, grabbed dir with safe nr of subdirs
    ** - Write the subDirNr^th sub directory name from the inode to buf
    */
   char* path = (char*)(args[0]);
   char* buf = (char*)(args[1]);
   uint32_t subDirNr = args[2];
   inode_id_t id;
   inode_t node;
   data_u ent;
   int result;

    // Seek the node at the end of the path
    result = _sys_seekFile(path, &id);
    if(result < 0) {
        __cio_printf("*ERROR* in _sys_dirname: Failed to seek file path %s (%d)\n", path, result);
        RET(_current) = E_BAD_PARAM;
        return;
    }

    // Read the inode from disk
    result = _fs_getInode(id, &node);
    if(result < 0) {
        __cio_printf("*ERROR* in _sys_dirname: Failed to grab inode %d.%d (%d)\n", id.devID, id.idx, result);
        RET(_current) = E_FAILURE;
        return;
    }

    // Check that this is a directory inode
    if(node.nodeType != INODE_DIR_TYPE) {
        __cio_printf("*ERROR* in _sys_dirname: Node at \"%s\" is not a directory\n", path);
        RET(_current) = E_NO_CHILDREN;
        return;
    }

    // Check that we can read from this node
    bool_t canRead;
    _fs_nodePermission(&node, _current->uid, _current->gid, &canRead, NULL, NULL);
    if(!canRead) {
        __cio_printf("*ERROR* in _sys_dirname: Not permitted to read from node \"%s\"\n", path);
        RET(_current) = E_NO_PERMISSION;
        return;
    }

    // Quietly check that subDirNr is in bounds
    if(subDirNr >= node.nBytes) {
        RET(_current) = E_FILE_LIMIT;
        return;
    }

    // Grab the proper directory entry
    result = _fs_getNodeEnt(&node, subDirNr, &ent);
    if(result < 0) {
        __cio_printf("*ERROR* in _sys_dirname: Failed to retrieve entry %d from directory \"%s\" (%d)\n", subDirNr, path, result);
        RET(_current) = E_FAILURE;
        return;
    }

    // Copy from its name field to the output buffer and return success
    for(int i = 0; i < 12; i++) {
        buf[i] = ent.dir.name[i];
    }
    RET(_current) = E_SUCCESS;
    return;


}

/**
 * _sys_fchown - Changes the ownership of a directory 
 * 
 * implements:
 *    int32_t fchown(char * path, uid_t uid, gid_t gid)
 */
static void _sys_fchown(uint32_t args[4]) {
    char * path = (char *) args[0];
    uid_t uid = (uid_t) args[1];
    gid_t gid = (gid_t) args[2];

    inode_id_t id;
    inode_t node;
    int ret;

    // Seek the file
    ret = _sys_seekFile(path, &id);
    if(ret < 0) {
        __cio_printf("*ERROR* in _sys_fchown: Failed to get file at %s (%d)\n", path, ret);
        RET(_current) = E_BAD_PARAM;
        return;
    }

    // Get the file's inode
    ret = _fs_getInode(id, &node);
    if(ret < 0) {
        __cio_printf("*ERROR* in _sys_fchown: Failed to get inode at %s (%d)\n", path, ret);
        RET(_current) = E_NO_DATA;
        return;
    }

    // Check that we have permissions
    bool_t canMeta;
    _fs_nodePermission(&node, _current->uid, _current->gid, NULL, NULL, &canMeta);
    if(!canMeta) {
        __cio_printf("*ERROR* in _sys_fchown: No meta permissions on %s\n", path);
        RET(_current) = E_NO_PERMISSION;
        return;
    }

    // Update the node
    node.uid = uid;
    node.gid = gid;

    // Write the updated node to disk
    ret = _fs_setInode(node);
    if(ret < 0) {
        __cio_printf("*ERROR* in _sys_fchown: Failed to update inode at %s (%d)\n", path, ret);
        RET(_current) = E_NO_DATA;
        return;
    }

    RET(_current) = E_SUCCESS;
    return;
}

/**
 * _sys_fSetPerm - Changes the permissions of a directory 
 * 
 * implements:
 *    int32_t fSetPerm(char * path, uint32_t permissions);
 */
static void _sys_fSetPerm(uint32_t args[4]) {
    char * path = (char *) args[0];
    uint32_t permissions = args[1];

    inode_id_t id;
    inode_t node;
    int ret;

    // Seek the file
    ret = _sys_seekFile(path, &id);
    if(ret < 0) {
        __cio_printf("*ERROR* in _sys_fSetPerm: Failed to get file at %s (%d)\n", path, ret);
        RET(_current) = E_BAD_PARAM;
        return;
    }

    // Get the file's inode
    ret = _fs_getInode(id, &node);
    if(ret < 0) {
        __cio_printf("*ERROR* in _sys_fSetPerm: Failed to get inode at %s (%d)\n", path, ret);
        RET(_current) = E_NO_DATA;
        return;
    }

    // Check that we have permissions
    bool_t canMeta;
    _fs_nodePermission(&node, _current->uid, _current->gid, NULL, NULL, &canMeta);
    if(!canMeta) {
        __cio_printf("*ERROR* in _sys_fSetPerm: No meta permissions on %s\n", path);
        RET(_current) = E_NO_PERMISSION;
        return;
    }

    // Update the node
    node.permissions = permissions;

    // Write the updated node to disk
    ret = _fs_setInode(node);
    if(ret < 0) {
        __cio_printf("*ERROR* in _sys_fSetPerm: Failed to update inode at %s (%d)\n", path, ret);
        RET(_current) = E_NO_DATA;
        return;
    }

    RET(_current) = E_SUCCESS;
    return;
}


/**
 * _sys_setDir - Sets a new working directory
 * 
 * implements:
 *      int32_t setDir(char * path);
 */
static void _sys_setDir(uint32_t args[4]) {
    char * path = (char*) args[0];
    int ret;

    inode_id_t id;
    inode_t node;
    
    // Seek the new directory
    ret = _sys_seekFile(path, &id);
    if(ret < 0) {
        __cio_printf("*ERROR* in _sys_setDir: Failed to seek directory \"%s\" (%d)\n", path, ret);
        RET(_current) = E_NOT_FOUND;
        return;
    }

    // Pull the node
    ret = _fs_getInode(id, &node);
    if(ret < 0) {
        __cio_printf("*ERROR* in _sys_setDir: Failed to get inode for directory \"%s\" (%d)\n", path, ret);
        RET(_current) = E_NOT_FOUND;
        return;
    }

    // Check that this is a directory
    if(node.nodeType != INODE_DIR_TYPE) {
        __cio_printf("*ERROR* in _sys_setDir: \"%s\" is not a directory\n", path);
        RET(_current) = E_BAD_PARAM;
        return;
    }

    // Check that we have read permissions here
    bool_t canRead;
    _fs_nodePermission(&node, _current->uid, _current->gid, &canRead, NULL, NULL);
    if(!canRead) {
        __cio_printf("*ERROR* in _sys_setDir: No read permissions in \"%s\"\n", path);
        RET(_current) = E_NO_PERMISSION;
        return;
    }

    // Update the working directory and exit successfully
    _current->wDir = id;
    RET(_current) = E_SUCCESS;
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
    
    _syscalls[ SYS_getuid ]   = _sys_getuid;
    _syscalls[ SYS_getgid ]   = _sys_getgid;
    _syscalls[ SYS_setuid ]   = _sys_setuid;
    _syscalls[ SYS_setgid ]   = _sys_setgid;

    _syscalls[ SYS_fopen ]    = _sys_fopen;
    _syscalls[ SYS_fclose ]   = _sys_fclose;
    _syscalls[ SYS_fcreate]   = _sys_fcreate;
    _syscalls[ SYS_fremove ]  = _sys_fremove;
    _syscalls[ SYS_fmove ]    = _sys_fmove;
    _syscalls[ SYS_getinode ] = _sys_getinode;
    _syscalls[ SYS_dirname]   = _sys_dirname;
    _syscalls[ SYS_fchown]    = _sys_fchown;
    _syscalls[ SYS_fSetPerm]  = _sys_fSetPerm;
    _syscalls[ SYS_setDir]    = _sys_setDir;


    /*
#define SYS_fmove     20
#define SYS_getinode  21
#define SYS_dirname   22
    */

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
