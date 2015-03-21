/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)gpio.h	1.4 (2.11BSD GTE) 1997/3/28
 */

/*
 * Ioctl definitions
 */
#ifndef	_UFLASH_H
#define _UFLASH_H

#include <sys/ioctl.h>
#include <sys/uio.h>

#define UFLASH_LOAD	(IOC_VOID | 1 << 16 | 'u'<<8)   /* configure as input */

#define UFLASH_MAGIC 0x24

struct uflash_head
{
	unsigned magic;
	char name[31];
};

#define UFLASH_HEADPTR ((struct uflash_head*)0x7d030000)

typedef void (*_startp)(int, char **, char **);
#define UFLASH_STARTPTR ((_startp)0x7d030030)

#ifdef KERNEL
int uflash_open (dev_t dev, int flag, int mode);
int uflash_close (dev_t dev, int flag, int mode);
int uflash_read (dev_t dev, struct uio *uio, int flag);
int uflash_write (dev_t dev, struct uio *uio, int flag);
int uflash_ioctl (dev_t dev, u_int cmd, caddr_t addr, int flag);
#endif

#endif
