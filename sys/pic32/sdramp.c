/*
 * Driver for external SDRAM-based swap device.
 *
 * See sdram.S for information on interface to sdram
 *
 * This code could use a bit of optimization.
 */

#include "param.h"
#include "systm.h"
#include "buf.h"
#include "errno.h"
#include "dk.h"
#include "sdram.h"

int sw_dkn = -1;                /* Statistics slot number */

/*
 * physical specs of SDRAM chip
 */
#define RAM_COLS 512
#define RAM_ROWS 4096
#define RAM_BANKS 4


/*
 * RAM_BURST_COUNT MUST be match the number of bytes
 * read/written by each call to sdram_read/sdram_write
 */ 
#define RAM_BURST_COUNT 8

/*
 * CHUNK_SIZE number of bytes in each "chunk"
 */
#define CHUNK_SIZE 128

#define RAM_BURST_GROUP_COUNT 8  

#define BLOCKS_PER_ROW ( RAM_COLS / CHUNK_SIZE )
  
static char swaptemp[CHUNK_SIZE];

static void 
read_chunk_from_sdram( uint64_t* dest, unsigned int blockNumber )
{
	int startColumn = ( ( blockNumber & ( BLOCKS_PER_ROW - 1 ) ) * CHUNK_SIZE ) / RAM_BURST_COUNT;
	int rowAndBank = blockNumber / BLOCKS_PER_ROW;
	int row = rowAndBank & ( RAM_ROWS - 1 );
	int bank = rowAndBank / RAM_ROWS;
	int col = startColumn;

	while( col < startColumn + CHUNK_SIZE/RAM_BURST_COUNT ) {
		int x = mips_intr_disable();
		sdram_wake();
		sdram_bank(bank);
		sdram_active(row);
		int i;
		for( i = 0; i < RAM_BURST_GROUP_COUNT; i++ ) {
			*dest++ = sdram_read( col );
			col += 1; //RAM_BURST_COUNT;
		}
		sdram_precharge();
		sdram_precharge_all();
		sdram_sleep();

		mips_intr_restore (x);

		asm volatile ("nop");
		asm volatile ("nop");
		asm volatile ("nop");
		asm volatile ("nop");
		asm volatile ("nop");
		asm volatile ("nop");
	} 
}

static void 
write_chunk_to_sdram( uint64_t* src, unsigned int blockNumber )
{
	int startColumn = ( ( blockNumber & ( BLOCKS_PER_ROW - 1 ) ) * CHUNK_SIZE ) / RAM_BURST_COUNT;
	int rowAndBank = blockNumber / BLOCKS_PER_ROW;
	int row = rowAndBank & ( RAM_ROWS - 1 );
	int bank = rowAndBank / RAM_ROWS;
  
	int col = startColumn;
	while( col < startColumn + CHUNK_SIZE/RAM_BURST_COUNT ) {
		int x = mips_intr_disable();

		sdram_wake();
		sdram_bank(bank);
		sdram_active(row);
		int i;
		for( i = 0; i < RAM_BURST_GROUP_COUNT; i++ ) {
			sdram_write( col, *src++ );
			col += 1; //RAM_BURST_COUNT;
		}
		sdram_precharge();
		sdram_precharge_all();
		sdram_sleep();

		mips_intr_restore (x);
      
		asm volatile ("nop");
		asm volatile ("nop");
		asm volatile ("nop");
		asm volatile ("nop");
		asm volatile ("nop");
		asm volatile ("nop");
	} 
}


/*
 * Read a block of data.
 */
static void
dev_read(unsigned blockno, char* data, unsigned nbytes)
{
	blockno = blockno * (DEV_BSIZE/CHUNK_SIZE);

	while( nbytes >= CHUNK_SIZE ) {
		read_chunk_from_sdram( (uint64_t*) swaptemp, blockno );
		bcopy( swaptemp, data, CHUNK_SIZE );
		data += CHUNK_SIZE;
		blockno += 1;
		nbytes -= CHUNK_SIZE;
	}

	if( nbytes ) {
		read_chunk_from_sdram( (uint64_t*) swaptemp, blockno );
		bcopy( swaptemp, data, nbytes );
	}

	return;
}

/*
 * Write a block of data.
 */
static void
dev_write (unsigned blockno, char *data, unsigned nbytes)
{
	blockno = blockno * (DEV_BSIZE/CHUNK_SIZE);

	while( nbytes >= CHUNK_SIZE ) {
		bcopy( data, swaptemp, CHUNK_SIZE );
		write_chunk_to_sdram( (uint64_t*) swaptemp, blockno );
		data += CHUNK_SIZE;
		blockno += 1;
		nbytes -= CHUNK_SIZE;
	}
	if( nbytes ) {
		read_chunk_from_sdram( (uint64_t*) swaptemp, blockno );
		bcopy( data, swaptemp, nbytes );
		write_chunk_to_sdram( (uint64_t*) swaptemp, blockno );
	}
	return;
}

int 
swopen(dev_t dev, int flag, int mode)
{
	static int has_been_opened = 0;

	if (!has_been_opened) {
		int x = mips_intr_disable();
		sdram_init();
		mips_intr_restore(x);
		has_been_opened = 1;
	
#ifdef UCB_METER
		/* Allocate statistics slot */
		dk_alloc (&sw_dkn, 1, "sw");
#endif
	}
	return 0;
}

void
swstrategy(register struct buf *bp)
{
	int s;

#if 0
	printf ("sw0: %s block %u, length %u bytes, addr %p\n",
		(bp->b_flags & B_READ) ? "read" : "write",
		bp->b_blkno, bp->b_bcount, bp->b_addr);
#endif
        led_control (LED_AUX, 1);
        s = splbio();
#ifdef UCB_METER
	if (sw_dkn >= 0) {
                dk_busy |= 1 << sw_dkn;
		dk_xfer[sw_dkn]++;
		dk_bytes[sw_dkn] += bp->b_bcount;
	}
#endif
	unsigned adj = 0;
	if (minor(bp->b_dev)==1) {
		adj = SWAPSZ; 
	}
	if (bp->b_flags & B_READ) {
		dev_read (bp->b_blkno+adj, bp->b_addr, bp->b_bcount);
	} else {
		dev_write (bp->b_blkno+adj, bp->b_addr, bp->b_bcount);
	}        
#if 0
	printf ("    %02x", (unsigned char) bp->b_addr[0]);
        int i;
	for (i=1; i<bp->b_bcount; i++)
                printf ("-%02x", (unsigned char) bp->b_addr[i]);
	printf ("\n");
#endif
	biodone (bp);
        led_control (LED_AUX, 0);
        splx (s);
#ifdef UCB_METER
	if (sw_dkn >= 0)
                dk_busy &= ~(1 << sw_dkn);
#endif
}
