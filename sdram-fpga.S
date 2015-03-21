/*
 * SDRAM Access Routines for PIC32.
 * 
 * Retromaster - 10.05.2010
 * 
 * This file is in the public domain. You can use, modify, and distribute the source code
 * and executable programs based on the source code. This file is provided "as is" and 
 * without any express or implied warranties whatsoever. Use at your own risk!
 *
 * Changes by jmcgee for inclusion in the retrobsd project. 
 */

/* SDRAM Used: HY57V281620
 * SDRAM Pin to PIC32 Pin Mapping:
 * SDRAM	PIC32
 * ------	------
 * note: the order of An to RBn DOES matter
 * At the moment it is a bit of a mess and 
 * needs to be cleaned up.
 *     A0 23	  RB11
 *     A1 24	  RB12
 *     A2 25	  RB13
 *     A3 26	  RB14
 *     A4 29	   RB5
 *     A5 30	   RB4
 *     A6 31	   RB3
 *     A7 32	   RB2
 *     A8 33	   RB6
 *     A9 34	   RB7
 *    A10 22	  RB15
 *    A11 35	   RB9
 *
 *    BA0 20	   RD4
 *    BA1 21	   RD5
 *
 * note: the order of DQn to RAn does not matter
 *    DQ0 2	   RA6
 *    DQ1 4	   RA1
 *    DQ2 5	   RA7
 *    DQ3 7	   RA2
 *    DQ4 8	   RA3
 *    DQ5 10	   RA4
 *    DQ6 11	   RA5
 *    DQ7 13	   RA0
 
 *    CLK 38	   OC1-RD0
 *    CKE 37	   A10
 *     CS 19	   RF12
 *     WE 16	   RF0
 *    CAS 17	   RF1
 *    RAS 18	   RF13
 *
 *  SDRam 42, 44, 45, 47, 48, 50, 51, 53 - (d8-d15) should be pulled up, but
 *  on the prototype board these were simply left floating  
 *  
 *  Power connections not listed.
 */


/* 
 * Retrobsd does not currently contain a good header
 * for pulling the pic32 port addresses into an assembly
 * file, so define some needed registers here for the moment.
 */

#define TRISA 0xBF886000
#define T2CON 0xBF800800
#define TMR2  0xBF800810
#define PR2   0xBF800820
#define T2CONSET 0xBF800808

#define OC1CON   0xBF803000
#define OC4CON   0xBF803600
#define OCXR_OFFSET 0x10
#define OCXRS_OFFSET 0x20

#define AD1PCFGSET 0xBF809068

/* Offsets (from TRISA) for the io ports */
#define SDR_OFFSET_A 0
#define SDR_OFFSET_B 0x40
#define SDR_OFFSET_C 0x80
#define SDR_OFFSET_D 0xc0
#define SDR_OFFSET_E 0x100
#define SDR_OFFSET_F 0x140
#define SDR_OFFSET_G 0x180

/* Offsets (from TRISA) for the various port control registers */
#define TRIS_OFFSET   0x0
#define PORT_OFFSET   0x10
#define LAT_OFFSET    0x20
#define ODCF_OFFSET   0x30

/* Offsets (from TRISA) for the various io port bit manipulator registers */
#define NOP_OP_OFFSET 0x0
#define CLR_OP_OFFSET 0x4
#define SET_OP_OFFSET 0x8
#define INV_OP_OFFSET 0xc


/* 
 * Specific assignments of ports ports used
 * Note: In general, it is not sufficient to 
 * change the constants below - corresponding
 * changes to the code below will likely be
 * required if any of the following values
 * are changed.
 */

/* DATA_PORT_TRIS must be assigned to a TRIS of the port
 * that has the low 8 bits tied the 8 data bits on the ram.
 */
#define SDR_OCR		OC1CON

#define SDR_DATA_IO 	SDR_OFFSET_A
#define SDR_DATA_TRIS	SDR_OFFSET_A

#define SDR_ADDRESS_IO SDR_OFFSET_B
#define SDR_ADDRESS_TRIS SDR_OFFSET_B
#define ADDRESS_MASK 0xfafc

#define SDR_BANK_IO SDR_OFFSET_D
#define SDR_BANK_TRIS SDR_OFFSET_D
#define BANK_0_BIT 4
#define BANK_1_BIT 5

