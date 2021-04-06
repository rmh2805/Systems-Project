#ifndef FS_H_
#define FS_H_

// Block size
#define BLOCK_SIZE 512

// Default size of char buffers
#define BUF_SIZE BLOCK_SIZE

//#define MAX_FILENAME_SIZE 128

//#define MAX_OPEN_FILES 128

//#define MAX_DISK_SIZE (1 << 28) // 256 MB

//#define MIN_DISK_SIZE (1 << 20) // 1 MB

//#define MAX_BLOCKS (MAX_DISK_SIZE / BLOCK_SIZE)

#define NUM_DIRECT_POINTERS 14

#define INODES_PER_BLOCK 2

/*
 * This structure will contain a variety of data 
 * relating to a file/directory in the FS.
 */
struct inode {
    // Meta Data 16 bytes
    uint32_t id;
    uint32_t nBlocks;
    uint32_t nBytes;
    uint32_t nRefs;

    // Permission information 8 bytes
    uid_t uid;
    gid_t gid; 
    uint32_t permissions: 24;
    uint32_t nodeType: 8;

    // Lock + Padding 4 bytes
    uint8_t lock; // 1 byte
    uint8_t pad[3]; // 3 bytes

    // Indirect Pointers 4 bytes
    block_t *extBlock; // Points to a block

    // Direct Pointers - each point to a block 
    data_u direct_pointers[NUM_DIRECT_POINTERS]; // (14 + 2) * 16 = 256 bytes per inode
};

// Each is 16 bytes
typedef union {
    block_t * blocks[4];
    struct dir {
        char name[12];
        block_t * block;
    }
} data_u;

typedef block_t uint32_t;

#endif /* FS_H_*/
