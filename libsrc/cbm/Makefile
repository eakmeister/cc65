#
# makefile for CC65 runtime library
#

.SUFFIXES: .o .s .c

#--------------------------------------------------------------------------
# Programs and flags

SYS	= none

AS	= ../../src/ca65/ca65
CC	= ../../src/cc65/cc65
LD	= ../../src/ld65/ld65

AFLAGS	= -t $(SYS) --forget-inc-paths -I../../asminc
CFLAGS	= -Osir -g -T -t $(SYS) --forget-inc-paths -I . -I ../../include

#--------------------------------------------------------------------------
# Rules

%.o:	%.c
	@$(CC) $(CFLAGS) $<
	@$(AS) -o $@ $(AFLAGS) $(*).s

%.o:	%.s
	@$(AS) -g -o $@ $(AFLAGS) $<

#--------------------------------------------------------------------------
# Object files

C_OBJS =	cbm_dir.o	\
		cbm_load.o	\
		cbm_save.o	\
		opendir.o	\
		readdir.o	\
		seekdir.o

S_OBJS =	c_acptr.o	\
		c_basin.o	\
		c_bsout.o	\
		c_chkin.o	\
		c_ciout.o	\
		c_ckout.o	\
		c_clall.o	\
		c_close.o	\
		c_clrch.o	\
		c_getin.o	\
		c_iobase.o	\
		c_listen.o	\
		c_load.o	\
		c_open.o	\
		c_readst.o	\
		c_save.o	\
		c_setlfs.o	\
		c_setnam.o	\
		c_talk.o	\
		c_unlsn.o	\
		c_untlk.o	\
		cbm_close.o	\
		cbm_filetype.o	\
		cbm_open.o	\
		cbm_read.o	\
		cbm_write.o	\
		cclear.o	\
		chline.o	\
		clock.o		\
		close.o		\
		closedir.o	\
		ctype.o		\
		curdevice.o	\
		cvline.o	\
		devicedir.o	\
		dir.o		\
		diskcmd.o	\
		diskinit.o	\
		exehdr.o	\
		filedes.o	\
		filename.o	\
		filetype.o	\
		filevars.o	\
		getdevice.o	\
		gotox.o 	\
		gotoxy.o 	\
		gotoy.o 	\
		initcwd.o	\
		loadaddr.o	\
		open.o		\
		oserrlist.o	\
		oserror.o	\
		read.o		\
		rewinddir.o	\
		rwcommon.o	\
		scratch.o	\
		syschdir.o	\
		sysremove.o	\
		sysrename.o	\
		telldir.o	\
		toascii.o	\
		wherex.o	\
		wherey.o	\
		write.o

#--------------------------------------------------------------------------
# Targets

.PHONY:	all clean zap

all:	$(C_OBJS) $(S_OBJS)

clean:
	@$(RM) *~ *.bck $(C_OBJS:.o=.s) $(C_OBJS) $(S_OBJS)

zap:	clean