#define SDR_CONTROL_IO SDR_OFFSET_F
#define SDR_CONTROL_TRIS SDR_OFFSET_F
#define CONTROL_WE_BIT 0
#define CONTROL_CAS_BIT 1
#define CONTROL_CS_BIT 12
#define CONTROL_RAS_BIT 13

#define SDR_CKE_IO SDR_OFFSET_A
#define SDR_CKE_TRIS SDR_OFFSET_A
#define CKE_BIT 10
#define DATA_DIR_BIT 15

#define CONTROL_ALL_MASK ( (1<<CONTROL_CS_BIT) | (1<<CONTROL_RAS_BIT) | (1<<CONTROL_CAS_BIT) | (1<<CONTROL_WE_BIT) )
#define BANK_ALL_MASK ( (1 << BANK_1_BIT) | ( 1 << BANK_0_BIT ) )

	/* Global Symbols */
	.globl sdram_init
	.globl sdram_read
	.globl sdram_write
	.globl sdram_active
	.globl sdram_auto_refresh
	.globl sdram_precharge
	.globl sdram_precharge_all
	.globl sdram_sleep
	.globl sdram_wake
	.globl sdram_bank
	
	.type sdram_init, @function
	.type sdram_read, @function
	.type sdram_write, @function
	.type sdram_active, @function
	.type sdram_auto_refresh, @function
	.type sdram_precharge, @function
	.type sdram_precharge_all, @function
	.type sdram_sleep, @function
	.type sdram_wake, @function
	.type sdram_bank, @function

    /* 
     * This code MUST execute from ram and the ram MUST be configured
     * for zero wait states. Interrupts MUST disabled before
     * calling any of these functions, and any DMA MUST also be
     * disabled.
     *
     * Also, the peripheral bus divisor must be set to 1.
     */

	.section .ramfunc,"ax",@progbits

    /* No instruction reordering */

    	.set noreorder

#define clock4 \
    nop;nop;nop;nop

#define clock3 \
    nop;nop;nop

#define clock2 \
    nop;nop

#define clock1 \
    nop

/*
 * The SDRAM clock is output from the output compare unit. 
 * This macro synchronizes with that clock so that we are
 * sure to have at least two clock cycles to issue control 
 * line changes and access the data bus before the rising
 * edge.
 */
 

#define sync_clock \
    	la	$t8, TMR2;      \
    	li 	$v0, 2;	        \
    	lw 	$v1, ($t8);     \
    	bge 	$v1, $v0, 1f;   \
    	nop;	                \
    	nop;	                \
    	nop;	                \
    1:      	                \
    	nop;							 

#define output_column 						\
	and	$t5, $a0, 0x1f; 				\
	sll	$t5, 2; 					\
	and	$t6, $a0, 0x20; 				\
	sll	$t6, 9; 					\
	or	$t5, $t6; 					\
	lw	$t6, SDR_ADDRESS_TRIS + LAT_OFFSET($t0);	\
	and	$t6, ~ADDRESS_MASK; 				\
	or 	$t6, $t5; 					\
	sw	$t6, SDR_ADDRESS_TRIS + LAT_OFFSET($t0); 

#define output_address 						\
	sll	$t5, $a0, 2; 					\
	and	$t5, ADDRESS_MASK; 				\
	and	$t6, $a0, 0x40; 				\
	sll	$t6, 8; 					\
	or	$t5, $t6; 					\
	and	$t6, $a0, 0x100; 				\
	sll	$t6, 7; 					\
	or	$t5, $t6; 					\
	lw	$t6, SDR_ADDRESS_TRIS + LAT_OFFSET($t0); 	\
	and	$t6, ~ADDRESS_MASK; 				\
	or	$t6, $t5; 					\
	sw	$t6, SDR_ADDRESS_TRIS + LAT_OFFSET($t0);
	
	/*.ent sdram_init*/

/*
 * Initializes the SDRAM. 
 * Should be called once sometime after startup
 * C Prototype:
 * extern __attribute__((far)) void sdram_init();
 */

