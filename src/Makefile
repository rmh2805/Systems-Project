#
# SCCS ID: @(#)Makefile	2.1	12/8/19
#
# Makefile to control the compiling, assembling and linking of standalone
# programs in the DSL.  Used for both individual interrupt handling
# assignments and the SP baseline OS (with appropriate tweaking).
#

#
# Application files
#

OS_C_SRC = clock.c kernel.c klibc.c kmem.c process.c queues.c \
	scheduler.c sio.c stacks.c syscalls.c kfs.c ramDiskDriver.c pci.c disk.c
OS_C_OBJ = clock.o kernel.o klibc.o kmem.o process.o queues.o \
	scheduler.o sio.o stacks.o syscalls.o kfs.o ramDiskDriver.o pci.o disk.o

OS_S_SRC = klibs.S
OS_S_OBJ = klibs.o

OS_LIBS =

OS_SRCS = $(OS_C_SRC) $(OS_S_SRC)
OS_OBJS = $(OS_C_OBJ) $(OS_S_OBJ)

USR_C_SRC = users.c ulibc.c
USR_C_OBJ = users.o ulibc.o

USR_S_SRC = ulibs.S
USR_S_OBJ = ulibs.o

USR_LIBS =

USR_SRCS = $(USR_C_SRC) $(USR_S_SRC)
USR_OBJS = $(USR_C_OBJ) $(USR_S_OBJ)

#
# Framework files
#

FMK_S_SRC = startup.S isr_stubs.S
FMK_S_OBJ = startup.o isr_stubs.o

FMK_C_SRC = cio.c support.c
FMK_C_OBJ = cio.o support.o

BOOT_SRC = bootstrap.S
BOOT_OBJ = bootstrap.o

FMK_SRCS = $(FMK_S_SRC) $(FMK_C_SRC)
FMK_OBJS = $(FMK_S_OBJ) $(FMK_C_OBJ)

# Collections of files

OBJECTS = $(FMK_OBJS) $(OS_OBJS) $(USR_OBJS)

SOURCES = $(BOOT_SRC) $(FMK_SRCS) $(OS_SRCS) $(USR_SRCS)

#
# Compilation/assembly definable options
#
# General options:
#	CLEAR_BSS		include code to clear all BSS space
#	GET_MMAP		get BIOS memory map via int 0x15 0xE820
#	SP_OS_CONFIG		enable SP OS-specific startup variations
#
# Debugging options:
#	DEBUG_KMALLOC		debug the kernel allocator code
#	DEBUG_KMALLOC_FREELIST	debug the freelist creation
#	DEBUG_UNEXP_INTS	debug any 'unexpected' interrupts
#	REPORT_MYSTERY_INTS	print a message on interrupt 0x27 specifically
#	TRACE_CX		include context restore trace code
#	TRACE=n			enable general internal tracing options
#	SANITY=n                enable "sanity check" level 'n'
#         0                     absolutely critical errors only
#         1                     important consistency checking
#         2                     less important consistency checking
#         3                     currently unused
#         4                     currently unused
#       STATUS=n                dump queue & process info every 'n' seconds
#

GEN_OPTIONS = -DCLEAR_BSS -DGET_MMAP -DSP_OS_CONFIG
DBG_OPTIONS = -DTRACE_CX -DCONSOLE_SHELL -DDEBUG_UNEXP_INTS

USER_OPTIONS = $(GEN_OPTIONS) $(DBG_OPTIONS)

OS_LOAD = 0x14000
FS_LOAD = 0x0bc00

#
# YOU SHOULD NOT NEED TO CHANGE ANYTHING BELOW THIS POINT!!!
#
# Compilation/assembly control
#

#
# We only want to include from the current directory
#
INCLUDES = -I.

#
# Compilation/assembly/linking commands and options
#
CPP = cpp
CPPFLAGS = $(USER_OPTIONS) -nostdinc $(INCLUDES)

#
# Compiler/assembler/etc. settings for 32-bit binaries
#
CC = gcc
CFLAGS = -m32 -fno-pie -std=c99 -fno-stack-protector -fno-builtin -Wall -Wstrict-prototypes $(CPPFLAGS)

AS = as
ASFLAGS = --32

LD = ld
LDFLAGS = -melf_i386 -no-pie

#		
# Transformation rules - these ensure that all compilation
# flags that are necessary are specified
#
# Note use of 'cpp' to convert .S files to temporary .s files: this allows
# use of #include/#define/#ifdef statements. However, the line numbers of
# error messages reflect the .s file rather than the original .S file. 
# (If the .s file already exists before a .S file is assembled, then
# the temporary .s file is not deleted.  This is useful for figuring
# out the line numbers of error messages, but take care not to accidentally
# start fixing things by editing the .s file.)
#
# The .c.X rule produces a .X file which contains the original C source
# code from the file being compiled mixed in with the generated
# assembly language code.  Very helpful when you need to figure out
# exactly what C statement generated which assembly statements!
#

.SUFFIXES:	.S .b .X

.c.X:
	$(CC) $(CFLAGS) -g -c -Wa,-adhln $*.c > $*.X

