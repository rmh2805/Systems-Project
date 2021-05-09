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


#endif