sdram_init:

	/* Initialize address lines */
	la	$t0, TRISA	/* base of io addresses */
	li	$t1, ADDRESS_MASK
	sw	$t1, SDR_ADDRESS_IO + TRIS_OFFSET + CLR_OP_OFFSET($t0)
	
	li 	$t1, 0xFFFF
	sw 	$t1, AD1PCFGSET
	
	li 	$t1, BANK_ALL_MASK 
	sw 	$t1, SDR_BANK_IO + TRIS_OFFSET + CLR_OP_OFFSET($t0)  

    	/* All address lines low */
	li	$t1, ADDRESS_MASK
	sw 	$t1, SDR_ADDRESS_IO + LAT_OFFSET + CLR_OP_OFFSET($t0)
	
	li 	$t1, BANK_ALL_MASK 
	sw 	$t1, SDR_BANK_IO + LAT_OFFSET + CLR_OP_OFFSET($t0)

	/* Initialize data lines */
	li 	$t1, 0xFF
	sw 	$t1, SDR_DATA_IO + TRIS_OFFSET + SET_OP_OFFSET($t0)

	/* Initialize SDRAM control lines */
	li 	$t1, CONTROL_ALL_MASK 
	sw 	$t1, SDR_CONTROL_IO + TRIS_OFFSET + CLR_OP_OFFSET($t0)

	/* Command Inhibit */
	li 	$t1, CONTROL_ALL_MASK 
	sw 	$t1, SDR_CONTROL_IO + LAT_OFFSET + SET_OP_OFFSET($t0)

li 	$t1, (1<<DATA_DIR_BIT) 
sw 	$t1, SDR_CKE_IO + LAT_OFFSET + SET_OP_OFFSET($t0)

	/* Initialize CKE line */
//	li 	$t1, (1<<CKE_BIT)
li 	$t1, (1<<CKE_BIT)|(1<<DATA_DIR_BIT) 
	sw 	$t1, SDR_CKE_IO + TRIS_OFFSET + CLR_OP_OFFSET($t0)
    
	/* CKE low */
	li 	$t1, (1<<CKE_BIT) 
	sw 	$t1, SDR_CKE_IO + LAT_OFFSET + CLR_OP_OFFSET($t0)

	/* SDRAM clock output */
    
	/* Initialize Timer2 */
	sw 	$zero, T2CON
	sw 	$zero, TMR2
	li	$t1, 3
	sw	$t1, PR2	
	li	$t1, 0x8000
	sw 	$t1, T2CONSET	

	/* Initialize OC device */
	sw 	$zero, SDR_OCR /*OC4CON	*/
	li	$t1, 1
	sw	$t1, SDR_OCR + OCXRS_OFFSET /*OC4RS*/		
	li	$t1, 3
	sw	$t1, SDR_OCR + OCXR_OFFSET /*OC4R*/		
	li	$t1, 0x8005
	sw 	$t1, SDR_OCR /*OC4CON*/		

	/* Clock output starts here */

	/* SD-RAM initialization delay */
	li	$t2, 500
	move	$t1, $zero
        
sdram_init_delay_1:
	addi	$t1, $t1, 1
	bne	$t1, $t2, sdram_init_delay_1
	nop

	/* CKE high */
	li 	$t1, (1<<CKE_BIT) 
	sw 	$t1, SDR_CKE_IO + LAT_OFFSET + SET_OP_OFFSET($t0)

	/* Delay some more */
	li	$t2, 3000
	move	$t1, $zero
        
sdram_init_delay_2:
	addi	$t1, $t1, 1
	bne	$t1, $t2, sdram_init_delay_2
	nop

	/* Get ready for the commands we are about to issue. */
	li 	$t4, (1<<CONTROL_CAS_BIT) 	
	li 	$t5, (1<<CONTROL_WE_BIT) 
	li 	$t6, 0x1810			/* Mode Register: CL:2, BL:8 (0x23) */
	li 	$t7, CONTROL_ALL_MASK 
	li 	$t8, 0x8000			/* A10 */

	sw	$t8, SDR_ADDRESS_IO + LAT_OFFSET + SET_OP_OFFSET($t0) /* A10 = 1 for Precharge ALL */

	sync_clock
	
	.set nomacro

	/* Precharge All */
	sw	$t7, SDR_CONTROL_IO + LAT_OFFSET + CLR_OP_OFFSET($t0) /* LLLL */
	sw	$t4, SDR_CONTROL_IO + LAT_OFFSET + SET_OP_OFFSET($t0) /* LLHL */
	clock2

	/* Auto Refresh 1 */
	sw	$t4, SDR_CONTROL_IO + LAT_OFFSET + CLR_OP_OFFSET($t0) /* LLLL */
	sw	$t5, SDR_CONTROL_IO + LAT_OFFSET + SET_OP_OFFSET($t0) /* LLLH */
	clock2

	/* Auto Refresh 2 */
	clock4

	/* Auto Refresh 3 */
	clock4

	/* Auto Refresh 4 */
	clock4

	/* Auto Refresh 5 */
	clock4

	/* Auto Refresh 6 */
	clock4

	/* Auto Refresh 7 */
	clock4

	/* Auto Refresh 8 */
	li	$t4, ADDRESS_MASK
	sw	$t4, SDR_ADDRESS_IO + LAT_OFFSET + CLR_OP_OFFSET($t0)
	clock2

	/* Load Mode Register */
	sw	$t6, SDR_ADDRESS_IO + LAT_OFFSET + SET_OP_OFFSET($t0)
	sw	$t7, SDR_CONTROL_IO + LAT_OFFSET + CLR_OP_OFFSET($t0)
	clock2

	/* Command Inhibit */
	sw	$t7, SDR_CONTROL_IO + LAT_OFFSET + SET_OP_OFFSET($t0)
	clock3 

	/* Command Inhibit */
	clock4
	
	.set macro

	jr	$ra
	nop

	/*.end sdram_init*/