.c.s:
	$(CC) $(CFLAGS) -S $*.c

.S.s:
	$(CPP) $(CPPFLAGS) -o $*.s $*.S

.S.o:
	$(CPP) $(CPPFLAGS) -o $*.s $*.S
	$(AS) $(ASFLAGS) -o $*.o $*.s -a=$*.lst
	$(RM) -f $*.s

.s.b:
	$(AS) $(ASFLAGS) -o $*.o $*.s -a=$*.lst
	$(LD) $(LDFLAGS) -Ttext 0x0 -s --oformat binary -e begtext -o $*.b $*.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

#
# Targets for remaking bootable image of the program
#
# Default target:  usb.image
#

usb.image: bootstrap.b prog.b prog.nl BuildImage prog.dis
	./BuildImage -d usb -o usb.image -b bootstrap.b fs.img $(FS_LOAD) prog.b $(OS_LOAD)

floppy.image: bootstrap.b prog.b prog.nl BuildImage prog.dis 
	./BuildImage -d floppy -o floppy.image -b bootstrap.b fs.img $(FS_LOAD) prog.b $(OS_LOAD)

prog.out: $(OBJECTS)
	$(LD) $(LDFLAGS) -o prog.out $(OBJECTS)

prog.o:	$(OBJECTS)
	$(LD) $(LDFLAGS) -o prog.o -Ttext $(OS_LOAD) $(OBJECTS) $(A_LIBS)

prog.b:	prog.o
	$(LD) $(LDFLAGS) -o prog.b -s --oformat binary -Ttext $(OS_LOAD) prog.o

fs.img: MakeFSImage AddFile .shadow .groups testLongNames.txt 
	./MakeFSImage fs.dmg 16 10 1 disk 2 
	./AddFile fs.dmg .shadow fs.img n n
	./AddFile fs.img testLongNames.txt fs.dmg y y
	./AddFile fs.dmg .groups fs.img y n
	rm fs.dmg

disk.img: MakeFSImage AddFile
	./MakeFSImage disk.dmg 128 30 2 disk2 3
	./AddFile disk.dmg placeholder disk.img y y
	rm disk.dmg

disk2.img: MakeFSImage
	./MakeFSImage disk2.img 128 30 3 

#
# Targets for copying bootable image onto boot devices
#

floppy:	floppy.image
	dd if=floppy.image of=/dev/fd0

usb:	usb.image
	/usr/local/dcs/bin/dcopy usb.image

#
# Special rule for creating the auxiliary programs
#
# These are required because we don't want to use the same options
# as for the standalone binaries.
#

BuildImage:	BuildImage.c
	$(CC) -o BuildImage BuildImage.c

Offsets:	Offsets.c
	$(CC) -mx32 -std=c99 $(INCLUDES) -o Offsets Offsets.c

MakeFSImage: MakeFSImage.c
	$(CC) -mx32 -std=c99 $(INCLUDES) -o MakeFSImage MakeFSImage.c

AddFile: AddFile.c
	$(CC) -mx32 -std=c99 $(INCLUDES) -ggdb -o AddFile AddFile.c

#
# Clean out this directory
#

clean:
	rm -f *.nl *.nll *.lst *.b *.o *.X *.image *.dis BuildImage Offsets MakeFSImage AddFile

realclean:	clean

qemu: usb.image
	qemu-system-x86_64 -serial mon:stdio -drive format=raw,id=usb,file=usb.image \
	-drive format=raw,id=disk,file='disk.img',if=none \
	-drive format=raw,id=fs,file='disk2.img',if=none \
	-device ide-drive,drive=disk,bus=ide.0 \
	-device ide-drive,drive=fs,bus=ide.1
	@echo ""

qemu-nox: usb.image
	qemu-system-x86_64 -nographic usb.image
	@echo ""

#
# Create a printable namelist from the prog.o file
#

prog.nl: prog.o
	nm -Bng prog.o | pr -w80 -3 > prog.nl

prog.nll: prog.o
	nm -Bn prog.o | pr -w80 -3 > prog.nll

#
# Generate a disassembly
#

prog.dis: prog.o
	objdump -d prog.o > prog.dis

#
# 'makedepend' is a program which creates dependency lists by looking
# at the #include lines in the source files.
#

depend:
	makedepend $(INCLUDES) $(SOURCES)

# DO NOT DELETE THIS LINE -- make depend depends on it.

