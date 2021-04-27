#include "kfs.h"

static driverInterface_t disks[MAX_DISKS];
char * inode_buffer;
char * data_buffer;

void _fs_init( void ) {
    __cio_puts( " FS:" );

    inode_buffer = _km_slice_alloc(); // Get 1024 bytes for our buffer
    data_buffer = inode_buffer + 512; // Set data_buffer to the second half
    for(unsigned int i = 0; i < MAX_DISKS; i++) {
        disks[i].fsNr = 0;
    }

    __cio_puts( " done" );
}

/**
 * Register a device for read and write
 * 
 * @param interface The proper interface for this device
 * 
 * @returns The number this was registered as (error if return < 0)
 */
int _fs_registerDev(driverInterface_t interface) {
    
    if(interface.readBlock == NULL || interface.writeBlock == NULL) {
            return E_FAILURE;
    }

    unsigned int nextFree;
    for(nextFree = 0; nextFree < MAX_DISKS; nextFree++) {
        if(disks[nextFree].fsNr == 0) {
            break;
        }
    }
    if(nextFree == MAX_DISKS) {
        return E_FAILURE; //Change to failure to allocate
    }

    // Read the metadata inode (inode index 0) from this device to determine its 
    // FS number:
    //  interface.fsNr = fsNr

    // If this device shares a number with an already present device, return 
    // failure: 
    //  if(interface.fsNr == disks[i].fsNr) return failure;
    for(int i = 0; i < MAX_DISKS; i++) {
        if(interface.fsNr == disks[i].fsNr) {
            return E_FAILURE;
        }
    }

    disks[nextFree] = interface;
    return nextFree;
}

/**
 * FS read handler
 * 
 * @param file The file descriptor to read from
 * @param buf The character buffer to read into
 * @param len The max number of characters to read into the buffer
 * 
 * @returns The number of bytes read from disk
 */
int _fs_read(fd_t file, char * buf, uint32_t len) {
    // Get inode_id from fd_t, and follow that to find inode offset. Read that inode in
    // and take either top or bottom 256. Then follow its pointersjjj
    // fd_t->inode_id & offset 
    // inode_id is a inode_id_t which contains idx, devID

    // char * block = | 512 bytes of inodes | 512 blocks of garbage |
    // | 512 Bytes of inodes | = | 0 - 255 inode1 | 256 - 511 inode2 |
    
    uint8_t devID = file.inode_id.devID;
    // Read in inode for file! 
    disks[devID].readBlock(file.inode_id.idx, inode_buffer, devID); 

    // Get data and setup an inode
    void * tmp = (void *)inode_buffer;
    inode_t * inode;
    if(file.inode_id.idx % 2 == 0) {// Its even
        inode = (inode_t *)tmp; // Map the first 256 bytes
    } else { // Its odd so we want the second inode in the block
        inode = (inode_t *)(tmp + 256); // If we want the second block add 256 
    }

    uint32_t nextBlock = 0;
    int bytes_read = 0;
    for(bytes_read = 0; bytes_read < inode->nBytes; bytes_read++) {
        if(bytes_read == len) {
            break; // We filled the buffer leave now
        }
        if(bytes_read % 512 == 0) { // We are at a multiple of 512, get a new block
            disks[devID].readBlock(inode->direct_pointers[nextBlock/4].blocks[nextBlock%4], 
                                   data_buffer, devID); 
            nextBlock++;
        }
        buf[bytes_read] = (char)data_buffer[bytes_read%512]; // Just do a basic copy
    }
    return bytes_read;
}

/**
 * FS write handler
 * 
 * @param file The file descriptor to write to
 * @param buf The character buffer to write from
 * @param len The max number of characters to write from the buffer
 * 
 * @returns The number of bytes read from disk
 */
int _fs_write(fd_t file, char * buf, uint32_t len) {

    return E_FAILURE;
}
