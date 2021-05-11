#include "kfs.h"

static driverInterface_t disks[MAX_DISKS];
char * inode_buffer;
char * data_buffer;
char * meta_buffer;

int _fs_getNodeEnt(inode_t* inode, int idx, data_u * ret);
int _fs_setNodeEnt(inode_t* inode, int idx, data_u ret);

void _fs_init( void ) {
    __cio_printf( " FS:" );

    inode_buffer = _km_slice_alloc(); // Get 1024 bytes for our buffer
    meta_buffer = _km_slice_alloc(); // We only are using the first 512 bytes
    assert(inode_buffer != NULL && meta_buffer != NULL);

    data_buffer = inode_buffer + BLOCK_SIZE; // Set data_buffer to the second half
    for(unsigned int i = 0; i < MAX_DISKS; i++) {
        disks[i].fsNr = 0;
    }

    __cio_printf( " done" );
}

/**
 * Register a device for read and write
 * 
 * @param interface The proper interface for this device
 * 
 * @returns The number this was registered as (error if return < 0)
 */
int _fs_registerDev(driverInterface_t interface) {
    __cio_printf("(regStart %d) ", interface.driverNr);
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
    int ret = interface.readBlock(0, inode_buffer, interface.driverNr);
    if (ret < 0) {
        __cio_printf("(Unable to read from disk (%d))", ret);
        return E_FAILURE;
    }
    inode_t inode = *(inode_t*)(inode_buffer);
    interface.fsNr = inode.id.devID;
    

    // If this device shares a number with an already present device, return 
    // failure: 
    for(int i = 0; i < MAX_DISKS; i++) {
        if(interface.fsNr == disks[i].fsNr) {
            __cio_printf("(Override of fs %d)", disks[i].fsNr);
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
int _fs_read(fd_t * file, char * buf, uint32_t len) {
    uint8_t devID = file->inode_id.devID;
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
        __cio_printf("*ERROR* in _fs_read: Canot find disk %d\n", file->inode_id.devID);
        return E_BAD_CHANNEL;
    }
    
    // Read in inode for file! 
    inode_t node;
    int ret = _fs_getInode(file->inode_id, &node);
    if(ret < 0) {
        __cio_printf("*ERROR* in _fs_read: Failed to read inode %d.%d (%d)\n", 
            file->inode_id.devID, file->inode_id.idx, ret);
        return E_FAILURE;
    }
    

    // Return EOF on EOF
    if(file->offset == node.nBytes) {
        return E_EOF;
    }

    
    for(bytes_read = 0; bytes_read < len && file->offset < node.nBytes;) {
        // Calculate the entry number of the next block
        uint32_t blockIdx = file->offset / BLOCK_SIZE;

        // Exit early if block is indirect (4 * nPtrBlocks)
        if(blockIdx >= NUM_DIRECT_POINTERS * 4) {
            return bytes_read;
        }

        // Get next data block's offset
        data_u data;
        ret = _fs_getNodeEnt(&node, blockIdx / 4, &data);
        if(ret < 0) {
            __cio_printf("*ERROR* in _fs_read: Failed to read node entry %d (%d)\n", 
                blockIdx/4, ret);
        }
        block_t block = data.blocks[blockIdx % 4];

        // Read the block in to the data buffer
        ret = disks[devID].readBlock(block, data_buffer, disks[devID].driverNr);
        if(ret < 0) {
            __cio_printf( "*ERROR* in _fs_read: Unable to read block %d from disk (%d)\n", block, ret);
            return E_FAILURE;
        }

        // Read bytes until buffer full, file done, or block end
        int idx = file->offset % BLOCK_SIZE; // Calculate the offset into the current block
        while(bytes_read < len && file->offset < node.nBytes && idx < BLOCK_SIZE) { 
            buf[bytes_read++] = data_buffer[idx++];
            file->offset += 1;
        }
    }
    return bytes_read;
}

int _fs_alloc_block(uint8_t fsNr, uint32_t * blockNr) {
    fsNr = (fsNr == 0) ? disks[0].fsNr : fsNr;

    inode_t metaNode;
    int ret = _fs_getInode((inode_id_t){(fsNr == 0) ? disks[0].fsNr : fsNr, 0}, &metaNode);
    if(ret < 0) {
        __cio_printf( "*ERROR* in _fs_alloc_block: Unable to read meta node from disk\n");
        return E_FAILURE;
    }
    
    // Dereference the drive idx
    uint8_t driveIdx, i;
    for(i = 0; i < MAX_DISKS; i++) {
        if(disks[i].fsNr == fsNr) {
            driveIdx = i;
            break;
        }
    }
    if(i == MAX_DISKS) {
        return E_BAD_CHANNEL;
    }

    // Calculate nr of Inode blocks to determine start of map blocks 
    uint32_t mapBase = metaNode.nRefs / (BLOCK_SIZE/sizeof(inode_t));
    if(metaNode.nRefs % (BLOCK_SIZE/sizeof(inode_t)) != 0) {
        mapBase += 1;
    }
    
    for(uint32_t mapIdx = 0; mapIdx < metaNode.nBlocks; mapIdx++) {
        ret = disks[driveIdx].readBlock(mapBase + mapIdx, data_buffer, disks[driveIdx].driverNr);
        if(ret < 0) {
            __cio_printf( " ERROR: Unable to read block from disk! (_fs_alloc_block)\n");
            return E_FAILURE;
        }

        for(uint32_t blockIdx = 0; blockIdx < BLOCK_SIZE; blockIdx++) {
            for(uint8_t bitPos = 0; bitPos < 8; bitPos++) {
                uint8_t mask = 0x80 >> bitPos;
                

                if((data_buffer[blockIdx] & mask) ^ mask) {
                    data_buffer[blockIdx] |= mask;
                    
                    ret = disks[driveIdx].writeBlock(mapBase + mapIdx, data_buffer, disks[driveIdx].driverNr);
                    if(ret < 0) {
                        __cio_printf( " ERROR: Unable to write new block out to disk!\n" );
                        return E_FAILURE;
                    }
                    *blockNr = mapBase + metaNode.nBlocks;
                    *blockNr += (mapIdx * 8 * BLOCK_SIZE);
                    *blockNr += blockIdx * 8;
                    *blockNr += bitPos;
                    return E_SUCCESS;
                }
            }
        }
    }
    return E_FAILURE;
}

int _fs_free_block(uint8_t fsNr, uint32_t blockNr) {
    fsNr = (fsNr == 0) ? disks[0].fsNr : fsNr;
    uint8_t driveIdx = 0;

    inode_t metaNode;
    int ret = _fs_getInode((inode_id_t){fsNr, 0}, &metaNode);
    if(ret < 0) {
        __cio_printf( " ERROR: Unable to read metanode from disk!\n");
        return E_FAILURE;
    }

    // Dereference the device ID
    int i = 0;
    for(; i < MAX_DISKS; i++) {
        if(disks[i].fsNr == fsNr) {
            driveIdx = i;
            break;
        }
    }
    if(i == MAX_DISKS) {
        return E_BAD_CHANNEL;
    }

    uint32_t mapBase = metaNode.nRefs / (BLOCK_SIZE/sizeof(inode_t));
    if(metaNode.nRefs % (BLOCK_SIZE/sizeof(inode_t)) != 0) {
        mapBase += 1;
    }

    
    int blockNr2 = blockNr - (mapBase + metaNode.nBlocks);

    uint32_t mapIdx = blockNr2/(8*BLOCK_SIZE);
    blockNr2 = blockNr2 % (8 * BLOCK_SIZE);
    uint32_t blockIdx = blockNr2 / 8;
    uint8_t bitPos = blockNr2 % 8;
    
    ret = disks[driveIdx].readBlock(mapBase + mapIdx, data_buffer, disks[driveIdx].driverNr);
    if(ret < 0) {
        __cio_printf( " ERROR: Unable to read block from disk! (_fs_free_block)\n");
        return E_FAILURE;
    }
    uint8_t bitMask = ~(0x80 >> bitPos);

    data_buffer[blockIdx] &= (bitMask & 0xFF);

    ret = disks[driveIdx].writeBlock(mapBase + mapIdx, data_buffer, disks[driveIdx].driverNr);
    if(ret < 0) {
        __cio_printf( " ERROR: Unable to write block to disk! (_fs_free_block)\n");
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
int _fs_write(fd_t * file, char * buf, uint32_t len) {
    uint8_t devID = file->inode_id.devID;
    int ret;
    uint32_t bufOffset;

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

    // Read in inode for file
    inode_t node;
    ret = _fs_getInode(file->inode_id, &node);
    if(ret < 0) {
        __cio_printf("*ERROR* in _fs_write: Failed to read inode %d.%d (%d)\n", 
            file->inode_id.devID, file->inode_id.idx, ret);
        return E_FAILURE;
    }

    // Make sure are offset is the end of the file
    if(file->offset != node.nBytes) {
        __cio_printf("*ERROR* in _fs_write: Cannot write to file without appending\n");
        return E_FAILURE;
    }

    bufOffset = 0;
    while(bufOffset < len) {
        // Calculate the entry index of the next block
        uint32_t blockIdx = file->offset / BLOCK_SIZE;
        
        // Exit early if block is indirect (4 * nPtrBlocks)
        if(blockIdx >= NUM_DIRECT_POINTERS * 4) {
            return bufOffset;
        }

        //Get the data entry for the next block
        data_u data;
        if(node.nBlocks != 0 && node.nBytes != 0) { // We don't want to do this if its new
            ret = _fs_getNodeEnt(&node, blockIdx / 4, &data);
            if(ret < 0) {
                __cio_printf("*ERROR* in _fs_write: Failed to read node entry %d (%d)\n",
                    blockIdx/4, ret);
                return E_FAILURE;
            }
        }
        // Potentially allocate a new block
        if(blockIdx > node.nBlocks) { // desync between write pos and current EOF
            return E_FAILURE;
        } else if(blockIdx == node.nBlocks) {
            block_t newBlock = 0;

            // Allocate the new data block
            ret = _fs_alloc_block(file->inode_id.devID, &newBlock);
            if(ret < 0) {
                __cio_printf("*ERROR* in _fs_write: Unable to alloc new block\n");
                return E_NO_DATA;
            }

            // Update the data entry
            data.blocks[blockIdx % 4] = newBlock;
            node.nBlocks += 1;

            // Write the updated entry out to the node
            _fs_setNodeEnt(&node, blockIdx / 4, data);
        }

        // Get the proper block offset and calculate offset into the block
        block_t block = data.blocks[blockIdx % 4];
        int idx = file->offset % BLOCK_SIZE;

        // Initialize the data buffer
        if (idx != 0) {    // If writing to the middle of a block, load it into the buffer
            ret = disks[devID].readBlock(block, data_buffer, disks[devID].driverNr); 
            if(ret < 0) {
                __cio_printf( "*ERROR* in _fs_write: Unable to read block %d from disk (%d)\n", 
                    block, ret);
                return E_FAILURE;
            }
        } else { // Otherwise clear the buffer
            for(int x = 0; x < 512; x++) {  // zero out the data buffer
                data_buffer[x] = 0;
            }
        }
        
        // Copy from the buffer into the data block until block or buf is done
        while(idx < BLOCK_SIZE && bufOffset < len) {
            data_buffer[idx++] = buf[bufOffset++];
            file->offset += 1;
            node.nBytes += 1;
        }
        
        // Write the data buffer out to disk
        ret = disks[devID].writeBlock(block, data_buffer, disks[devID].driverNr); 
        if(ret < 0) {
            __cio_printf( "*ERROR* in _fs_write: Unable to write block %d to disk (%d)\n", 
                block, ret);
            return E_FAILURE;
        }
    }
    
    // Write the updated inode to disk
    ret = _fs_setInode(node);
    if(ret < 0) {
        __cio_printf("*ERROR* in _fs_read: Failed to write inode to disk %d.%d (%d)\n", 
            node.id.devID, node.id.idx, ret);
        return E_FAILURE;
    }
    
    // Return the number of bytes written
    return bufOffset;
}

int _fs_kRead(inode_id_t id, int offset, char* buf, int bufSize) {
    uint8_t devID = id.devID;
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
        __cio_printf("*ERROR* in _fs_kRead: Cannot find disk %d\n", id.devID);
        return E_BAD_CHANNEL;
    }
    
    // Read in inode for file! 
    inode_t node;
    int ret = _fs_getInode(id, &node);
    if(ret < 0) {
        __cio_printf("*ERROR* in _fs_kRead: Failed to read inode %d.%d (%d)\n", 
            id.devID, id.idx, ret);
        return ret;
    }
    

    // Return EOF on EOF
    if(offset == node.nBytes) {
        return E_EOF;
    }

    
    for(bytes_read = 0; bytes_read < bufSize && offset < node.nBytes;) {
        // Calculate the entry number of the next block
        uint32_t blockIdx = offset / BLOCK_SIZE;

        // Exit early if block is indirect (4 * nPtrBlocks)
        if(blockIdx >= NUM_DIRECT_POINTERS * 4) {
            return bytes_read;
        }

        // Get next data block's offset
        data_u data;
        ret = _fs_getNodeEnt(&node, blockIdx / 4, &data);
        if(ret < 0) {
            __cio_printf("*ERROR* in _fs_read: Failed to read node entry %d (%d)\n", 
                blockIdx/4, ret);
        }
        block_t block = data.blocks[blockIdx % 4];

        // Read the block in to the data buffer
        ret = disks[devID].readBlock(block, data_buffer, disks[devID].driverNr);
        if(ret < 0) {
            __cio_printf( "*ERROR* in _fs_read: Unable to read block %d from disk (%d)\n", block, ret);
            return ret;
        }

        // Read bytes until buffer full, file done, or block end
        int idx = offset % BLOCK_SIZE; // Calculate the offset into the current block
        while(bytes_read < bufSize && offset < node.nBytes && idx < BLOCK_SIZE) { 
            buf[bytes_read++] = data_buffer[idx++];
            offset += 1;
        }
    }
    return bytes_read;
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
        __cio_printf("*ERROR* in _fs_getInode: Non-existent node %d.%d\n", id.devID, id.idx);
        return E_BAD_CHANNEL;
    }
    
    uint32_t disk = 0;
    for(; disk < MAX_DISKS; disk++) {
        if(disks[disk].fsNr == id.devID) {
            break;
        }
    }
    if (disk == MAX_DISKS) {
        __cio_printf("*ERROR* in _fs_getInode: Unable to find disk %d\n", id.devID);
        return E_BAD_CHANNEL; // Ensure disk exists
    }
    
    // Read the metadata node from disk
    int result = disks[disk].readBlock(0, meta_buffer, disks[disk].driverNr);
    if(result < 0) return result;   // Ensure meta read was successful
    inode_t metaNode = *(inode_t*)(meta_buffer);
    
    // Check that this inode exists on this disk
    if(id.idx >= metaNode.nRefs) return E_BAD_PARAM;
    
    // Read in the inode from disk
    uint32_t idx = id.idx/(BLOCK_SIZE/sizeof(inode_t));
    result = disks[disk].readBlock(idx, inode_buffer, disks[disk].driverNr);
    if(result < 0) return result;   // Ensure read was successful
    
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
 * Clears an inode on disk
 * 
 * @param id the id of the node to clear 
 * 
 * @return A standard exit status
 */
int _fs_clearInode(inode_id_t id) {
    uint32_t disk, inodeOffset;
    int ret;
    block_t inodeBlock;

    if(id.devID == 0) { // There is no device zero
        return E_BAD_CHANNEL;
    }
    
    // Determine the disk index
    for(disk = 0; disk < MAX_DISKS; disk++){
        if(disks[disk].fsNr == id.devID) {
            break;
        }
    }
    if(disk == MAX_DISKS) { // If disk not found, return failure status
        return E_BAD_CHANNEL;
    }
    
    // Load the inode block into memory
    inodeBlock = id.idx / (BLOCK_SIZE / sizeof(inode_t));
    ret = disks[disk].readBlock(inodeBlock, inode_buffer, disks[disk].driverNr);
    if(ret < 0) {
        return ret;
    }
    
    // Clear the buffer at inode
    inodeOffset = sizeof(inode_t) * (id.idx % (BLOCK_SIZE / sizeof(inode_t)));
    char * dst = inode_buffer + inodeOffset;
    for(int i = 0; i < sizeof(inode_t); i++) {
        dst[i] = 0;
    }
    
    // Write the updated buffer out to memory
    ret = disks[disk].writeBlock(inodeBlock, inode_buffer, disks[disk].driverNr);
    if(ret < 0) {
        return ret;
    }
    
    return E_SUCCESS;
}

/**
 * Helper function to return the `idx`th data entry from the passed inode 
 * (exposed)
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
        __cio_printf("*ERROR in _fs_getNodeEnt()* Passed index: %d not in DIR bounds\n", idx);
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
int _fs_setNodeEnt(inode_t* inode, int idx, data_u ent) {
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
    inode_t tgt, child;
    int ret;

    // Get the inode
    ret = _fs_getInode(inode, &tgt);
    if(ret < 0) {
        __cio_printf("ERROR in _fs_addDirEnt: Unable to read dir\n");
        return ret;
    }

    // Check that the grabbed inode is a directory
    if(tgt.nodeType != INODE_DIR_TYPE) {
        __cio_printf("ERROR in _fs_addDirEnt: Non-directory inode specified\n");
        return E_BAD_PARAM;
    }

    // todo Add support for extention blocks

    // Fail if out of direct pointers
    if(tgt.nBytes == NUM_DIRECT_POINTERS) {
        __cio_printf("ERROR in _fs_addDirEnt: Unable to add new dir entry\n");
        return E_FILE_LIMIT;
    }

    // Check for name overlap
    for(uint32_t i = 0; i < tgt.nBytes; i++) {
        data_u temp;
        bool_t matched;

        ret = _fs_getNodeEnt(&tgt, i, &temp);
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
            __cio_printf("ERROR in _fs_addDirEnt: %s already exists in DIR\n", name);
            return E_BAD_PARAM;
        }
    }

    // Create a new entry for this file (zero padded string)
    data_u ent;
    ent.dir.inode = buf;

    int i = 0;
    for(; name[i] != 0 && i < 12; i++) {
        ent.dir.name[i] = name[i];
    }
    for(; i < 12; i++) {
        ent.dir.name[i] = 0;
    }

    // Set the new node entry
    ret = _fs_setNodeEnt(&tgt, tgt.nBytes, ent);
    if(ret < 0) {
        return ret;
    }

    // Write the updated inode to disk
    ret = _fs_setInode(tgt);
    if(ret < 0) {
        __cio_printf("ERROR in _fs_addDirEnt: Unable to write inode to disk\n");
        return ret;
    }

    // Update referand reference count
    ret = _fs_getInode(buf, &child);
    if(ret < 0) {
        __cio_printf("ERROR in _fs_addDirEnt: Unable to read child inode from disk\n");
        return ret;
    }

    child.nRefs += 1;

    ret = _fs_setInode(child);
    if(ret < 0) {
        __cio_printf("ERROR in _fs_addDirEnt: Unable to write child inode to disk\n");
        return ret;
    }

    // Return success
    return E_SUCCESS;
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
    inode_id_t childId;
    inode_t tgt, child;
    int ret, idx;
    
    // Get the inode
    ret = _fs_getInode(inode, &tgt);
    if(ret < 0) {
        __cio_printf("ERROR in _fs_rmDirEnt*: Unable to read dir\n");
        return ret;
    }
    
    // Check that the grabbed inode is a directory
    if(tgt.nodeType != INODE_DIR_TYPE) {
        __cio_printf("ERROR in _fs_rmDirEnt*: Non-directory inode specified\n");
        return E_BAD_PARAM;
    }

    // Search for the matching directory entry
    for(idx = 0; idx < tgt.nBytes; idx++) {
        data_u ent;
        bool_t matched;

        // Grab the next entry from disk
        ret = _fs_getNodeEnt(&tgt, idx, &ent);
        if(ret < 0) {
            return ret;
        }

        matched = true;
        for(int i = 0; i < 12; i++) {
            if(ent.dir.name[i] == 0 && name[i] == 0) { // Match on 2 null strings
                break;
            }

            if(ent.dir.name[i] != name[i]) {
                matched = false;
                break;
            }
        }
        if(matched) { // If this is a match, break out
            childId = ent.dir.inode;
            break;
        }
    }
    if(idx == tgt.nBytes) { // Fail if no match was found
        __cio_printf("ERROR in _fs_rmDirEnt*: Entry \"%s\" not in tgt DIR\n", name );
        return E_BAD_PARAM;
    }
    
    // Grab the child from disk
    ret = _fs_getInode(childId, &child);
    if(ret < 0) {
        __cio_printf("ERROR in _fs_rmDirEnt*: Unable to read child %d.%d\n", childId.devID, childId.idx);
        return ret;
    }

    // Update child nRefs
    if(child.nRefs > 0) child.nRefs -= 1;
    // Either remove child or write child out to disk
    ret = (child.nRefs == 0) ? _fs_freeNode(childId) : _fs_setInode(child);
    if(ret < 0) {
        __cio_printf("ERROR in _fs_rmDirEnt*: Unable to update child %d.%d\n", childId.devID, childId.idx);
        return ret;
    }

    // Prepare a blank entry for update
    data_u blankEnt;
    for(int i = 0; i < sizeof(blankEnt); i++) {
        ((char*)(&blankEnt))[i] = 0;
    }

    // If this is not the final entry, fill its gap with the final entry
    if(idx != tgt.nBytes - 1) {
        data_u ent;
        ret = _fs_getNodeEnt(&tgt, tgt.nBytes - 1, &ent); // Grab the final entry
        if(ret < 0) return ret;

        ret = _fs_setNodeEnt(&tgt, idx, ent); // Use it to fill the emptied spot
        if(ret < 0) return ret;
    }

    // Blank the final entry and decrement the entry counter
    ret = _fs_setNodeEnt(&tgt, tgt.nBytes - 1, blankEnt);
    if(ret < 0) return ret;
    tgt.nBytes -= 1;

    // Write the updated inode to disk and return
    ret = _fs_setInode(tgt);
    if(ret < 0) return ret;

    return E_SUCCESS;
}

/**
 * Returns a particular inode entry (Exposed)
 * 
 * @param inode The inode to access
 * @param idx The index of the data entry to grab
 * @param entry A return pointer for the directory entry
 * 
 * @return A standard exit status
 */
int _fs_getDirEnt(inode_id_t inode, uint32_t idx, data_u* entry) {
    inode_t tgt;
    int ret;
    
    // Grab the inode from disk
    ret = _fs_getInode(inode, &tgt);
    if(ret < 0) {
        return ret;
    }

    // Call the internal handler and return as it does
    return _fs_getNodeEnt(&tgt, idx, entry);
}

/**
 * Returns the inode of a named directory entry (Exposed)
 * 
 * @param inode The inode to access
 * @param name The name of the entry to grab
 * @param entry A return pointer for the referenced inode address
 * 
 * @return A standard exit status
 */
int _fs_getSubDir(inode_id_t inode, char* name, inode_id_t * ret) {
    inode_t tgt;
    int rV;

    // Get the root inode
    rV = _fs_getInode(inode, &tgt);
    if(rV < 0) return rV;

    // Check that this is a directory inode
    if(tgt.nodeType != INODE_DIR_TYPE) {
        __cio_printf("*ERROR* in _fs_getSubDir(): Non-directory inode specified (%d.%d)\n", inode.devID, inode.idx);
        return E_BAD_PARAM;
    }

    // Search through the inode for a matching name
    for(int idx = 0; idx < tgt.nBytes; idx++) {
        // Grab the next entry
        data_u ent;
        rV = _fs_getNodeEnt(&tgt, idx, &ent);
        if(ret < 0) return rV;

        // Perform a comparison with the selected entry
        bool_t matched = true;
        for(int i = 0; i < 12; i++) {
            if(name[i] == 0 && ent.dir.name[i] == 0) {
                break;
            }

            if(name[i] != ent.dir.name[i]) {
                matched = false;
                break;
            }
        }
        if(matched) {
            *ret = ent.dir.inode;
            return E_SUCCESS;
        }

    }
    return E_FAILURE;
}

/**
 * Returns the index of a free inode (Exposed)
 * 
 * @param devID The device ID
 * @param ret   The inode_id_t we will fill
 * 
 * @return A standard exit status
 */
int _fs_allocNode(uint8_t devID, inode_id_t * ret) {
    inode_t metaNode, node;
    int result;

    // Set ret to target the metanode on this disk
    if(devID == 0) {
        ret->devID = disks[0].fsNr;
        if(ret->devID == 0) {
            __cio_printf("*ERROR* in _fs_allocNode: No default disk registered\n");
            return E_FAILURE;
        }
    } else {
        ret->devID = devID;
    }
    ret->idx = 0;

    // Read the metanode for disk
    result = _fs_getInode(*ret, &metaNode);
    if(result < 0) {
        __cio_printf( "*ERROR* in _fs_allocNode: Unable to read meta node for disk %d (%d)\n", devID, result);
        return result;
    }

    // Iterate through all non-basic (meta and root) nodes on disk
    for (ret->idx = 2; ret->idx < metaNode.nRefs; ret->idx += 1) {
        // Grab the next node from disk
        result = _fs_getInode(*ret, &node);
        if(result < 0) {
            __cio_printf( "*ERROR* in _fs_allocNode: Unable to read inode %d.%d\n", ret->devID, ret->idx);
            return result;
        }

        if(node.id.devID == 0) {
            return E_SUCCESS;
        }
    }

    // Was unable to find an inode on disk
    __cio_printf("*ERROR* in _fs_allocNode: Unable to alloc an inode on disk %d\n", ret->devID);
    return E_FAILURE;
}

/**
 * Frees the specified inode (if possible) (Exposed)
 * 
 * @param id The id of the inode to free
 * 
 * @return A standard exit status (<0 on failure to free)
 */
int _fs_freeNode(inode_id_t id) {
    inode_t node;
    int ret;

    // Read the specified inode from disk
    ret = _fs_getInode(id, &node);
    if(ret < 0) {
        __cio_printf( "*ERROR* in _fs_freeNode: Unable to read inode %d.%d\n", id.devID, id.idx);
        return ret;
    }

    // Fail on directory with children
    if(node.nodeType == INODE_DIR_TYPE && node.nBytes == 0) {
        __cio_printf( "*ERROR* in _fs_freeNode: Unable to free non-empty directory\n", id.devID, id.idx);
        return ret;
    }

    // Free indirect blocks associated with this node
    //todo free indirect blocks

    // Free all direct blocks associated with this node
    if(node.nodeType == INODE_FILE_TYPE) {
        for(int i = 0; i < node.nBlocks; i++) {
            data_u data;
            block_t block;

            // Get the index of the data block
            ret = _fs_getNodeEnt(&node, i/4, &data);
            if(ret < 0) {
                __cio_printf( "*ERROR* in _fs_freeNode: Unable to free data block %d (non-fatal)\n", i);
                continue;
            }
            block = data.blocks[i%4];

            // Free this block
            ret = _fs_free_block(node.id.devID, block);
            if(ret < 0) {
                __cio_printf( "*ERROR* in _fs_freeNode: Unable to free data block %d (non-fatal)\n", i);
                continue;
            }
        }
    }
    node.nBlocks = 0;

    // Clear this inode on disk
    ret = _fs_clearInode(id);
    if(ret < 0) {
        __cio_printf( "*ERROR* in _fs_freeNode: Unable to write inode %d.%d to disk\n", id.devID, id.idx);
        return ret;
    }

    return E_SUCCESS;
}


/**
 * Checks for permissions on the specified inode
 * 
 * @param id The id of the inode to check
 * @param uid The uid of the accessor
 * @param gid The gid of the accessor
 * @param canRead A return pointer for read permission status
 * @param canWrite A return pointer for write permission status
 * @param canMeta A return pointer for meta (i.e. change ownership/permissions) permission status
 * 
 * @return A standard exit status (<0 on failure)
 */
int _fs_getPermission(inode_id_t id, uid_t uid, gid_t gid, bool_t * canRead, bool_t * canWrite, bool_t * canMeta) {
    int ret;
    inode_t node;

    ret = _fs_getInode(id, &node);
    if(ret < 0) {
        __cio_printf("*ERROR* in _fs_getPermission: Failed to read inode (%d)\n", ret);
        return E_FAILURE;
    }

    return _fs_nodePermission(&node, uid, gid, canRead, canWrite, canMeta);
}

/**
 * Checks for permissions on the passed inode
 * 
 * @param node The inode to check
 * @param uid The uid of the accessor
 * @param gid The gid of the accessor
 * @param canRead A return pointer for read permission status
 * @param canWrite A return pointer for write permission status
 * @param canMeta A return pointer for meta (i.e. change ownership/permissions) permission status
 * 
 * @return A standard exit status (<0 on failure)
 */
int _fs_nodePermission(inode_t * node, uid_t uid, gid_t gid, bool_t * canRead, bool_t * canWrite, bool_t * canMeta) {
    bool_t back;    // Safe referand to handle null return pointers
    if(canRead == NULL) canRead = &back;
    if(canWrite == NULL) canWrite = &back;
    if(canMeta == NULL) canMeta = &back;

    // First check for bypass UID or GID
    if(uid == UID_ROOT || gid == GID_SUDO) {
        *canRead = true;
        *canWrite = true;
        *canMeta = true;

        return E_SUCCESS;
    }

    // Set meta iff this is the owner
    *canMeta = (uid == node->uid);

    // First consider general permissions
    *canWrite = (node->permissions & 0x20) ? true : false;
    *canRead = (node->permissions & 0x10) ? true : false;

    // Next consider group permissions (retaining pervious setting)
    if(node->gid == 0) { // Check for user group first
        if(node->uid == uid) {
            *canWrite |= (node->permissions & 0x08) ? true : false;
            *canRead |= (node->permissions & 0x04) ? true : false;
        }
    } else if(node->gid == gid) {
        *canWrite |= (node->permissions & 0x08) ? true : false;
        *canRead |= (node->permissions & 0x04) ? true : false;
    }

    // Finally consider user permissions
    if(node->uid == uid) {
        *canWrite |= (node->permissions & 0x02) ? true : false;
        *canRead |= (node->permissions & 0x01) ? true : false;
    }

    return E_SUCCESS;
}