bootstrap.o: bootstrap.h
startup.o: bootstrap.h
isr_stubs.o: bootstrap.h
cio.o: cio.h klib.h common.h fs.h kdefs.h kmem.h compat.h support.h kernel.h
cio.o: x86arch.h process.h stacks.h queues.h kfs.h driverInterface.h x86pic.h
support.o: support.h klib.h common.h fs.h kdefs.h cio.h kmem.h compat.h
support.o: kernel.h x86arch.h process.h stacks.h queues.h kfs.h
support.o: driverInterface.h x86pic.h bootstrap.h
clock.o: x86arch.h x86pic.h x86pit.h common.h fs.h kdefs.h cio.h kmem.h
clock.o: compat.h support.h kernel.h process.h stacks.h queues.h kfs.h
clock.o: driverInterface.h klib.h clock.h scheduler.h
kernel.o: common.h fs.h kdefs.h cio.h kmem.h compat.h support.h kernel.h
kernel.o: x86arch.h process.h stacks.h queues.h kfs.h driverInterface.h
kernel.o: klib.h clock.h bootstrap.h syscalls.h sio.h ramDiskDriver.h
kernel.o: scheduler.h disk.h pci.h users.h
klibc.o: common.h fs.h kdefs.h cio.h kmem.h compat.h support.h kernel.h
klibc.o: x86arch.h process.h stacks.h queues.h kfs.h driverInterface.h klib.h
kmem.o: common.h fs.h kdefs.h cio.h kmem.h compat.h support.h kernel.h
kmem.o: x86arch.h process.h stacks.h queues.h kfs.h driverInterface.h klib.h
kmem.o: bootstrap.h
process.o: common.h fs.h kdefs.h cio.h kmem.h compat.h support.h kernel.h
process.o: x86arch.h process.h stacks.h queues.h kfs.h driverInterface.h
process.o: klib.h bootstrap.h scheduler.h
queues.o: common.h fs.h kdefs.h cio.h kmem.h compat.h support.h kernel.h
queues.o: x86arch.h process.h stacks.h queues.h kfs.h driverInterface.h
queues.o: klib.h
scheduler.o: common.h fs.h kdefs.h cio.h kmem.h compat.h support.h kernel.h
scheduler.o: x86arch.h process.h stacks.h queues.h kfs.h driverInterface.h
scheduler.o: klib.h syscalls.h
sio.o: common.h fs.h kdefs.h cio.h kmem.h compat.h support.h kernel.h
sio.o: x86arch.h process.h stacks.h queues.h kfs.h driverInterface.h klib.h
sio.o: ./uart.h x86pic.h sio.h scheduler.h
stacks.o: common.h fs.h kdefs.h cio.h kmem.h compat.h support.h kernel.h
stacks.o: x86arch.h process.h stacks.h queues.h kfs.h driverInterface.h
stacks.o: klib.h
syscalls.o: common.h fs.h kdefs.h cio.h kmem.h compat.h support.h kernel.h
syscalls.o: x86arch.h process.h stacks.h queues.h kfs.h driverInterface.h
syscalls.o: klib.h x86pic.h ./uart.h bootstrap.h syscalls.h scheduler.h
syscalls.o: clock.h sio.h
kfs.o: kfs.h common.h fs.h kdefs.h cio.h kmem.h compat.h support.h kernel.h
kfs.o: x86arch.h process.h stacks.h queues.h klib.h driverInterface.h
ramDiskDriver.o: ramDiskDriver.h common.h fs.h kdefs.h cio.h kmem.h compat.h
ramDiskDriver.o: support.h kernel.h x86arch.h process.h stacks.h queues.h
ramDiskDriver.o: kfs.h driverInterface.h klib.h
pci.o: common.h fs.h kdefs.h cio.h kmem.h compat.h support.h kernel.h
pci.o: x86arch.h process.h stacks.h queues.h kfs.h driverInterface.h klib.h
pci.o: pci.h
disk.o: common.h fs.h kdefs.h cio.h kmem.h compat.h support.h kernel.h
disk.o: x86arch.h process.h stacks.h queues.h kfs.h driverInterface.h klib.h
disk.o: pci.h disk.h clock.h
users.o: common.h fs.h kdefs.h cio.h kmem.h compat.h support.h kernel.h
users.o: x86arch.h process.h stacks.h queues.h kfs.h driverInterface.h klib.h
users.o: users.h userland/main1.c userland/main2.c userland/main3.c
users.o: userland/main4.c userland/main5.c userland/main6.c userland/userH.c
users.o: userland/userI.c userland/userJ.c userland/userP.c userland/userQ.c
users.o: userland/userR.c userland/userS.c userland/userV.c userland/userW.c
users.o: userland/userX.c userland/userY.c userland/userZ.c userland/mUser1.c
users.o: userland/mUser2.c userland/mUser3.c userland/mUser4.c
users.o: userland/testFS1.c userland/testFS2.c userland/testFS3.c
users.o: userland/testFS4.c userland/testFS5.c userland/testFS6.c
users.o: userland/testFS7.c userland/init.c userland/doTests.c
users.o: userland/idle.c userland/signIn.c userland/testShell.c
users.o: userland/cat.c userland/ls.c userland/chmod.c userland/ap.c
ulibc.o: common.h fs.h kdefs.h cio.h kmem.h compat.h support.h kernel.h
ulibc.o: x86arch.h process.h stacks.h queues.h kfs.h driverInterface.h klib.h
ulibs.o: syscalls.h common.h fs.h kdefs.h cio.h kmem.h compat.h support.h
ulibs.o: kernel.h x86arch.h process.h stacks.h queues.h kfs.h
ulibs.o: driverInterface.h klib.h
