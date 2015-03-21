/*
 * uflash driver for PIC32.
 *
 * Based on GPIO driver:
 * Copyright (C) 2012 Serge Vakulenko, <serge@vak.ru>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#include "param.h"
#include "conf.h"
#include "user.h"
#include "ioctl.h"
#include "uflash.h"
#include "systm.h"
#include "uio.h"
#include "stat.h"

#include "map.h"
#include "proc.h"
#include "buf.h"
#include "inode.h"
#include "namei.h"
#include "fs.h"
#include "mount.h"
#include "file.h"
#include "signalvar.h"

/* TODO: some sort of interlock is needed so that the contents
 * of flash are not replaced while a processing that depends 
 * on the contents is executing.
 */

#define MINOR_UNIT	  0x07			/* Minor mask: unit number */

#define FLASH_PAGE_SIZE 4096
#define FLASH_BUFFER_INT_COUNT 4

static void flashErasePage(unsigned addrPage);
static void flashWriteUint32(unsigned addrUint32, unsigned *rgu32Data, unsigned cu32Data);

// Flash operations
#define NVMOP_WORD_PGM		0x4001	  // Word program operation
#define NVMOP_ROW_PGM		0x4003	  // Row write
#define NVMOP_PAGE_ERASE	0x4004	  // Page erase operation

#define USE_USER_FLASH_START (USER_FLASH_START-0x20000000UL)

/*
 * To enable debug output, comment out the first line,
 * and uncomment the second line.
 */
#define PRINTDBG(...) /*empty*/
//#define PRINTDBG printf

char targetToFlash[31] = "";

int
uflash_open (dev, flag, mode)
	dev_t dev;
{
	if (u.u_uid != 0)
		return EPERM;
	return 0;
}

int
uflash_close (dev, flag, mode)
	dev_t dev;
{
	return 0;
}

int
uflash_read (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct iovec *iov;
	register u_int cnt;
	char *buf = targetToFlash;

	cnt = strlen(targetToFlash);

	/* I/o size should be large enough. */
	iov = uio->uio_iov;
	if (iov->iov_len < cnt)
		return EIO;

	/* Read only cnt bytes. */
	if (uio->uio_offset >= cnt)
		return 0;

	cnt -= uio->uio_offset;


	/* Print port status to buffer. */
	//PRINTDBG ("uflash_read -> %s", buf);

	bcopy (buf + uio->uio_offset, iov->iov_base, cnt);
	iov->iov_base += cnt;
	iov->iov_len -= cnt;
	uio->uio_resid -= cnt;
	uio->uio_offset += cnt;
	return 0;
}

int
uflash_write (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct iovec *iov = uio->uio_iov;
	register u_int cnt = 30;
	char * buf = targetToFlash;

	/* I/o size should be large enough. */
	if (iov->iov_len > cnt)
		return EIO;

	if( iov->iov_len < cnt ) cnt = iov->iov_len;

	cnt -= uio->uio_offset;
	bcopy (iov->iov_base, buf+uio->uio_offset, cnt);
	buf[uio->uio_offset+cnt]=0;

	iov->iov_base += cnt;
	iov->iov_len -= cnt;
	uio->uio_resid -= cnt;
	uio->uio_offset += cnt;

	//PRINTDBG ("uflash_read ('%s')\n", buf);
	return 0;
}

/*
 * Commands:
 * UFLASH_LOAD  - load the program specified into user flash
 */

int
uflash_ioctl (dev, cmd, addr, flag)
	dev_t dev;
	register u_int cmd;
	caddr_t addr;
{
	int result = 0;
	struct inode *ip;

	//unsigned unit = cmd & 0xff;
	cmd &= ~0xff;

	if( cmd != UFLASH_LOAD )
		return EINVAL;
	
	struct nameidata nd;
	register struct	nameidata *ndp = &nd;

	//printf ("uflash_ioctl ('%s', ['%s', '%s', ...])\n", uap->fname, uap->argp[0], uap->argp[1]);

	NDINIT (ndp, LOOKUP, FOLLOW, targetToFlash);
	ip = namei (ndp);
	if (ip == NULL) {
		result = EACCES;
		goto done;
	}
	
	struct stat sb;
	int err = ino_stat (ip, &sb);
	if( err ) {
		result = err;
		goto done;
	}
	
	/* Erase Pages */
	
	unsigned address = USE_USER_FLASH_START;
	int remaining = (int)sb.st_size;
	
	while( remaining > 0 ) {
		flashErasePage( address );
		address += 4096;
		remaining -= 4096;
	}
	
	/* TODO: reorgainize writing so that everything except the first
	 * couple of bytes is written to flash and verified before
	 * writing and verifying the first couple of bytes. The purpose
	 * would be to leave the uflash area in an invalid state so that
	 * the stub code would not attempt to execute it.
	 */
	
	/* Write bin */
	
	address = USE_USER_FLASH_START;
	remaining = (int)sb.st_size;
	
	unsigned buffer[FLASH_BUFFER_INT_COUNT];
	
	while( remaining ) {
		unsigned readsize = FLASH_BUFFER_INT_COUNT * sizeof(unsigned);
		if( readsize > remaining ) readsize = remaining;
		
		int resid;
		err = rdwri (UIO_READ, ip, (caddr_t)buffer, readsize, (off_t) address - USE_USER_FLASH_START, IO_UNIT, &resid);
	
		if( err ) {
			result = err;
			goto done;
		}
		
		if( resid ) {
			result = EIO;
			goto done;
		}
		
		flashWriteUint32(address, buffer, FLASH_BUFFER_INT_COUNT);
	
		address += readsize;
		remaining -= readsize;
	}

	/* Verify bin */

	address = USE_USER_FLASH_START;
	remaining = (int)sb.st_size;

	while( remaining ) {
		unsigned readsize = FLASH_BUFFER_INT_COUNT * sizeof(unsigned);
		if( readsize > remaining ) readsize = remaining;
		
		int resid;
		err = rdwri (UIO_READ, ip, (caddr_t)buffer, readsize, (off_t) address - (unsigned)USE_USER_FLASH_START, IO_UNIT, &resid);
	
		if( err ) {
			result = err;
			goto done;
		}
		
		if( resid ) {
			result = EIO;
			goto done;
		}
		
		int i;
		for( i = 0; i < readsize; ++i ) {
			if( ((char*)address)[i] != ((char*)buffer)[i] ) {
				result = EIO;
				goto done;
			}
		}
	
		address += readsize;
		remaining -= readsize;
	}
	
done:
	if (ip)
		iput(ip);
		
	return result;
}



