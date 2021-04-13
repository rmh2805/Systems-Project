#ifndef FS_H_
#define FS_H_

#include "common.h"

// Block size
#define BLOCK_SIZE 512

// Default size of char buffers
#define FS_BUF_SIZE BLOCK_SIZE

#define MAX_FILENAME_SIZE 12

#define MAX_OPEN_FILES 4

//#define MAX_DISK_SIZE (1 << 28) // 256 MB

//#define MIN_DISK_SIZE (1 << 20) // 1 MB

//#define MAX_BLOCKS (MAX_DISK_SIZE / BLOCK_SIZE)

#define NUM_DIRECT_POINTERS 14

typedef uint32_t block_t;

typedef struct {
    uint32_t devID : 8;
    uint32_t idx : 24;
} _inode_id_t;

typedef struct {
        char name[12];
        _inode_id_t block;
} dirEnt_t;

// Each is 16 bytes
typedef union {
    block_t blocks[4];
    dirEnt_t dir;
} data_u;

/*
 * This structure will contain a variety of data 
 * relating to a file/directory in the FS.
 */
typedef struct inode_s {
    // Meta Data (16 bytes)
    _inode_id_t id;
    uint32_t nBlocks;
    uint32_t nBytes;
    uint32_t nRefs;

    // Permission information (8 bytes)
    uint32_t permissions: 24;
    uint32_t nodeType: 8;
    uid_t uid;
    gid_t gid; 

    // Lock + Padding (4 bytes)
    uint8_t lock; // 1 byte
    uint8_t pad[3]; // 3 bytes

    // Indirect Pointers 4 bytes
    block_t extBlock; // Points to a block

    // Direct Pointers - each point to a block 
    data_u direct_pointers[NUM_DIRECT_POINTERS]; // 32 + 14 * 16 = 256 bytes per inode
} inode_t;

/*
 * Going to use the already existing read/write syscalls 
 */

/*
 * Unused id == 0
 * Root id   == 1
 * 
 * "device" 0 is reserved
 * boot device as 1
 */

/*
 *  - fOpen
 *      - Check that we can open a new file (fd available)
 *      - Grab file's inode id from provided path
 *          - return error on directory
 *          - return error(?) on undefined file (possibly call fCreate)
 *      - Set next free process FD with inode's ID and 0 offset
 *      - Return FD index + 2 (chanel number)
 *
 *  - fClose
 *      - null out the proper FD
 *  
 *  - fCreate
 *      - creates a new inode reference within a directory
 *      - Fail on overwrite
 *      - Takes a name and a type (file, directory, link)
 *
 *  - fRm
 *      - removes an inode reference from a directory
 *      - fail on free root, self reference ('.') or parent ('..')
 *      - fail on rm last link to non-empty directory
 *      - if that was the final reference to an inode, also free the referenced 
 *      inode and its associated blocks
 * 
 *  - getInode
 *      - Get the metadata of an inode at the end of a path
 * 
 *  - dirName
 *      - Handling indirect directory listings
 * 
 *  - _fs_read(fd_t fd, char* buf, uint32_t length)
 *      - Returns number of bytes read
 *      - links heavily into the driver
 *
 *  - fs_init() 
 *     specifies beginning of data section
 */

//todo describe _fs_write
//
// TODO 
// DISK ORG:
//   INODES BLOCKS
//   BITMAPS 
//   DATA BLOCKS
//
//   Don't need to worry about unknown end of disk! 

// In FS module: _fs_read(), _fs_write()

// Store FD inside PCB

#endif /* FS_H_*/
