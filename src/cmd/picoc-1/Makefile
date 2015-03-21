TOPSRC          = $(shell cd ../../..; pwd)
#include $(TOPSRC)/target.mk
MACHINE		= mips
DESTDIR		?= $(TOPSRC)

# chipKIT PIC32 compiler on Linux
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Download from https://github.com/jasonkajita/chipKIT-cxx/downloads
# and unzip to /usr/local.
# Need to copy pic32-tools/pic32mx/include/stdarg.h
# to pic32-tools/lib/gcc/pic32mx/4.5.1/include.
# MPLABX C32 compiler doesn't support some functionality
# we need, so use chipKIT compiler by default.
ifndef GCCPREFIX
    GCCPREFIX   = /usr/local/pic32-tools/bin/pic32-
    LDFLAGS     = -Wl,--oformat=elf32-tradlittlemips
    INCLUDES    = -I/usr/local/pic32-tools/lib/gcc/pic32mx/4.5.1/include
endif

CC		= $(GCCPREFIX)gcc -mips32r2 -EL -nostdinc -fshort-double -I$(TOPSRC)/include $(INCLUDES)
CXX             = $(GCCPREFIX)g++ -mips32r2 -EL -nostdinc -fshort-double -I$(TOPSRC)/include $(INCLUDES)
LD		= $(GCCPREFIX)ld
AR		= $(GCCPREFIX)ar
RANLIB          = $(GCCPREFIX)ranlib
SIZE            = $(GCCPREFIX)size
OBJDUMP         = $(GCCPREFIX)objdump -mmips:isa32r2
AS		= $(CC) -x assembler-with-cpp -c
YACC            = byacc
LEX             = flex
INSTALL		= install -m 644
INSTALLDIR	= install -m 755 -d
TAGSFILE	= tags
MANROFF		= nroff -man -h
ELF2AOUT	= $(TOPSRC)/tools/elf2aout/elf2aout

CFLAGS		= -O -g 

#LDFLAGS		+= -N -nostartfiles -fno-dwarf2-cfi-asm -T$(TOPSRC)/src/elf32-mips.ld \
#		   $(TOPSRC)/src/crt0.o -L$(TOPSRC)/src

LIBS		= -lc

### END OF TARGET INCLUDE

#CC=gcc
CFLAGS+= -g -DUNIX_HOST -DVER=\"`svnversion -n`\" -DFILENAME_MAX=64 -DL_tmpnam=30 -DCLOCKS_PER_SEC=80000000 -DPATH_MAX=200 -Os
LIBS=-lm -lc

TARGET	= picoc.bin
SRCS	= picoc.c table.c lex.c parse.c expression.c heap.c type.c \
	variable.c clibrary.c platform.c include.c debug.c \
	platform/platform_unix.c platform/library_unix.c \
	cstdlib/stdio.c cstdlib/math.c cstdlib/string.c cstdlib/stdlib.c \
	cstdlib/time.c cstdlib/errno.c cstdlib/ctype.c cstdlib/stdbool.c \
	cstdlib/unistd.c retrobsd.c
OBJS	:= $(SRCS:%.c=%.o)

LDFLAGS		+= -N -nostartfiles -fno-dwarf2-cfi-asm  \
		   -L$(TOPSRC)/src
LDFLAGS         += -T using-bootloader.ld -Wl,-Map=picoc.map

OBJCOPY		= $(GCCPREFIX)objcopy

all: $(TARGET)

$(TARGET): $(OBJS) crt0.o
	${CC} ${LDFLAGS} -o ${TARGET}.elf crt0.o ${OBJS} ${LIBS}
	${OBJDUMP} -S ${TARGET}.elf > ${TARGET}.dis
	${SIZE} ${TARGET}.elf
	$(OBJCOPY) -O binary ${TARGET}.elf ${TARGET}
	$(OBJCOPY) -O ihex --change-addresses=0xa0000000 ${TARGET}.elf ${TARGET}.hex
	#${ELF2AOUT} ${TARGET}.elf $@
	cp $(TARGET) $(TOPSRC)/uflash

install: $(TARGET)
	cp $(TARGET) $(TOPSRC)/uflash

test:	all
	(cd tests; make test)

clean:
	rm -f $(TARGET) $(OBJS) *~ *.elf *.dis

count:
	@echo "Core:"
	@cat picoc.h interpreter.h picoc.c table.c lex.c parse.c expression.c platform.c heap.c type.c variable.c include.c debug.c | grep -v '^[ 	]*/\*' | grep -v '^[ 	]*$$' | wc
	@echo ""
	@echo "Everything:"
	@cat $(SRCS) *.h */*.h | wc

.PHONY: clibrary.c

picoc.o: picoc.c picoc.h
table.o: table.c interpreter.h platform.h
lex.o: lex.c interpreter.h platform.h
parse.o: parse.c picoc.h interpreter.h platform.h
expression.o: expression.c interpreter.h platform.h
heap.o: heap.c interpreter.h platform.h
type.o: type.c interpreter.h platform.h
variable.o: variable.c interpreter.h platform.h
clibrary.o: clibrary.c picoc.h interpreter.h platform.h
platform.o: platform.c picoc.h interpreter.h platform.h
include.o: include.c picoc.h interpreter.h platform.h
debug.o: debug.c interpreter.h platform.h
platform/platform_unix.o: platform/platform_unix.c picoc.h interpreter.h platform.h
platform/library_unix.o: platform/library_unix.c interpreter.h platform.h
cstdlib/stdio.o: cstdlib/stdio.c interpreter.h platform.h
cstdlib/math.o: cstdlib/math.c interpreter.h platform.h
cstdlib/string.o: cstdlib/string.c interpreter.h platform.h
cstdlib/stdlib.o: cstdlib/stdlib.c interpreter.h platform.h
cstdlib/time.o: cstdlib/time.c interpreter.h platform.h
cstdlib/errno.o: cstdlib/errno.c interpreter.h platform.h
cstdlib/ctype.o: cstdlib/ctype.c interpreter.h platform.h
cstdlib/stdbool.o: cstdlib/stdbool.c interpreter.h platform.h
cstdlib/unistd.o: cstdlib/unistd.c interpreter.h platform.h
