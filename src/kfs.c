#include "kfs.h"

static driverInterface_t disks[MAX_DISKS];
char * inode_buffer;
char * data_buffer;
char * meta_buffer;

void _fs_init( void ) {
    __cio_puts( " FS:" );

    inode_buffer = _km_slice_alloc(); // Get 1024 bytes for our buffer
    meta_buffer = _km_slice_alloc(); // We only are using the first 512 bytes
    assert(inode_buffer != NULL && meta_buffer != NULL);

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
    uint32_t bytes_read;
    
    
    // Compute the disk index for devID;
    uint32_t i = 0;
    for(; i < MAX_DISKS; i++) {
        if(devID == disks[i].fsNr) {
            devID = i;
            break;
        }
    }
    if(i == MAX_DISKS) {
        return E_BAD_CHANNEL;
    }
    
    // Read in inode for file! 
    int ret = disks[devID].readBlock(file.inode_id.idx, inode_buffer, disks[devID].driverNr); 
    if(ret < 0) {
        __cio_puts( " ERROR: Unable to read inode block from disk! (_fs_read)");
        return ret;
    }

    // Get data and setup an inode
    uint32_t idx = sizeof(inode_t) * (file.inode_id.idx % (BLOCK_SIZE/sizeof(inode_t)));
    inode_t * inode = (inode_t *)(inode_buffer + idx);

    // 
    uint32_t blockIdx = file.offset / BLOCK_SIZE;
    for(bytes_read = 0; bytes_read < len && bytes_read < inode->nBytes;) {
        // Read next block from disk
        if(blockIdx < NUM_DIRECT_POINTERS) {
            ret = disks[devID].readBlock(inode->direct_pointers[blockIdx/4].blocks[blockIdx%4],
                    data_buffer, disks[devID].driverNr);
            if(ret < 0) {
                __cio_puts( " ERROR: Unable to read block from disk! (_fs_read)");
                return ret;
            }
        } else {
            //todo implement indirect reads
            return bytes_read;
        }
        
        uint32_t blockIdx = file.offset % BLOCK_SIZE;
        while(bytes_read < len && bytes_read < inode->nBytes && blockIdx < BLOCK_SIZE) {
            buf[bytes_read++] = data_buffer[blockIdx++];
        }
        file.offset += (blockIdx - (file.offset % BLOCK_SIZE)); // update file offset
        
        blockIdx++;
    }
    return bytes_read;
}

