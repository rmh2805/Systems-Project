/**
** @file disk.h
**
** @author Koen Komeya
**
** Disk protocol
*/

#ifndef DISK_H_
#define DISK_H_

#define _DISK_BAD_SECTOR 0x01
#define _DISK_SECTOR_USED 0x02

/**
** _disk_init()
**
** Initialize the disk protocol
*/
void _disk_init( void );

/**
** _disk_get_disk_driver()
**
** Initialize the disk protocol
*/
void _disk_get_disk_driver( int id, driverInterface_t *dri_buf );

/**
** _disk_get_disk_driver_count()
**
** Initialize the disk protocol
*/
int _disk_get_disk_driver_count( void );


#endif

