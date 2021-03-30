/*
** SCCS ID:	@(#)Offsets.c	1.1	4/2/19
**
** File:	Offsets.c
**
** Author:		Warren R. Carithers
**
** Description:		Print byte offsets for fields in various structures.
**
** This program exists to simplify life.  If/when fields in a structure are
** changed, this can be modified, recompiled and executed to come up with
** byte offsets for use in accessing structure fields from assembly language.
**
** IMPORTANT NOTE:  compiling this on a 64-bit architecture will yield
** incorrect results by default, as 64-bit GCC versions most often use
** the LP64 model (longs and pointers are 64 bits).  Add the "-mx32"
** option to the compiler (compile for x86_64, but use 32-bit sizes),
** and make sure you have the 'libc6-dev-i386' package installed (for
** Ubuntu systems).
*/

#define	SP_KERNEL_SRC

#include "common.h"

// avoid complaints about stdio.h
#undef NULL

#include "process.h"
#include "stacks.h"

#include <stdio.h>

context_t context;
pcb_t pcb;
stack_t stack;

int main( void ) {

    puts( "Sizes of basic types:" );
    printf( "\tchar %u short %u int %u long %u long long %u\n",
        sizeof(char), sizeof(short), sizeof(int),
	sizeof(long), sizeof(long long) );
    putchar( '\n');

    puts( "Sizes of our types:" );
    printf( "\tint8_t  %u uint8_t  %u", sizeof(int8_t), sizeof(uint8_t) );
    printf( "\tint16_t %u uint16_t %u\n", sizeof(int16_t), sizeof(uint16_t) );
    printf( "\tint32_t %u uint32_t %u", sizeof(int32_t), sizeof(uint32_t) );
    printf( "\tint64_t %u uint64_t %u\n", sizeof(int64_t), sizeof(uint64_t) );
    printf( "\tstack_t %u\n", sizeof(stack_t) );
    putchar( '\n');

    printf( "Offsets into context_t (%u bytes):\n", sizeof(context) );
    printf( "   ss:\t\t%d\n", (char *)&context.ss - (char *)&context );
    printf( "   gs:\t\t%d\n", (char *)&context.gs - (char *)&context );
    printf( "   fs:\t\t%d\n", (char *)&context.fs - (char *)&context );
    printf( "   es:\t\t%d\n", (char *)&context.es - (char *)&context );
    printf( "   ds:\t\t%d\n", (char *)&context.ds - (char *)&context );
    printf( "   edi:\t\t%d\n", (char *)&context.edi - (char *)&context );
    printf( "   esi:\t\t%d\n", (char *)&context.esi - (char *)&context );
    printf( "   ebp:\t\t%d\n", (char *)&context.ebp - (char *)&context );
    printf( "   esp:\t\t%d\n", (char *)&context.esp - (char *)&context );
    printf( "   ebx:\t\t%d\n", (char *)&context.ebx - (char *)&context );
    printf( "   edx:\t\t%d\n", (char *)&context.edx - (char *)&context );
    printf( "   ecx:\t\t%d\n", (char *)&context.ecx - (char *)&context );
    printf( "   eax:\t\t%d\n", (char *)&context.eax - (char *)&context );
    printf( "   vector:\t%d\n",(char *)&context.vector - (char *)&context);
    printf( "   code:\t%d\n", (char *)&context.code - (char *)&context );
    printf( "   eip:\t\t%d\n", (char *)&context.eip - (char *)&context );
    printf( "   cs:\t\t%d\n", (char *)&context.cs - (char *)&context );
    printf( "   eflags:\t%d\n",(char *)&context.eflags - (char *)&context);
    putchar( '\n' );

    printf( "Offsets into pcb_t (%u bytes):\n", sizeof(pcb) );
    printf( "   context:\t%d\n", (char *)&pcb.context - (char *)&pcb );
    printf( "   stack:\t%d\n", (char *)&pcb.stack - (char *)&pcb );
    printf( "   exit_status:\t%d\n", (char *)&pcb.exit_status - (char *)&pcb );
    printf( "   event:\t%d\n", (char *)&pcb.event - (char *)&pcb );
    printf( "   pid:\t\t%d\n", (char *)&pcb.pid - (char *)&pcb );
    printf( "   ppid:\t%d\n", (char *)&pcb.ppid - (char *)&pcb );

    printf( "   state:\t%d\n", (char *)&pcb.state - (char *)&pcb );
    printf( "   priority:\t%d\n", (char *)&pcb.priority - (char *)&pcb );
    printf( "   quantum:\t%d\n",(char *)&pcb.quantum - (char *)&pcb);
    printf( "   ticks:\t%d\n",(char *)&pcb.ticks - (char *)&pcb);
    printf( "   filler:\t%d\n",(char *)&pcb.filler - (char *)&pcb);

    return( 0 );
}
