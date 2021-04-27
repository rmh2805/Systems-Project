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
    uint32_t workingBlock = file.offset/512;
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

    // Read starting block based on offset
    disks[devID].readBlock(inode->direct_pointers[workingBlock/4].blocks[workingBlock%4], 
                           data_buffer, devID); 
    workingBlock++;

    //uint32_t nextBlock = 0;
    //int bytes_read = 0;
    for(bytes_read = 0; bytes_read < inode->nBytes; bytes_read++) {
        if(bytes_read == len) {
            break; // We filled the buffer leave now
        }
        if(bytes_read % 512 == 0 && bytes_read != 0) { // At a multiple of 512, get a new block
            disks[devID].readBlock(inode->direct_pointers[workingBlock/4].blocks[workingBlock%4], 
                                   data_buffer, devID); 
            workingBlock++;
        }
        buf[bytes_read] = (char)data_buffer[file.offset%512]; // Just do a basic copy
        file.offset++;
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
 * @returns The number of bytes written to disk
 */
int _fs_write(fd_t file, char * buf, uint32_t len) {

    uint8_t devID = file.inode_id.devID;
    uint32_t workingBlock = file.offset/512; // block in file we are starting to write
    uint32_t total_blocks_to_write = len/512 + 1;
    uint32_t workingOffset = file.offset % 512;

    // So you need to read this block and start writing shit after that
    // THEN you write that whole block out. THEN you need to get the next block
    // allocating it if necessary and then allocating it and moving forward and 
    // filling that shit. To allocate the node you need to read the metanode (root) and
    // make a new block! 

    disks[devID].readBlock(file.inode_id.idx, inode_buffer, devID); 

    void * tmp = (void *)inode_buffer;
    inode_t * inode;
    if(file.inode_id.idx % 2 == 0) {// Its even
        inode = (inode_t *)tmp; // Map the first 256 bytes
    } else { // Its odd so we want the second inode in the block
        inode = (inode_t *)(tmp + 256); // If we want the second block add 256 
    }

    // We need to get the first block (via offset) 
    // Then we need to go through each block for as long as we write and copy
    // data over, and then when that fills up - we need to write that block out to disk
    // So we need a read, then a write
    for(int i = 0; i < total_blocks_to_write; i++) { // For each block
        disks[devID].writeBlock(inode->direct_pointers[workingBlock/4].blocks[workingBlock%4], 
                           buf, devID); 
        workingBlock++; // Goto next block
        if// Need to increment buf
    }

    return E_FAILURE;
}
