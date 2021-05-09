/**
** File:	users.c
**
** Author:	CSCI-452 class of 20205
**
** Contributor:
**
** Description:	User-level code.
*/

#include "common.h"
#include "users.h"

/*
** USER PROCESSES
**
** Each is designed to test some facility of the OS; see the users.h
** header file for a summary of which system calls are tested by
** each user function.
**
** Output from user processes is usually alphabetic.  Uppercase
** characters are "expected" output; lowercase are "erroneous"
** output.
**
** More specific information about each user process can be found in
** the header comment for that function (below).
**
** To spawn a specific user process, uncomment its SPAWN_x
** definition in the users.h header file.
*/

/*
** Prototypes for all vase user main routines (even ones that may not exist,
** for completeness)
*/

int32_t idle( uint32_t, uint32_t );

int32_t main1( uint32_t, uint32_t ); int32_t main2( uint32_t, uint32_t );
int32_t main3( uint32_t, uint32_t ); int32_t main4( uint32_t, uint32_t );
int32_t main5( uint32_t, uint32_t ); int32_t main6( uint32_t, uint32_t );

int32_t userA( uint32_t, uint32_t ); int32_t userB( uint32_t, uint32_t );
int32_t userC( uint32_t, uint32_t ); int32_t userD( uint32_t, uint32_t );
int32_t userE( uint32_t, uint32_t ); int32_t userF( uint32_t, uint32_t );
int32_t userG( uint32_t, uint32_t ); int32_t userH( uint32_t, uint32_t );
int32_t userI( uint32_t, uint32_t ); int32_t userJ( uint32_t, uint32_t );
int32_t userK( uint32_t, uint32_t ); int32_t userL( uint32_t, uint32_t );
int32_t userM( uint32_t, uint32_t ); int32_t userN( uint32_t, uint32_t );
int32_t userO( uint32_t, uint32_t ); int32_t userP( uint32_t, uint32_t );
int32_t userQ( uint32_t, uint32_t ); int32_t userR( uint32_t, uint32_t );
int32_t userS( uint32_t, uint32_t ); int32_t userT( uint32_t, uint32_t );
int32_t userU( uint32_t, uint32_t ); int32_t userV( uint32_t, uint32_t );
int32_t userW( uint32_t, uint32_t ); int32_t userX( uint32_t, uint32_t );
int32_t userY( uint32_t, uint32_t ); int32_t userZ( uint32_t, uint32_t );

int32_t mUser1(uint32_t, uint32_t ); int32_t mUser2(uint32_t, uint32_t );
int32_t mUser3(uint32_t, uint32_t ); int32_t mUser4(uint32_t, uint32_t );

int32_t testFS1(uint32_t, uint32_t); int32_t testFS2(uint32_t, uint32_t);
int32_t testFS3(uint32_t, uint32_t); int32_t testFS4(uint32_t, uint32_t);
int32_t testFS5(uint32_t, uint32_t); int32_t testFS6(uint32_t, uint32_t);
int32_t testFS7(uint32_t, uint32_t);

int32_t signIn(uint32_t, uint32_t ); int32_t testShell(uint32_t , uint32_t );
int32_t cat(uint32_t, uint32_t);     int32_t ls(uint32_t, uint32_t);

/*
** The user processes
**
** We #include the source code from the userland/ directory only if
** a specific process is being spawned.
**
** Remember to #include the code required by any process that will
** be spawned - e.g., userH spawns userZ.  The user code files all
** contain CPP include guards, so multiple inclusion of a source
** file shouldn't cause problems.
*/
#include "userland/main1.c"
#include "userland/main2.c"
#include "userland/main3.c"
#include "userland/main4.c"
#include "userland/main5.c"
#include "userland/main6.c"

#include "userland/userH.c"
#include "userland/userI.c"
#include "userland/userJ.c"
#include "userland/userP.c"
#include "userland/userQ.c"
#include "userland/userR.c"
#include "userland/userS.c"
#include "userland/userV.c"
#include "userland/userW.c"
#include "userland/userX.c"
#include "userland/userY.c"
#include "userland/userZ.c"

#include "userland/mUser1.c"
#include "userland/mUser2.c"
#include "userland/mUser3.c"
#include "userland/mUser4.c"

#include "userland/testFS1.c"
#include "userland/testFS2.c"
#include "userland/testFS3.c"
#include "userland/testFS4.c"
#include "userland/testFS5.c"
#include "userland/testFS6.c"
#include "userland/testFS7.c"

/*
** System processes - these should always be included here
*/

#include "userland/init.c"
#include "userland/idle.c"

#include "userland/signIn.c"
#include "userland/testShell.c"

#include "userland/cat.c"
#include "userland/ls.c"
