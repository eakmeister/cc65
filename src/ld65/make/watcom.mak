#
# ld65 Makefile for the Watcom compiler (using GNU make)
#

# ------------------------------------------------------------------------------
# Generic stuff

# Environment variables for the watcom compiler
export WATCOM  = c:\\watcom
export INCLUDE = $(WATCOM)\\h

# We will use the windows compiler under linux (define as empty for windows)
export WINEDEBUG=fixme-all
WINE = wine

# Programs
AR     	= $(WINE) wlib
CC      = $(WINE) wcc386
LD     	= $(WINE) wlink
WSTRIP	= $(WINE) wstrip -q

LNKCFG  = ld.tmp

# Program arguments
CFLAGS  = -d1 -obeilr -zp4 -5 -zq -w2 -i=..\\common

# Target files
EXE	= ld65.exe

# Create NT programs by default
ifndef TARGET
TARGET = NT
endif

# --------------------- OS2 ---------------------
ifeq ($(TARGET),OS2)
SYSTEM  = os2v2
CFLAGS  += -bt=$(TARGET)
endif

# -------------------- DOS4G --------------------
ifeq ($(TARGET),DOS32)
SYSTEM  = dos4g
CFLAGS  += -bt=$(TARGET)
endif

# --------------------- NT ----------------------
ifeq ($(TARGET),NT)
SYSTEM  = nt
CFLAGS  += -bt=$(TARGET)
endif

# ------------------------------------------------------------------------------
# Implicit rules

%.obj:  %.c
	$(CC) $(CFLAGS) -fo=$@ $^


# ------------------------------------------------------------------------------
# All OBJ files

OBJS =	asserts.obj     \
        bin.obj		\
	binfmt.obj	\
        cfgexpr.obj     \
	condes.obj	\
	config.obj	\
        dbgfile.obj     \
	dbgsyms.obj	\
	error.obj	\
	exports.obj	\
	expr.obj	\
	extsyms.obj   	\
	fileinfo.obj	\
	fileio.obj	\
        filepath.obj    \
	fragment.obj	\
	global.obj	\
	library.obj	\
	lineinfo.obj	\
	main.obj	\
	mapfile.obj	\
        memarea.obj     \
	o65.obj		\
	objdata.obj	\
	objfile.obj	\
	scanner.obj	\
        scopes.obj      \
	segments.obj	\
        span.obj        \
        spool.obj       \
	tgtcfg.obj      \
        tpool.obj

LIBS = ../common/common.lib


# ------------------------------------------------------------------------------
# Main targets

all:	  	$(EXE)


# ------------------------------------------------------------------------------
# Other targets


$(EXE): 	$(OBJS) $(LIBS)
	@echo "DEBUG ALL" > $(LNKCFG)
	@echo "OPTION QUIET" >> $(LNKCFG)
	@echo "OPTION MAP" >> $(LNKCFG)
	@echo "OPTION STACK=65536" >> $(LNKCFG)
	@echo "NAME $@" >> $(LNKCFG)
	@for i in $(OBJS); do echo "FILE $${i}"; done >> $(LNKCFG)
	@for i in $(LIBS); do echo "LIBRARY $${i}"; done >> $(LNKCFG)
	@$(LD) system $(SYSTEM) @$(LNKCFG)
	@rm $(LNKCFG)

clean:
	@rm -f *~ core

zap:	clean
	@rm -f $(OBJS) $(EXE) $(EXE:.exe=.map)

strip:
	@-$(WSTRIP) $(EXE)

