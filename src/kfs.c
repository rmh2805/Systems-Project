#include "kfs.h"

static driverInterface_t disks[MAX_DISKS];

void _fs_init( void ) {
    __cio_puts( " FS:" );

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

    disks[nextDisk] = interface
    return nextDisk;
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
    
    uint8_t devID = fd_t.inode_id.devID;
    // Read in inode for file! 
    disks[devID].readBlock(fd_t

    // Read in each direct pointers data into our buffer! 
    

    return E_FAILURE;
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