/* 
 * Sends ACTIVE command 
 * C Prototype:
 * extern __attribute__((far)) void sdram_active(uint16_t rowaddr);
 */
sdram_active:

	la 	$t0, TRISA		/* Port Base */
	li 	$t7, (1<<CONTROL_CS_BIT)|(1<<CONTROL_RAS_BIT) 

	/* Set row */
	output_address
	
	sync_clock
	
	.set nomacro 
	
	/* Active */
	sw	$t7, SDR_CONTROL_IO + LAT_OFFSET + INV_OP_OFFSET($t0)
	clock3

	/* Command Inhibit */
	sw	$t7, SDR_CONTROL_IO + LAT_OFFSET + INV_OP_OFFSET($t0)
	clock3

	/* Command Inhibit */
	clock4

	.set macro
	
	jr	$ra
	nop
	

/* 
 * Sends WRITE command 
 * C Prototype:
 * extern __attribute__((far)) void sdram_write(uint16_t pseudocoladdr, uint64_t val);
 * Each pseudo column contains 8 bytes of data (consists of 8 ram columns)
 */

sdram_write:

	la 	$t0, TRISA		/* Port Base */
	li 	$t4, 0xFF
	li 	$t7, (1<<CONTROL_CS_BIT) | (1<<CONTROL_CAS_BIT) | (1<<CONTROL_WE_BIT) 
li	$t3, (1<<DATA_DIR_BIT)

	/* Set column */
	output_column

	/* Set data lines */
	srl 	$t5, $a2, 24
	sb	$t5, SDR_DATA_IO + LAT_OFFSET + NOP_OP_OFFSET($t0)

sw $t3, SDR_CKE_IO + LAT_OFFSET + INV_OP_OFFSET($t0)
	
    	sync_clock	
	
	.set nomacro 
	
	/* Write */
	sw	$t7, SDR_CONTROL_IO + LAT_OFFSET + INV_OP_OFFSET($t0) /* LHLL */
	sw	$t4, SDR_DATA_IO + TRIS_OFFSET + INV_OP_OFFSET($t0)   /* 1 - enable data lines */
	srl 	$t5, $a2, 16
	clock1

	/* Command Inhibit */
	sw	$t7, SDR_CONTROL_IO + LAT_OFFSET + INV_OP_OFFSET($t0) /* HHHH */
	sb	$t5, SDR_DATA_IO + LAT_OFFSET + NOP_OP_OFFSET($t0)    /* 2 */
	srl 	$t5, $a2, 8
	clock1

	/* Command Inhibit */
	sb	$t5, SDR_DATA_IO + LAT_OFFSET + NOP_OP_OFFSET($t0)    /* 3 */
	clock3

	/* Command Inhibit */
	sb		$a2, SDR_DATA_IO + LAT_OFFSET + NOP_OP_OFFSET($t0)    /* 4 */
	srl 	$t5, $a3, 24
	clock2

	/* Command Inhibit */
	sb	$t5, SDR_DATA_IO + LAT_OFFSET + NOP_OP_OFFSET($t0)    /* 5 */
	srl 	$t5, $a3, 16
	clock2

	/* Command Inhibit */
	sb	$t5, SDR_DATA_IO + LAT_OFFSET + NOP_OP_OFFSET($t0)    /* 6 */
	srl 	$t5, $a3, 8
	clock2

	/* Command Inhibit */
	sb	$t5, SDR_DATA_IO + LAT_OFFSET + NOP_OP_OFFSET($t0)    /* 7 */
	clock3

	/* Command Inhibit */
	sb	$a3, SDR_DATA_IO + LAT_OFFSET + NOP_OP_OFFSET($t0)    /* 8 */
	clock3

	sw	$t4, SDR_DATA_IO + TRIS_OFFSET + INV_OP_OFFSET($t0)   /* Data lines input again */

