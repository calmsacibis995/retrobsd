#
# Public Domain.  1995/03/13 - Steven Schultz
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

#CFLAGS		+= -Werror
LIBS            += -ltermcap

OBJS            = blockio.o display.o editcor.o exec.o find.o \
                  unixcall.o globals.o insert.o main.o misc.o \
                  modify.o move.o ucsd.o undo.o wildargs.o \
                  version.o
MAN             = lv.0

all:            lv ${MAN}

lv:             ${OBJS}
		${CC} ${LDFLAGS} -o lv.elf ${OBJS} ${LIBS}
		${OBJDUMP} -S lv.elf > lv.dis
		${SIZE} lv.elf
		${ELF2AOUT} lv.elf $@ && rm lv.elf

version.o: version.c VERSION
		$(CC) $(CFLAGS) -c -DVERSION=\"`cat VERSION`\" version.c

.SUFFIXES:      .0 .1

.1.0:
		${MANROFF} $*.1 > $@

clean:
		rm -f *.o *.elf ${MAN} lv *.elf *.dis tags *~

install:        all
		install lv ${DESTDIR}/bin/
		cp lv.0 ${DESTDIR}/share/man/cat1/

# Dependencies

blockio.o    : levee.h extern.h blockio.c config.h
display.o    : levee.h extern.h termcap.i display.c config.h
editcor.o    : levee.h extern.h editcor.c config.h
exec.o       : levee.h extern.h exec.c config.h
find.o       : levee.h extern.h grep.h find.c config.h
globals.o    : levee.h globals.c config.h
insert.o     : levee.h extern.h insert.c config.h
main.o       : levee.h extern.h main.c config.h
misc.o       : levee.h extern.h misc.c config.h
modify.o     : levee.h extern.h grep.h modify.c config.h
move.o       : levee.h extern.h move.c config.h
rmxcall.o    : levee.h rmxcall.c config.h
ucsd.o       : levee.h extern.h ucsd.c config.h
undo.o       : levee.h extern.h undo.c config.h
unixcall.o   : levee.h extern.h unixcall.c config.h
wildargs.o   : levee.h extern.h wildargs.c config.h
