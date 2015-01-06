/*
 * LEVEE, or Captain Video;  A vi clone
 *
 * Copyright (c) 1982-2007 David L Parsons
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, without or
 * without modification, are permitted provided that the above
 * copyright notice and this paragraph are duplicated in all such
 * forms and that any documentation, advertising materials, and
 * other materials related to such distribution and use acknowledge
 * that the software was developed by David L Parsons (orc@pell.chi.il.us).
 * My name may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE.
 */
/*
 * Unix interface for levee
 */
#include "levee.h"

#ifdef OS_UNIX

#include "extern.h"
#ifdef HAVE_TERMIOS_H
#   include <termios.h>
#endif
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>

int
min(a,b)
int a, b;
{
    return (a>b) ? b : a;
}

int
max(a,b)
int a, b;
{
    return (a<b) ? b : a;
}

void strput(s)
char *s;
{
    if (s)
	write(1, s, strlen(s));
}

#if !HAVE_BASENAME
char *
basename(s)
char *s;
{
    char *rindex();
    char *p;

    if (p=strrchr(s,'/'))
	return 1+p;
    return s;
}
#endif

static int ioset = 0;

#ifdef HAVE_TERMIOS_H

#if !HAVE_TCGETATTR
#define tcgetattr(fd,t)	ioctl(fd, TCGETS, t)
#define tcsetattr(fd,n,t) ioctl(fd, n, t)
#define TCSANOW	TCSETAF
#endif

static struct termios old;

void
initcon()
{
    struct termios new;

    if (!ioset) {
	tcgetattr(0, &old);	/* get editing keys */

        Erasechar = old.c_cc[VERASE];
        eraseline = old.c_cc[VKILL];

        new = old;

	new.c_iflag &= ~(IXON|IXOFF|IXANY|ICRNL|INLCR);
	new.c_lflag &= ~(ICANON|ISIG|ECHO);
	new.c_oflag = 0;

	tcsetattr(0, TCSANOW, &new);
        ioset=1;
    }
}

void
fixcon()
{
    if (ioset) {
	 tcsetattr(0, TCSANOW, &old);
         ioset = 0;
    }
}

#else /* HAVE_TERMIOS_H */

static struct sgttyb old;
static struct tchars oldtchars;
static struct ltchars oldltchars;

void
initcon()
{
    struct tchars new_tc;
    struct ltchars new_ltc;
    struct sgttyb new;

    if (!ioset) {
        ioctl(0, TIOCGETC, &oldtchars);
        ioctl(0, TIOCGLTC, &oldltchars);

        /* get editing keys */
        Erasechar = 0177;
        eraseline = 'U' & 037;

        new_tc = oldtchars;
        new_tc.t_eofc = -1;         /* end-of-file */
        new_tc.t_quitc = -1;        /* quit */
        new_tc.t_intrc = -1;        /* interrupt */
        new_ltc = oldltchars;
        new_ltc.t_suspc = -1;       /* stop process */
        new_ltc.t_dsuspc = -1;      /* delayed stop process */
        new_ltc.t_rprntc = -1;      /* reprint line */
        new_ltc.t_flushc = -1;      /* flush output */
        new_ltc.t_werasc = -1;      /* word erase */
        new_ltc.t_lnextc = -1;      /* literal next character */
        ioctl(0, TIOCSETC, &new_tc);
        ioctl(0, TIOCSLTC, &new_ltc);

        ioctl(0, TIOCGETP, &old);
        new = old;

        new.sg_flags &= ~(ECHO | CRMOD | XTABS | RAW);
        new.sg_flags |= CBREAK;

        ioctl(0, TIOCSETP, &new);
        ioset = 1;
    }
}

void
fixcon()
{
    if (ioset) {
        ioctl(0, TIOCSETP, &old);
        ioctl(0, TIOCSETC, &oldtchars);
        ioctl(0, TIOCSLTC, &oldltchars);
        ioset = 0;
    }
}

#endif /* HAVE_TERMIOS_H */

int
getKey()
{
    unsigned char c[1];
    fd_set input;

    fflush(stdout);
    /* we're using Unix select() to wait for input, so lets hope that
     * all the Unices out there support select().  If your Unix doesn't,
     * you can make this work by replacing this select loop with:
     *
     *       while (read(0,c,1) != 1)
     *           ;
     *       return c[1];
     *
     * ... and watch your load-average peg.
     */
    while (1) {
	FD_ZERO(&input);
	FD_SET(0, &input);

	select(1, &input, 0, 0, 0);
	if( read(0,c,1) == 1)
	    return c[0];
    };
}
#endif
