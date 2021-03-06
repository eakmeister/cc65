#
# makefile for CC65 runtime library
#

.SUFFIXES: .o .s .c

#--------------------------------------------------------------------------
# Programs and flags

SYS	= apple2enh

AS	= ../../src/ca65/ca65
CC	= ../../src/cc65/cc65
CO	= ../../src/co65/co65
LD	= ../../src/ld65/ld65

AFLAGS	= -t $(SYS) --forget-inc-paths -I../apple2 -I../../asminc
CFLAGS	= -Osir -g -T -t $(SYS) --forget-inc-paths -I ../apple2 -I ../../include

#--------------------------------------------------------------------------
# Rules

%.o:	../apple2/%.c
	@$(CC) -o $(notdir $(*).s) $(CFLAGS) $<
	@$(AS) -o $@ $(AFLAGS) $(*).s

%.o:	../apple2/%.s
	@$(AS) -g -o $@ $(AFLAGS) $<

extra/%.o: ../apple2/extra/%.s
	@$(AS) -g -o $@ $(AFLAGS) $<

%-emd.o: %.emd
	@$(CO) -o $(*)-emd.s --code-label _$(subst -,_,$(*)) $<
	@$(AS) -o $@ $(AFLAGS) $(*)-emd.s

%-joy.o: %.joy
	@$(CO) -o $(*)-joy.s --code-label _$(subst -,_,$(*)) $<
	@$(AS) -o $@ $(AFLAGS) $(*)-joy.s

%-mou.o: %.mou
	@$(CO) -o $(*)-mou.s --code-label _$(subst -,_,$(*)) $<
	@$(AS) -o $@ $(AFLAGS) $(*)-mou.s

%-ser.o: %.ser
	@$(CO) -o $(*)-ser.s --code-label _$(subst -,_,$(*)) $<
	@$(AS) -o $@ $(AFLAGS) $(*)-ser.s

%-tgi.o: %.tgi
	@$(CO) -o $(*)-tgi.s --code-label _$(subst -,_,$(*)) $<
	@$(AS) -o $@ $(AFLAGS) $(*)-tgi.s

%.emd: 	%.o ../runtime/zeropage.o
	@$(LD) -o $@ -t module $^

%.joy:	%.o ../runtime/zeropage.o
	@$(LD) -o $@ -t module $^

%.mou:	%.o ../runtime/zeropage.o
	@$(LD) -o $@ -t module $^

%.ser:	%.o ../runtime/zeropage.o
	@$(LD) -o $@ -t module $^

%.tgi:	%.o ../runtime/zeropage.o
	@$(LD) -o $@ -t module $^

#--------------------------------------------------------------------------
# Object files

C_OBJS=	closedir.o		\
	opendir.o		\
	readdir.o		\
	rewinddir.o

S_OBJS=	_scrsize.o		\
	break.o			\
	cclear.o		\
	cgetc.o			\
	chline.o		\
	close.o			\
	clrscr.o		\
	color.o			\
	cout.o			\
	cputc.o			\
	crt0.o			\
	ctype.o			\
	curdevice.o		\
	cvline.o		\
	devicedir.o		\
	dioclose.o		\
	diocommon.o		\
	dioopen.o		\
	dioread.o		\
	diosectcount.o		\
	diosectsize.o		\
	diowrite.o		\
	dosdetect.o		\
	exec.o			\
	exehdr.o		\
	filedes.o		\
	filename.o		\
	get_ostype.o		\
	getdevice.o		\
	gotoxy.o		\
	gotoy.o			\
	home.o			\
	initcwd.o		\
	iobuf.o			\
	irq.o			\
	isdevice.o		\
	joy_stat_stddrv.o	\
	joy_stddrv.o		\
	kbhit.o			\
	mainargs.o		\
	mcbdefault.o		\
	mli.o			\
	mouse_stat_stddrv.o	\
	mouse_stddrv.o		\
	open.o			\
	oserrlist.o		\
	oserror.o		\
	randomize.o		\
	rdkey.o			\
	read.o			\
	reboot.o		\
	revers.o		\
	rwcommon.o		\
	syschdir.o		\
	sysmkdir.o		\
	sysremove.o		\
	sysrename.o		\
	sysrmdir.o		\
	systime.o		\
	sysuname.o		\
	textframe.o		\
	tgi_colors.o		\
	tgi_stat_stddrv.o	\
	tgi_stddrv.o		\
	toascii.o		\
	videomode.o		\
	vtabz.o			\
	wherex.o		\
	wherey.o		\
	write.o			\
	$(EMDS:.emd=-emd.o)	\
	$(JOYS:.joy=-joy.o)	\
	$(MOUS:.mou=-mou.o)	\
	$(SERS:.ser=-ser.o)	\
	$(TGIS:.tgi=-tgi.o)

S_EXTRA_OBJS=			\
	extra/iobuf-0800.o

#--------------------------------------------------------------------------
# Drivers

EMDS = apple2-auxmem.emd

JOYS = apple2-stdjoy.joy

MOUS = apple2-stdmou.mou

SERS = apple2-ssc.ser

TGIS = apple2-280-192-8.tgi apple2-40-48-16.tgi

#--------------------------------------------------------------------------
# Targets

.PHONY:	all clean zap

all:	$(C_OBJS) $(S_OBJS) $(S_EXTRA_OBJS) $(EMDS) $(JOYS) $(MOUS) $(SERS) $(TGIS)

../runtime/zeropage.o:
	$(MAKE) -C $(dir $@) $(notdir $@)

clean:
	@$(RM) $(C_OBJS:.o=.s) $(C_OBJS) $(S_OBJS) $(S_EXTRA_OBJS) \
	       $(EMDS:.emd=.o) $(EMDS:.emd=-emd.s) \
	       $(JOYS:.joy=.o) $(JOYS:.joy=-joy.s) \
	       $(MOUS:.mou=.o) $(MOUS:.mou=-mou.s) \
	       $(SERS:.ser=.o) $(SERS:.ser=-ser.s) \
	       $(TGIS:.tgi=.o) $(TGIS:.tgi=-tgi.s)

zap:	clean
	@$(RM) $(EMDS) $(JOYS) $(MOUS) $(SERS) $(TGIS)