clock4 /* make certain sdram output has had time to disable - probably not needed */
sw $t3, SDR_CKE_IO + LAT_OFFSET + INV_OP_OFFSET($t0)
	.set macro
	
	jr	$ra
	nop


/* 
 * Sends READ command
 * C Prototype:
 * extern __attribute__((far)) uint64_t sdram_read(uint16_t pseudocoladdr);
 * Each pseudo column contains 8 bytes of data (consists of 8 ram columns)
 */
sdram_read:

	la 	$t0, TRISA		/* Port Base */
	li 	$t7, (1<<CONTROL_CS_BIT) | (1<<CONTROL_CAS_BIT) 

	/* Set column */
	output_column

	sync_clock

	.set nomacro 
	
	/* Read */
	sw	$t7, SDR_CONTROL_IO + LAT_OFFSET + INV_OP_OFFSET($t0) /* LHLH */
	clock3

	/* Command Inhibit */
	sw	$t7, SDR_CONTROL_IO + LAT_OFFSET + INV_OP_OFFSET($t0) /* HHHH */
	clock3

	/* Command Inhibit */
	clock3
	lbu	$v0, SDR_DATA_IO + PORT_OFFSET + NOP_OP_OFFSET($t0)       /* 1 */

	/* Command Inhibit */
	clock3
	lbu		$t5, SDR_DATA_IO + PORT_OFFSET + NOP_OP_OFFSET($t0)       /* 2 */

	/* Command Inhibit */
	sll	$v0, $v0, 8
	or	$v0, $v0, $t5
	clock1
	lbu	$t5, SDR_DATA_IO + PORT_OFFSET + NOP_OP_OFFSET($t0)       /* 3 */

	/* Command Inhibit */
	sll	$v0, $v0, 8
	or	$v0, $v0, $t5
	clock1
	lbu	$t5, SDR_DATA_IO + PORT_OFFSET + NOP_OP_OFFSET($t0)       /* 4 */

	/* Command Inhibit */
	sll	$v0, $v0, 8
	or	$v0, $v0, $t5
	clock1
	lbu	$v1, SDR_DATA_IO + PORT_OFFSET + NOP_OP_OFFSET($t0)       /* 5 */	

	/* Command Inhibit */
	clock3
	lbu	$t5, SDR_DATA_IO + PORT_OFFSET + NOP_OP_OFFSET($t0)       /* 6 */	

	/* Command Inhibit */
	sll	$v1, $v1, 8
	or	$v1, $v1, $t5
	clock1
	lbu	$t5, SDR_DATA_IO + PORT_OFFSET + NOP_OP_OFFSET($t0)       /* 7 */	

	/* Command Inhibit */
	sll	$v1, $v1, 8
	or	$v1, $v1, $t5
	clock1
	lbu	$t5, SDR_DATA_IO + PORT_OFFSET + NOP_OP_OFFSET($t0)       /* 8 */		

	/* Command Inhibit */
	sll	$v1, $v1, 8
	or	$v1, $v1, $t5

	.set macro 
	
	jr	$ra
	nop


/* 
 * Sends PRECHARGE ALL command 
 * C Prototype:
 * extern __attribute__((far)) void sdram_precharge_all(void);
 */
sdram_precharge_all:

	la 	$t0, TRISA		/* Port Base */
	li 	$t3, 0x8000		/* A10 */
	li 	$t4, (1<<CONTROL_CS_BIT) | (1<<CONTROL_RAS_BIT) | (1<<CONTROL_WE_BIT)

	sw	$t3, SDR_ADDRESS_IO + LAT_OFFSET + SET_OP_OFFSET($t0) /* A10 = 1 for Precharge ALL */

	sync_clock
	
	.set nomacro
	
	/* Precharge All */
	sw	$t4, SDR_CONTROL_IO + LAT_OFFSET + INV_OP_OFFSET($t0) /* LLHL */
	clock3

	/* Command Inhibit */
	sw	$t4, SDR_CONTROL_IO + LAT_OFFSET + INV_OP_OFFSET($t0) /* LLLL */
	clock3

	.set macro 
	
	jr	$ra
	nop


