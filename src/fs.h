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
    unsigned short mode; // This is the same as umode_t
    uint32_t size; 
    uint32_t id;
    enum inode_type type;
    uint32_t nBlocks;
    uint32_t nBytes;

    // Permission information
    uid_t uid;
    gid_t gid; 
    uint32_t permissions;

    uint8_t lock; // Spinlock? 
    
    // Direct Pointers - each point to a block
    data_u direct_pointers[DIRECT_POINTER_SIZE]; // 15 * 4 = 60 bytes

    // Indirect Pointers
};

// Each is 20 bytes
typedef union {
    block_t * blocks[5];
    struct dir {
        char name[16];
        block_t * block;
    }
} data_u;


struct block_t {

};

enum inode_type {
    NORMAL_FILE,
    NORMAL_DIR,
}

#endif /* FS_H_*/