int _fs_alloc_block(uint8_t devID, uint32_t * blockNr) {

    int ret = disks[devID].readBlock(0, meta_buffer, disks[devID].driverNr);
    if(ret < 0) {
        __cio_puts( " ERROR: Unable to read metanode from disk!");
        return E_FAILURE;
    }
    // Read in meta
    inode_t metaNode = *(inode_t*)(meta_buffer);
    
    // Calculate nr of Inode blocks to determine start of map blocks 
    uint32_t mapBase = metaNode.nRefs / (BLOCK_SIZE/sizeof(inode_t));
    if(metaNode.nRefs % (BLOCK_SIZE/sizeof(inode_t)) != 0) {
        mapBase += 1;
    }
    
    for(uint32_t mapIdx = 0; mapIdx < metaNode.nBlocks; mapIdx++) {
        ret = disks[devID].readBlock(mapBase + mapIdx, data_buffer, disks[devID].driverNr);
        if(ret < 0) {
            __cio_puts( " ERROR: Unable to read block from disk! (_fs_alloc_block)");
            return E_FAILURE;
        }
        for(uint32_t blockIdx; blockIdx < BLOCK_SIZE; blockIdx++) {
            for(uint8_t bitPos = 0; bitPos < 8; bitPos++) {
                uint8_t mask = 0x80 >> bitPos;
                if((data_buffer[blockIdx] & mask) ^ mask) {
                    data_buffer[blockIdx] |= mask;
                    ret = disks[devID].writeBlock(mapBase + mapIdx, data_buffer, disks[devID].driverNr);
                    if(ret < 0) {
                        __cio_puts( " ERROR: Unable to write new block out to disk!" );
                        return E_FAILURE;
                    }
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
    int ret = disks[devID].readBlock(0, meta_buffer, disks[devID].driverNr);
    if(ret < 0) {
        __cio_puts( " ERROR: Unable to read metanode from disk!");
        return E_FAILURE;
    }
    inode_t metaNode = *(inode_t*)(meta_buffer);

    uint32_t mapBase = metaNode.nRefs / (BLOCK_SIZE/sizeof(inode_t));
    if(metaNode.nRefs % (BLOCK_SIZE/sizeof(inode_t)) != 0) {
        mapBase += 1;
    }

    uint32_t mapIdx = blockNr/(8*BLOCK_SIZE);
    blockNr = blockNr % (8 * BLOCK_SIZE);
    uint32_t blockIdx = blockNr / 8;
    uint8_t bitPos = blockNr % 8;
    
    ret = disks[devID].readBlock(mapBase + mapIdx, data_buffer, disks[devID].driverNr);
    if(ret < 0) {
        __cio_puts( " ERROR: Unable to read block from disk! (_fs_free_block)");
        return E_FAILURE;
    }
    uint8_t bitMask = (0x80 >> bitPos) ^ 0xFF;
    data_buffer[blockIdx] &= bitMask;
    ret = disks[devID].writeBlock(mapBase + mapIdx, data_buffer, disks[devID].driverNr);
    if(ret < 0) {
        __cio_puts( " ERROR: Unable to write block to disk! (_fs_free_block)");
        return E_FAILURE;
    }
    
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

    // Compute the disk index for devID;
    uint32_t i = 0;
    for(; i < MAX_DISKS; i++) {
        if(devID == disks[i].fsNr) {
            devID = i;
            break;
        }
    }
    if(i == MAX_DISKS) {
        return E_BAD_CHANNEL;
    }

    uint32_t num_written = 0;
    uint32_t bufOffset = 0;

    // Read Inode!
    int ret = disks[devID].readBlock(file.inode_id.idx, inode_buffer, disks[devID].driverNr); 
    if(ret < 0) {
        __cio_puts( " ERROR: Unable to read inode block from disk! (_fs_write)");
        return ret;
    }

    // Read in inode! 
    uint32_t idx = sizeof(inode_t) * (file.inode_id.idx % (BLOCK_SIZE/sizeof(inode_t)));
    inode_t * inode = (inode_t *)(inode_buffer + idx);

    while(bufOffset < len) {
        uint32_t curBlock = file.offset/BLOCK_SIZE;
        uint32_t i, s = i = file.offset % BLOCK_SIZE;
        
        if(curBlock >= NUM_DIRECT_POINTERS) {   // Nothing here
            return num_written;
        } if(curBlock >= inode->nBlocks) {
            // Allocate a new block
            _fs_alloc_block(devID, &curBlock);
            
            // Zero out data buffer
            for(int x = 0; x < 512; x++) {
                data_buffer[x] = 0;
            }
            // Update inode
            inode->nBlocks++;
            inode->direct_pointers[inode->nBlocks/4][inode->nBlocks%4] = curBlock;
        } else if (i != 0) {
            // Read the block you're appending to into data buffer
            ret = disks[devID].readBlock(curBlock, data_buffer, disks[devID].driverNr); 
            if(ret < 0) {
                __cio_printf( " ERROR: Unable to write block to disk! (_fs_write) (%d)", ret);
                return num_written;
            }
        }
        while(i < BLOCK_SIZE) {
            data_buffer[i++] = buf[bufOffset++];
        }
        
        ret = disks[devID].writeBlock(curBlock, data_buffer, disks[devID].driverNr); 
        if(ret < 0) {
            __cio_printf( " ERROR: Unable to write block to disk! (_fs_write) (%d)", ret);
            return num_written;
        }
        num_written += (i - s);
        file.offset += (i - s);
    }
    // update inodes bytes
    inode->nBytes += num_written;
    ret = disks[devID].writeBlock(file.inode_id.idx, inode_buffer, disks[devID].driverNr); 
    if(ret < 0) {
        __cio_printf( " ERROR: Unable to write inode back to disk! (_fs_write) (%d)", ret);
        return num_written;
    }
    return num_written;
}

/**
 * Reads an inode from disk (Exposed)
 * 
 * @param id The inode id to access
 * @param inode A return pointer for the grabbed inode
 * 
 * @return A standard exit status
 */
int _fs_getInode(inode_id_t id, inode_t * inode) {
    if(id.devID == 0 && id.idx == 1) {  // Reassign default channel
        id = (inode_id_t){disks[0].fsNr, 1};
    }
    
    if(id.devID == 0) { // Return 0 if targeting a non-existent dev ID
        return E_BAD_CHANNEL;
    }
    
    uint32_t disk = 0;
    for(; disk < MAX_DISKS; disk++) {
        if(disks[disk].fsNr == id.devID) {
            break;
        }
    }
    if (disk == MAX_DISKS) return E_BAD_CHANNEL; // Ensure disk exists
    
    // Read the metadata node from disk
    int result = disks[disk].readBlock(0, meta_buffer, disks[disk].driverNr);
    if(result <= 0) return result; // Ensure meta read was successful
    inode_t metaNode = *(inode_t*)(meta_buffer);
    
    // Check that this inode exists on this disk
    if(id.idx >= metaNode.nRefs) return E_BAD_PARAM;
    
    // Read in the inode from disk
    uint32_t idx = id.idx/(BLOCK_SIZE/sizeof(inode_t));
    result = disks[disk].readBlock(idx, inode_buffer, disks[disk].driverNr);
    if(result <= 0) return result; // Ensure read was successful
    
    // Copy the read inode to the return struct
    idx = sizeof(inode_t) * id.idx % (BLOCK_SIZE/sizeof(inode_t));
    *inode = *(inode_t*)(inode_buffer + idx);
    
    return E_SUCCESS;
}
