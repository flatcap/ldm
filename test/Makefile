# Copyright (C) 2001 Richard Russon

SRC	= compat.c copy.c dump.c ldminfo.c sparse.c
OBJ	= $(SRC:.c=.o)

LDMDEP	= ../linux/fs/partitions/partitions.o
INFODEP	= $(LDMDEP) compat.o copy.o dump.o ldminfo.o

OUT	= ldminfo sparse

CFLAGS += -include extra.h
CFLAGS += -I$(KERNEL)/include
CFLAGS += -I$(KERNEL)/fs/partitions

all:	$(OUT)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

ldminfo: $(INFODEP)
	$(CC) -o ldminfo $(INFODEP)

sparse:
	$(CC) -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 sparse.c -o $@

clean:
	$(RM) $(OUT) $(OBJ)

distclean: clean
	$(RM) tags

tags:	force
	ctags -R * ../linux/fs/partitions

force:

