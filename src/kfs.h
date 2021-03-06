
#ifndef  KFS_H_
#define  KFS_H_

#define SP_KERNEL_SRC
#include "common.h"
#include "process.h"
#include "driverInterface.h"

#define MAX_DISKS 10

void _fs_init(void);

/**
 * Register a device for read and write
 * 
 * @param interface The proper interface for this device
 * 
 * @returns The number this was registered as (error if return < 0)
 */
int _fs_registerDev(driverInterface_t interface);

/**
 * FS read handler
 * 
 * @param file The file descriptor to read from
 * @param buf The character buffer to read into
 * @param len The max number of characters to read into the buffer
 * 
 * @returns The number of bytes read from disk
 */
int _fs_read(fd_t * file, char * buf, uint32_t len);

/**
 * FS write handler
 * 
 * @param file The file descriptor to write to
 * @param buf The character buffer to write from
 * @param len The max number of characters to write from the buffer
 * 
 * @returns The number of bytes read from disk
 */
int _fs_write(fd_t * file, char * buf, uint32_t len);

int _fs_kRead(inode_id_t id, int offset, char* buf, int bufSize);

/**
 * Reads an inode from disk
 * 
 * @param id The inode id to access
 * @param inode A return pointer for the grabbed inode
 * 
 * @return A standard exit status
 */
int _fs_getInode(inode_id_t id, inode_t * inode);

/**
 * Writes an inode to disk
 * 
 * @param inode the inode to write to disk 
 * 
 * @return A standard exit status
 */
int _fs_setInode(inode_t inode);

/**
 * Helper function to return the `idx`th data entry from the passed inode
 * 
 * @param inode The inode to grab data entries from
 * @param idx The index of the entry to grab
 * @param ret A return pointer for the grabbed node entry
 * 
 * @return A standard exit status
 */
int _fs_getNodeEnt(inode_t* inode, int idx, data_u * ret);

/**
 * Adds an entry to a directory inode
 * 
 * @param inode The inode to update
 * @param name The name to associate with this entry
 * @param buf The new target inode
 * 
 * @return A standard exit status
 */
int _fs_addDirEnt(inode_id_t inode, const char* name, inode_id_t buf);

/**
 * Removes an entry from a directory inode
 * 
 * @param inode The inode to update
 * @param name The entry name to remove
 * 
 * @return A standard exit status
 */
int _fs_rmDirEnt(inode_id_t inode, const char* name);

/**
 * Returns the data from a particular directory entry
 * 
 * @param inode The inode to access
 * @param idx The index of the data entry to grab
 * @param entry A return pointer for the directory entry
 * 
 * @return A standard exit status
 */
int _fs_getDirEnt(inode_id_t inode, uint32_t idx, data_u* entry);

/**
 * Returns the inode of a named directory entry
 * 
 * @param inode The inode to access
 * @param name The name of the entry to grab
 * @param entry A return pointer for the referenced inode address
 * 
 * @return A standard exit status
 */
int _fs_getSubDir(inode_id_t inode, char* name, inode_id_t * ret);

/**
 * Returns the index of a free inode
 *
 * @param devID the device ID
 * @param ret   the inode_id_t we will fill
 *
 * @return A standard exit status
 */
int _fs_allocNode(uint8_t devID, inode_id_t * ret);

/**
 * Frees the specified inode (if possible)
 * 
 * @param id The id of the inode to free
 * 
 * @return A standard exit status (<0 on failure to free)
 */
int _fs_freeNode(inode_id_t id);

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
int _fs_getPermission(inode_id_t id, uid_t uid, gid_t gid, bool_t * canRead, bool_t * canWrite, bool_t * canMeta);

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
int _fs_nodePermission(inode_t * node, uid_t uid, gid_t gid, bool_t * canRead, bool_t * canWrite, bool_t * canMeta);

#endif //KFS_H_
