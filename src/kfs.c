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
        
        if(curBlock >= NUM_DIRECT_POINTERS) {   // Cannot currently handle indirect blocks
            return num_written;
        } 
        
        if(curBlock >= inode->nBlocks) { // Allocate needed new blocks
                // Allocate a new block
                ret = _fs_alloc_block(devID, &curBlock);
                if(ret < 0) {
                    __cio_printf( " ERROR: Unable to alloc block on disk! (_fs_write) (%d)", ret);
                    return ret;
                }
                
                
                // Update inode
                inode->nBlocks++;
                inode->direct_pointers[inode->nBlocks/4].blocks[inode->nBlocks%4] = curBlock;
        }
        
        // Dereference the current block
        curBlock = inode->direct_pointers[(file.offset/BLOCK_SIZE)/4].blocks[(file.offset/BLOCK_SIZE) % 4];
        
        // Initialize the data buffer
        if (i != 0) {    // If writing to the middle of a block, load it into the buffer
            ret = disks[devID].readBlock(curBlock, data_buffer, disks[devID].driverNr); 
            if(ret < 0) {
                __cio_printf( " ERROR: Unable to write block to disk! (_fs_write) (%d)", ret);
                return ret;
            }
        } else { // Otherwise clear the buffer
            for(int x = 0; x < 512; x++) {  // zero out the data buffer
                data_buffer[x] = 0;
            }
        }
        
        // Copy from the buffer into the data block until block or buf is done
        while(i < BLOCK_SIZE && bufOffset < len) {
            data_buffer[i++] = buf[bufOffset++];
        }
        
        // Write the data buffer out to disk
        ret = disks[devID].writeBlock(curBlock, data_buffer, disks[devID].driverNr); 
        if(ret < 0) {
            __cio_printf( " ERROR: Unable to write block to disk! (_fs_write) (%d)", ret);
            return ret;
        }
        num_written += (i - s);
        file.offset += (i - s);
    }
    
    // Update the inode and write it to disk
    inode->nBytes += num_written;
    ret = disks[devID].writeBlock(file.inode_id.idx, inode_buffer, disks[devID].driverNr); 
    if(ret < 0) {
        __cio_printf( " ERROR: Unable to write inode back to disk! (_fs_write) (%d)", ret);
        return ret;
    }
    
    // Return the number of bytes written
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
    idx = sizeof(inode_t) * (id.idx % (BLOCK_SIZE/sizeof(inode_t)));
    *inode = *(inode_t*)(inode_buffer + idx);
    
    return E_SUCCESS;
}

/**
 * Writes an inode to disk (Exposed)
 * 
 * @param inode the inode to write to disk 
 * 
 * @return A standard exit status
 */
int _fs_setInode(inode_t inode) {
    uint32_t disk, inodeOffset;
    int ret;
    block_t inodeBlock;
    
    if(inode.id.devID == 0) { // There is no device zero
        return E_BAD_CHANNEL;
    }
    
    // Determine the disk index
    for(disk = 0; disk < MAX_DISKS; disk++){
        if(disks[disk].fsNr == inode.id.devID) {
            break;
        }
    }
    if(disk == MAX_DISKS) { // If disk not found, return failure status
        return E_BAD_CHANNEL;
    }
    
    // Load the inode block into memory
    inodeBlock = inode.id.idx / (BLOCK_SIZE / sizeof(inode_t));
    ret = disks[disk].readBlock(inodeBlock, inode_buffer, disks[disk].driverNr);
    if(ret < 0) {
        return ret;
    }
    
    // Copy the inode into the buffer
    inodeOffset = sizeof(inode_t) * (inode.id.idx % (BLOCK_SIZE / sizeof(inode_t)));
    inode_t * dst = (inode_t *)(inode_buffer + inodeOffset);
    *dst = inode;
    
    // Write the updated buffer out to memory
    ret = disks[disk].writeBlock(inodeBlock, inode_buffer, disks[disk].driverNr);
    if(ret < 0) {
        return ret;
    }
    
    return E_SUCCESS;
}

/**
 * Internal helper function to return the `idx`th data entry from the passed 
 * inode
 * 
 * @param inode The inode to grab data entries from
 * @param idx The index of the entry to grab
 * @param ret A return pointer for the grabbed node entry
 * 
 * @return A standard exit status
 */