/***	void flashOperation(unsigned nvmop, unsigned addr, unsigned data)
**
**	Synopsis:   
**	  Performs either a page erase, word write, or row write
**
**	Parameters:
**	  nvmop	either NVMOP_PAGE_ERASE, NVMOP_WORD_PGM, or NVMOP_ROW_PGM
**		addr	the unsigned_t flash address of: the page to erase, word location to write, or row location to write
**		data	a unsigned_t of data to write, or the unsigned_t of the SRAM address containing the array of data to write to the row
**
**	Return Values:
**	  True if successful, false if failed
**
**	Errors:
**	  None
**
**  Notes:
**	  data has no meaning when page erase is specified and should be set to 0ul
**
*/

/*
 * nvm_operation is borrowed from usb_boot.c and is controlled
 * by the following copyright:
 * 
 * USB HID bootloader for PIC32 microcontroller.
 *
 * Based on Microchip sources.
 * Heavily rewritten by Serge Vakulenko, <serge@vak.ru>.
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the 'Company') for its PIC(R) Microcontroller is intended and
 * supplied to you, the Company's customer, for use solely and
 * exclusively on Microchip PIC Microcontroller products. The
 * software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN 'AS IS' CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 */
static void nvm_operation (unsigned op, unsigned address, unsigned data)
{
	int x;

	// Convert virtual address to physical
	NVMADDR = address & 0x1fffffff;
	NVMDATA = data;

	// Disable interrupts
	x = mips_intr_disable();

	// Enable Flash Write/Erase Operations
	NVMCON = PIC32_NVMCON_WREN | op;

	// Data sheet prescribes 6us delay for LVD to become stable.
	// To be on the safer side, we shall set 7us delay.
	udelay (7);

	NVMKEY = 0xAA996655;
	NVMKEY = 0x556699AA;
	NVMCONSET = PIC32_NVMCON_WR;

	// Wait for WR bit to clear
	while (NVMCON & PIC32_NVMCON_WR)
		continue;

	// Disable Flash Write/Erase operations
	NVMCONCLR = PIC32_NVMCON_WREN;

	// Enable interrupts
	mips_intr_restore (x);
#if 0
	// Check error status
	if (NVMCON & (PIC32_NVMCON_WRERR | PIC32_NVMCON_LVDERR)) {
		TODO;
	}
#endif
}

// flashErasePage and flashWriteUint32 where originally from the file flash.c
// implements the low level flash control and access.
//
// flash.c originated from the cpustick.com skeleton project from
// http://www.cpustick.com/downloads.htm and was originally written
// by Rich Testardi; please preserve this reference and share bug
// fixes with rich@testardi.com.
/***	void flashErasePage(unsigned addrPage)
**
**	Synopsis:   
**	  Erases the page starting at the page address.
*
**	Parameters:
**	  addrPage	The virtual address of the page to erase
**
**	Return Values:
**	  None
**
**	Errors:
**	  None
**
**  Notes:
**
**	  addrPage must be page aligned.
**
*/
static void flashErasePage(unsigned addrPage)
{
	int i;
	int j;
	unsigned x = ~0;
	unsigned *rgUint32 = (unsigned *) addrPage;

	// we learned that just because the flash does not successfully
	// erase on the first attempt, it might on another. We found that
	// we can double the life of flash by attempting to
	// erase the flash up to 5 times before quitting.
	for(j=0; j<5; j++) {
		// first check to see if the page needs to be erased
	   	for (i = 0; i < FLASH_PAGE_SIZE/sizeof(unsigned); i++) {
			x &= rgUint32[i];
		}
		
		// flash erased, we are done.
		if(x == ~0) {
			break;
		} 

		// Unlock and Erase Page
		nvm_operation(NVMOP_PAGE_ERASE, addrPage, 0);
	}

	// at this point, we don't care if the flash ever actually erased as
	// as we will catch the failure when we verify the programming.
 }


/***	void flashWriteUint32(unsigned addrUint32, unsigned *rgu32Data, unsigned cu32Data)
**
**	Synopsis:   
**	  Writes an array to unsigned to flash
*
**	Parameters:
**	  rgu32Data	Pointer to an array of unsigned
**	  cu32Data	The number of unsigned to write
**
**	Return Values:
**	  None
**
**	Errors:
**	  None
**
**  Notes:
**
**	  Assumes the pages have already been erased.
**
*/ 
static void flashWriteUint32(unsigned addrUint32, unsigned *rgu32Data, unsigned cu32Data)
{
	int  i = 0;

	for(i=0; i < cu32Data; i++) {
		// only do this if the data is not what is already in flash
		if(rgu32Data[i] != ~0) {	
			// Write the data
			nvm_operation(NVMOP_WORD_PGM, addrUint32, rgu32Data[i]);
		}
		addrUint32 += sizeof(unsigned);
	}
}
