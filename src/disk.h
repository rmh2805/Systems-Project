/**
** @file disk.h
**
** @author Koen Komeya
**
** Disk protocol. See disk.c for explanation of this module.
*/

#ifndef DISK_H_
#define DISK_H_

/**
** _disk_init()
**
** Initialize the disk protocol.
** Requires kfs to be initialized prior.
*/
void _disk_init( void );

/**
** _disk_get_drive_count()
**
** Gets the amount of drives.
**
** @return        The number of drives found, otherwise negative if an error happened
*/
int _disk_get_drive_count( void );

/**
** _disk_get_drive_capacity(drive)
**
** Gets drive's capacity in sectors
**
** @param devId   The drive to measure
**
** @return        The capacity of the drive
*/
int _disk_get_drive_capacity( uint8_t drive );

/**
** _disk_read_block(block, buf, drive)
**
** Reads a block/sector from a given disk into a buffer.
** Implements the readBlock procedure of driverInterface_t.
**
** @param block   The block/sector to read from.
** @param buf     The buffer to read into.
** @param drive   The drive to read from
**
** @return        Zero on success, otherwise negative if an error happened
*/
int _disk_read_block( uint32_t block, char* buf, uint8_t drive );

/**
** _disk_write_block(block, buf, drive)
**
** Writes to a block/sector of a given disk from a buffer.
** Implements the writeBlock procedure of driverInterface_t.
**
** @param block   The block/sector to write to.
** @param buf     The buffer to write from.
** @param drive   The drive to write to.
**
** @return        Zero on success, otherwise negative if an error happened
*/
int _disk_write_block( uint32_t block, char* buf, uint8_t drive );


#endif

