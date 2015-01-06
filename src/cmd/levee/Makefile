# makefile for Levee

exedir=/usr/local/bin
mandir=/usr/local/man
CC=cc

CFLAGS=-g
LDFLAGS=-g

OBJS = blockio.o display.o editcor.o exec.o find.o \
    unixcall.o globals.o insert.o main.o misc.o \
    modify.o move.o ucsd.o undo.o wildargs.o \
    version.o

lev: $(OBJS)
	$(CC) $(LDFLAGS) -o lev $(OBJS) -lcurses

version.o: version.c VERSION
	$(CC) $(CFLAGS) -c -DVERSION=\"`cat VERSION`\" version.c

clean:
	rm -f *.o lev

distclean spotless: clean
	rm -f Makefile config.cmd config.sub config.h config.mak config.log config.md

install: install.bin install.man

install.bin: lev
	/home/sergev/Daily/workdir-2015-01-06/levee-3.5a/config.md $(PREFIX)$(exedir)
	/usr/bin/install -s -m 755 lev $(PREFIX)$(exedir)/levee
	/bin/ln -s -f levee $(PREFIX)$(exedir)/lv

install.man:
	/home/sergev/Daily/workdir-2015-01-06/levee-3.5a/config.md $(PREFIX)$(mandir)/man1
	/usr/bin/install -m 444 lv.1 $(PREFIX)$(mandir)/man1/levee.1
	echo ".so man1/levee.1" > $(PREFIX)$(mandir)/man1/lv.1

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
