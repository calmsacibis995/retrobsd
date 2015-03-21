
.SUFFIXES: .c .s .S .o

.c.o:
	lccom -target=mips-el $< > $*.s
	as -o $@ $*.s

.s.o:
	as -o $@ $<

.S.o:
	as -o $@ $<

all:	hello 

hello:	hello.o sys.o
	ld -o hello -e main hello.o sys.o

clean:
	rm -f *.o *.s hello
	
