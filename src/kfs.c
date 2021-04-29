#include "kfs.h"

static driverInterface_t disks[MAX_DISKS];
char * inode_buffer;
char * data_buffer;
char * meta_buffer;

void _fs_init( void ) {
    __cio_puts( " FS:" );

    inode_buffer = _km_slice_alloc(); // Get 1024 bytes for our buffer
    meta_buffer = _km_slice_alloc(); // We only are using the first 512 bytes

    //todo Check that inode_buffer is properly allocated

    data_buffer = inode_buffer + BLOCK_SIZE; // Set data_buffer to the second half
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
            return E_BAD_PARAM;
    }

    unsigned int nextFree;
    for(nextFree = 0; nextFree < MAX_DISKS; nextFree++) {
        if(disks[nextFree].fsNr == 0) {
            break;
        }
    }
    if(nextFree == MAX_DISKS) {
        return E_BAD_CHANNEL; //Change to failure to allocate
    }

    // Read the metadata inode (inode index 0) from this device to determine its 
    // FS number:
    interface.readBlock(0, inode_buffer, interface.driverNr);
    inode_t inode = *(inode_t*)(inode_buffer);
    interface.fsNr = inode.id.devID;

    // If this device shares a number with an already present device, return 
    // failure: 
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

    uint8_t devID = file.inode_id.devID;
    uint32_t workingBlock = file.offset/BLOCK_SIZE;
    uint32_t bytes_read;
    // Read in inode for file! 
    disks[devID].readBlock(file.inode_id.idx, inode_buffer, disks[devID].driverNr); 

    // Get data and setup an inode
    void * tmp = (void *)inode_buffer;
    inode_t * inode;
    if(file.inode_id.idx % 2 == 0) {// Its even
        inode = (inode_t *)tmp; // Map the first 256 bytes
    } else { // Its odd so we want the second inode in the block
        inode = (inode_t *)(tmp + sizeof(inode_t)); // If we want the second block add 256 bytes
    }

    // Read starting block based on offset
    disks[devID].readBlock(inode->direct_pointers[workingBlock/4].blocks[workingBlock%4], 
                           data_buffer, disks[devID].driverNr); 
    workingBlock++;

    for(bytes_read = 0; bytes_read < inode->nBytes; bytes_read++) {
        if(bytes_read == len) {
            break; // We filled the buffer leave now
        }
        if(bytes_read % BLOCK_SIZE == 0 && bytes_read != 0) { // At multiple of 512, get new block
            disks[devID].readBlock(inode->direct_pointers[workingBlock/4].blocks[workingBlock%4], 
                                   data_buffer, disks[devID].driverNr); 
            workingBlock++;
        }
        buf[bytes_read] = (char)data_buffer[file.offset % BLOCK_SIZE]; // Just do a basic copy
        file.offset++;
    }
    return bytes_read;
}

int _fs_alloc_block(uint8_t devID, uint32_t * blockNr) {

    disks[devID].readBlock(0, meta_buffer, disks[devID].driverNr);
    // Read in meta
    inode_t metaNode = *(inode_t*)(meta_buffer);
    
    // Calculate nr of Inode blocks to determine start of map blocks 
    uint32_t mapBase = metaNode.nRefs / (BLOCK_SIZE/sizeof(inode_t));
    if(metaNode.nRefs % (BLOCK_SIZE/sizeof(inode_t)) != 0) {
        mapBase += 1;
    }
    
    for(uint32_t mapIdx = 0; mapIdx < metaNode.nBlocks; mapIdx++) {
        disks[devID].readBlock(mapBase + mapIdx, data_buffer, disks[devID].driverNr);
        for(uint32_t blockIdx; blockIdx < BLOCK_SIZE; blockIdx++) {
            for(uint8_t bitPos = 0; bitPos < 8; bitPos++) {
                uint8_t mask = 0x80 >> bitPos;
                if((data_buffer[blockIdx] & mask) ^ mask) {
                    data_buffer[blockIdx] |= mask;
                    disks[devID].writeBlock(mapBase + mapIdx, data_buffer, disks[devID].driverNr);
                    *blockNr = (mapIdx * 8 * BLOCK_SIZE);
                    *blockNr += blockIdx * 8;
                    *blockNr += bitPos;
                    return E_SUCCESS;
                }
            }
        }
    }
    return E_FAILURE;
}

int _fs_free_block(uint8_t devID, uint32_t blockNr) {
    disks[devID].readBlock(0, meta_buffer, disks[devID].driverNr);
    inode_t metaNode = *(inode_t*)(meta_buffer);

    uint32_t mapBase = metaNode.nRefs / (BLOCK_SIZE/sizeof(inode_t));
    if(metaNode.nRefs % (BLOCK_SIZE/sizeof(inode_t)) != 0) {
        mapBase += 1;
    }

    uint32_t mapIdx = blockNr/(8*BLOCK_SIZE);
    blockNr = blockNr % (8 * BLOCK_SIZE);
    uint32_t blockIdx = blockNr / 8;
    uint8_t bitPos = blockNr % 8;
    
    disks[devID].readBlock(mapBase + mapIdx, data_buffer, disks[devID].driverNr);
    uint8_t bitMask = (0x80 >> bitPos) ^ 0xFF;
    data_buffer[blockIdx] &= bitMask;
    disks[devID].writeBlock(mapBase + mapIdx, data_buffer, disks[devID].driverNr);
    
    return E_SUCCESS;
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
    uint32_t num_written = 0;
    uint32_t buffOffset = 0;

    // Read Inode!
    disks[devID].readBlock(file.inode_id.idx, inode_buffer, disks[devID].driverNr); 

    // Read in inode! 
    void * tmp = (void *)inode_buffer;
    inode_t * inode;
    if(file.inode_id.idx % 2 == 0) { // Its even
        inode = (inode_t *)tmp; // Map the first 256 bytes
    } else { // Its odd so we want the second inode in the block
        inode = (inode_t *)(tmp + sizeof(inode_t)); // If we want the second block add 256 
    }

    while(buffOffset < len) {
        uint32_t curBlock = file.offset/BLOCK_SIZE;
        uint32_t i, s = i = file.offset % BLOCK_SIZE;
        if(curBlock >= inode->nBlocks) {
            // Allocate a new block
            _fs_alloc_block(devID, &curBlock);
            disks[devID].readBlock(curBlock, data_buffer, disks[devID].driverNr); 
        } else if (i != 0) {
            // Read the block you're appending to into data buffer
            disks[devID].readBlock(curBlock, data_buffer, disks[devID].driverNr); 
        }
        while(i < BLOCK_SIZE) {
            data_buffer[i++] = buf[buffOffset++];
        }
        num_written += (i - s);
        file.offset += (i - s);
        // Write data_buffer out to disk
        disks[devID].writeBlock(curBlock, data_buffer, disks[devID].driverNr); 
    }
    // update inodes bytes
    inode->nBytes += num_written;
    return num_written;
}
