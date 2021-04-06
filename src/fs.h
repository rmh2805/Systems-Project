#ifndef FS_H_
#define FS_H_

// Default size of char buffers
#define BUF_SiZE 4096

// Block size
#define BLOCK_SIZE 4096 // What size is this? 

#define MAX_FILENAME_SIZE 128

#define MAX_OPEN_FILES 128

#define MAX_DISK_SIZE (1 << 28) // 256 MB

#define MIN_DISK_SIZE (1 << 20) // 1 MB

#define MAX_BLOCKS (MAX_DISK_SIZE / BLOCK_SIZE)

#define DIRECT_POINTER_SIZE 15

/*
 * This structure will contain a variety of data 
 * relating to a file/directory in the FS.
 */
struct inode {
    // Meta Data
    uint32_t id;
    uint32_t nBlocks;
    uint32_t nBytes;
    uint32_t nRefs;

    // Permission information
    uid_t uid;
    gid_t gid; 
    uint32_t permissions: 24;
    uint32_t nodeType: 8;

    // Direct Pointers - each point to a block
    data_u direct_pointers[DIRECT_POINTER_SIZE]; // * 16 = 

    // Indirect Pointers
    block_t *extBlock; // Points to a block

    // Padding + lock = 4
    uint8_t lock;
    uint8_t pad[3];
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
