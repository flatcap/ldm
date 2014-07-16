# Copyright (C) 2001 Richard Russon

KERNEL	= /usr/src/linux
CC	= gcc
LD	= ld
LN	= ln -sf
RM	= rm -f

# General compile flags

CFLAGS += -Wall
CFLAGS += -g
CFLAGS += -O2
CFLAGS += -D_FILE_OFFSET_BITS=64

# LDM compile flags

CFLAGS += -DCONFIG_LDM_PARTITION
CFLAGS += -DCONFIG_LDM_DEBUG
CFLAGS += -DCONFIG_LDM_EXPORT_SYMBOLS

# Kernel compile flags

CFLAGS += -D__KERNEL__
CFLAGS += -Wstrict-prototypes
CFLAGS += -fomit-frame-pointer
CFLAGS += -fno-strict-aliasing
CFLAGS += -pipe
CFLAGS += -mpreferred-stack-boundary=2
CFLAGS += -march=$(shell uname -m)

# ld flags

LFLAGS += -m elf_i386 -r

#-------------------------------------------------------------------------------

export CC CFLAGS KERNEL LD LFLAGS LN RM

LINKS	= docs src
DRIVER	= Makefile.driver
TEST	= test
UTIL	= ldmutil

all:
	make -f $(DRIVER)
	make -C $(TEST)
	make -C $(UTIL)

clean:
	make -f $(DRIVER) clean
	make -C $(TEST)   clean
	make -C $(UTIL)   clean

distclean:
	make -f $(DRIVER) distclean
	make -C $(TEST)   distclean
	make -C $(UTIL)   distclean
	$(RM) $(LINKS)
	find . -name tags -exec $(RM) {} \;
	find . -name \*.data -o -name \*.part -exec $(RM) {} \;

docs:
	$(LN) linux/Documentation docs

src:
	$(LN) linux/fs/partitions src

links:	$(LINKS)

tags:	force
	ctags -R *
	(cd linux/fs/partitions; ctags -R *)
	(cd $(TEST); ctags -R * ../linux/fs/partitions)
	(cd $(UTIL); ctags -R * ../linux/fs/partitions)

force:

