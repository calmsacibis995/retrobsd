/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

/*
 * C runtime startoff.  When an a.out is loaded by the kernel, the kernel
 * sets up the stack as follows:
 *
 *	_________________________________
 *	| (NULL)			| top of memory
 *	|-------------------------------|
 *	| 				|
 *	| environment strings		|
 *	| 				|
 *	|-------------------------------|
 *	| 				|
 *	| argument strings		|
 *	| 				|
 *	|-------------------------------|
 *	| envv[envc] (NULL)		| end of environment vector tag, a 0
 *	|-------------------------------|
 *	| envv[envc-1]			| pointer to last environment string
 *	|-------------------------------|
 *	| ...				|
 *	|-------------------------------|
 *	| envv[0]			| pointer to first environment string
 *	|-------------------------------|
 *	| argv[argc] (NULL)		| end of argument vector tag, a 0
 *	|-------------------------------|
 *	| argv[argc-1]			| pointer to last argument string
 *	|-------------------------------|
 *	| ...				|
 *	|-------------------------------|
 *	| argv[0]			| pointer to first argument string
 *	|-------------------------------|
 *	|				| space for fourth argument
 *	|-------------------------------|
 *	|				| space for third argument
 *	|-------------------------------|
 *	|				| space for second argument
 *	|-------------------------------|
 * sp->	| 				| space for first
 *	---------------------------------
 *
 * Arguments are passed in registers $a0, $a1 and $a2.
 * Register $gp is set to the start of data section.
 *
 * Crt0 simply moves the env to environ variable, calculates
 * the __progname and then calls main.
 */
extern int main (int, char ** /*, char ** */ );

char **environ;
const char *__progname = "";

void _start (int, char **, char **);

extern void __data_start();
extern void _etext();


/* The entry function. */
void
_start (argc, argv, env)
	int argc;
	char **argv;
	char **env;
{
        asm volatile ("la $gp, _gp");

	/* Copy the .data image from flash to ram.
	 * Linker places it at the end of .text segment. */
	extern unsigned _edata;
	unsigned *src = (unsigned*) &_etext;
	unsigned *dest = (unsigned*)&__data_start;
	unsigned *limit = (unsigned*)&_edata; //0x7d03b554 0x7f00b550; 



	extern char _end[];
	//extern const char *_curbrk;
	//_curbrk = _end;

	_brk (_end);

	//sbrk((char*) - (char*)dest);
	//brk((char*)0x7f00f000); //0x7f00c000

	while (dest < limit) {
		/*printf ("copy %08x from (%08x) to (%08x)\n", *src, src, dest);*/
		*dest++ = *src++;
	}
	unsigned *bssend = (unsigned*)_end;
	while( dest < _end )
		*dest++ = 0;

	//printf("%p %p %p\n",(char*)&_edata,(char*)&__data_start,(char*)&_edata-(char*)&__data_start);

	environ = env;
	if (argc > 0 && argv[0] != 0) {
		const char *s;

		__progname = argv[0];
		for (s = __progname; *s != '\0'; s++)
			if (*s == '/')
				__progname = s + 1;
	}

	//printf("%p %p %p %p\n",(char*)&_edata,(char*)&__data_start,(char*)&_edata-(char*)&__data_start, _end);

	exit (main (argc, argv));
}