int _fs_getNodeEnt(inode_t* inode, int idx, data_u * ret) {
    if(inode == NULL) {
        __cio_printf("*ERROR in _fs_getNodeEnt()* NULL inode\n");
        return E_BAD_PARAM;
    }

    // Check that this is assigned within the inode 
    if(idx >= inode->nBytes) {
        __cio_printf("*ERROR in _fs_getNodeEnt()* Passed index not in DIR bounds\n");
        return E_BAD_PARAM;
    }

    // Return direct entry
    if(idx < NUM_DIRECT_POINTERS) {
        *ret = inode->direct_pointers[idx];
        return E_SUCCESS;
    } else { // Handle indirect entries
        __cio_printf("*ERROR in _fs_getNodeEnt()* Unable to handle indirect entries\n");
        return E_BAD_PARAM;
    }
}

/**
 * Internal helper function to set the `idx`th data entry in the passed node
 * 
 * @param inode The inode to modify
 * @param idx The index of the entry to set
 * @param ret A return pointer for the grabbed node entry
 * 
 * @return The index modified (Error if ret < 0)
 */
int setNodeEnt(inode_t* inode, int idx, data_u ent) {
    if(inode == NULL) {
        __cio_printf("*ERROR in _fs_setNodeEnt()* NULL inode\n");
        return E_BAD_PARAM;
    }

    // If need a new entry, append this block to the end of the current list
    if(idx >= inode->nBytes) {
        //todo add support for indirect entries
        if(inode->nBytes + 1 < NUM_DIRECT_POINTERS) {
            idx = inode->nBytes++;
        } else {
            __cio_printf("*ERROR in _fs_setNodeEnt()* does not support indirect entries\n");
            return E_BAD_PARAM;
        }
    }

    if(idx < NUM_DIRECT_POINTERS) { // Set the direct entry
        inode->direct_pointers[idx] = ent;
    } else { // todo Add support for indirect entries
        __cio_printf("*ERROR in _fs_setNodeEnt()* does not support indirect entries\n");
        return E_BAD_PARAM;
    }

    return idx;
}

/**
 * Adds an entry to a directory inode (Exposed)
 * 
 * @param inode The inode to update
 * @param name The name to associate with this entry
 * @param buf The new target inode
 * 
 * @return A standard exit status
 */
int _fs_addDirEnt(inode_id_t inode, const char* name, inode_id_t buf) {
    inode_t *tgt = NULL;
    int ret;

    // Get the inode
    ret = _fs_getInode(inode, tgt);
    if(ret < 0) {
        return ret;
    }

    // todo Add support for extention blocks

    // Fail if out of direct pointers
    if(tgt->nBytes == NUM_DIRECT_POINTERS) {
        __cio_printf("ERROR in _fs_addDirEnt*: Unable to add new dir entry\n");
        return E_FILE_LIMIT;
    }

    // Check for name overlap
    for(uint32_t i = 0; i < tgt->nBytes; tgt++) {
        data_u temp;
        bool_t matched;

        ret = _fs_getNodeEnt(tgt, i, &temp);
        if(ret < 0) return ret;

        matched = true;
        for(uint32_t j = 0; j < 12; j++) {
            if(temp.dir.name[j] == 0 && name[j] == 0) {
                break;
            }

            if(temp.dir.name[j] != name[j]) {
                matched = false;
                break;
            }
        }
        if (matched) {
            __cio_printf("ERROR in _fs_addDirEnt*: %s already exists in DIR\n", name);
            return E_BAD_PARAM;
        }
    }

    return E_FAILURE;
}

/**
 * Removes an entry from a directory inode (Exposed)
 * 
 * @param inode The inode to update
 * @param name The entry name to remove
 * 
 * @return A standard exit status
 */
int _fs_rmDirEnt(inode_id_t inode, const char* name) {
    return E_FAILURE;
}

/**
 * Returns the data from a particular directory entry (Exposed)
 * 
 * @param inode The inode to access
 * @param idx The index of the data entry to grab
 * @param entry A return pointer for the directory entry
 * 
 * @return A standard exit status
 */
int _fs_getDirEnt(inode_id_t inode, uint32_t idx, data_u* entry) {
    return E_FAILURE;
}