/* 
 * Sends PRECHARGE command 
 * C Prototype:
 * extern __attribute__((far)) void sdram_precharge(void);
 */
sdram_precharge:
    
	la 	$t0, TRISA		/* Port Base */
	li 	$t3, 0x8000		/* A10 */
	li 	$t4, (1<<CONTROL_CS_BIT) | (1<<CONTROL_RAS_BIT) | (1<<CONTROL_WE_BIT) 

	sw	$t3, SDR_ADDRESS_IO + LAT_OFFSET + CLR_OP_OFFSET($t0) /* A10 = 0 for Precharge */

	sync_clock
	
	.set nomacro
	
	/* Precharge All */
	sw	$t4, SDR_CONTROL_IO + LAT_OFFSET + INV_OP_OFFSET($t0) /* LLHL */
	clock3

	/* Command Inhibit */
	sw	$t4, SDR_CONTROL_IO + LAT_OFFSET + INV_OP_OFFSET($t0) /* LLLL */
	clock3

	.set macro
	
	jr	$ra
	nop


/* 
 * Sends AUTO REFRESH command 
 * All banks must be in PRECHARGEd state
 * C Prototype:
 * extern __attribute__((far)) void sdram_auto_refresh(void);
 */
sdram_auto_refresh:

	la 	$t0, TRISA		/* Port Base */
	li 	$t4, (1<<CONTROL_CS_BIT)|(1<<CONTROL_RAS_BIT)|(1<<CONTROL_CAS_BIT) 

	sync_clock
	
	.set nomacro 
	
	/* Auto Refresh */
	sw	$t4, SDR_CONTROL_IO + LAT_OFFSET + INV_OP_OFFSET($t0) /* LLLH */
	clock3

	/* Command Inhibit */
	sw	$t4, SDR_CONTROL_IO + LAT_OFFSET + INV_OP_OFFSET($t0) /* LLLL */
	clock3

	.set macro
	
	jr	$ra
	nop

/* 
 * Puts the SDRAM into the self refresh mode.
 * SDRAM retains data in this state.
 * C Prototype:
 * extern __attribute__((far)) void sdram_sleep(void);
 */
sdram_sleep:

	la 	$t0, TRISA		/* Port Base */
	li 	$t1, (1<<CKE_BIT) 
	li 	$t4, (1<<CONTROL_CS_BIT)|(1<<CONTROL_RAS_BIT)|(1<<CONTROL_CAS_BIT) 

	sync_clock
	
	.set nomacro
	
	/* Auto Refresh */
	sw	$t4, SDR_CONTROL_IO + LAT_OFFSET + INV_OP_OFFSET($t0) /* LLLH */
	sw 	$t1, SDR_CKE_IO + LAT_OFFSET + CLR_OP_OFFSET($t0)     /* CKE low */
	clock2

	/* Command Inhibit */
	sw	$t4, SDR_CONTROL_IO + LAT_OFFSET + INV_OP_OFFSET($t0) /* LLLL */
	clock3

	.set macro
	
	jr	$ra
	nop

/* 
 * Takes the SDRAM out of the self refresh mode.
 * Parameters: none
 * C Prototype:
 * extern __attribute__((far)) void sdram_wake(void);
 */
sdram_wake:

	la 	$t0, TRISA		/* Port Base */
	li 	$t1, (1<<CKE_BIT) 

	sync_clock
	
	.set nomacro
	
	/* Command Inhibit */
	sw 	$t1, SDR_CKE_IO + LAT_OFFSET + SET_OP_OFFSET($t0) /* CKE low */
	clock3

	/* Command Inhibit */
	clock4

	/* Command Inhibit */
	clock4

	.set macro
	
	jr	$ra
	nop


/* 
 * Selects the bank to which commands are issued
 * Parameters:
 * C Prototype:
 * extern __attribute__((far)) void sdram_bank(uint8_t bank);
 * (only lower 2 bits are of bank are currently significant)
 */
sdram_bank:
    
	la 	$t0, TRISA			/* Port Base */
	lw	$t1, SDR_BANK_IO + LAT_OFFSET + NOP_OP_OFFSET($t0);
	and	$t1, ~BANK_ALL_MASK
	sll     $t2, $a0, BANK_0_BIT
	or      $t1, $t2
	sw      $t1, SDR_BANK_IO + LAT_OFFSET + NOP_OP_OFFSET($t0);
	
	jr	$ra
	nop
