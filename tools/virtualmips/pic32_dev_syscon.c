/*
 * System controller for PIC32.
 *
 * Copyright (C) 2011 Serge Vakulenko <serge@vak.ru>
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "device.h"
#include "mips_memory.h"
#include "cpu.h"
#include "pic32.h"

#define SYSCON_REG_SIZE     0x1000

extern cpu_mips_t *current_cpu;
extern int dev_flash_flash_op( struct vdevice *dev, m_uint32_t addr, u_int op, m_uint32_t extra );

static int syskey_unlock;

static inline unsigned write_op (a, b, op)
{
    switch (op & 0xc) {
    case 0x0:           /* Assign */
        a = b;
        break;
    case 0x4:           /* Clear */
        a &= ~b;
        break;
    case 0x8:           /* Set */
        a |= b;
        break;
    case 0xc:           /* Invert */
        a ^= b;
        break;
    }
    return a;
}

static void soft_reset (cpu_mips_t *cpu)
{
    pic32_t *pic32 = (pic32_t*) cpu->vm->hw_data;

    mips_reset (cpu);
    cpu->pc = pic32->start_address;

    /* reset all devices */
    dev_reset_all (cpu->vm);
    dev_sdcard_reset (cpu);
}

void *dev_pic32_syscon_access (cpu_mips_t *cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type, m_reg_t *data,
    m_uint8_t *has_set_value)
{
    pic32_t *pic32 = dev->priv_data;

    
    if (offset >= SYSCON_REG_SIZE) {
        *data = 0;
        return NULL;
    }

    int keep_nvmstate = 0;

    if (op_type == MTS_READ)
        *data = 0;
    switch (offset & 0xff0) {
    case PIC32_OSCCON & 0xff0:
        if (op_type == MTS_READ) {
            *data = pic32->osccon;
            if (cpu->vm->debug_level > 2)
                printf ("        read OSCCON -> %08x\n", *data);
        } else {
            pic32->osccon = *data;
            if (cpu->vm->debug_level > 2)
                printf ("        OSCCON := %08x\n", *data);
        }
        break;

    case PIC32_OSCTUN & 0xff0:
        if (op_type == MTS_READ) {
            *data = pic32->osctun;
            if (cpu->vm->debug_level > 2)
                printf ("        read OSCTUN -> %08x\n", *data);
        } else {
            pic32->osctun = *data;
            if (cpu->vm->debug_level > 2)
                printf ("        OSCTUN := %08x\n", *data);
        }
        break;

    case PIC32_DDPCON & 0xff0:          /* Debug Data Port Control */
        if (op_type == MTS_READ) {
            *data = pic32->ddpcon;
            if (cpu->vm->debug_level > 2)
                printf ("        read DDPCON -> %08x\n", *data);
        } else {
            pic32->ddpcon = *data;
            if (cpu->vm->debug_level > 2)
                printf ("        DDPCON := %08x\n", *data);
        }
        break;

    case PIC32_DEVID & 0xff0:           /* Device identifier */
        /* read-only register */
        if (op_type == MTS_READ) {
            *data = pic32->devid;
            if (cpu->vm->debug_level > 2)
                printf ("        read DEVID -> %08x\n", *data);
        }
        break;

    case PIC32_SYSKEY & 0xff0:
        if (op_type == MTS_READ) {
            *data = pic32->syskey;
            if (cpu->vm->debug_level > 2)
                printf ("        read SYSKEY -> %08x\n", *data);
        } else {
            pic32->syskey = *data;
            if (cpu->vm->debug_level > 2)
                printf ("        SYSKEY := %08x\n", *data);

            /* Unlock state machine. */
            switch (syskey_unlock) {
            case 0:
                if (pic32->syskey == 0xaa996655)
                    syskey_unlock = 1;
                else
                    syskey_unlock = 0;
                break;
            case 1:
                if (pic32->syskey == 0x556699aa)
                    syskey_unlock = 2;
                else
                    syskey_unlock = 0;
                break;
            default:
                syskey_unlock = 0;
                break;
            }
        }
        break;
    case PIC32_RCON & 0xff0:
        if (op_type == MTS_READ) {
            *data = pic32->rcon;
            if (cpu->vm->debug_level > 2)
                printf ("        read RCON -> %08x\n", *data);
        } else {
            pic32->rcon = *data;
            if (cpu->vm->debug_level > 2)
                printf ("        RCON := %08x\n", *data);
        }
        break;
    case PIC32_RSWRST & 0xff0:
        if (op_type == MTS_READ) {
            *data = pic32->rswrst;
            if (cpu->vm->debug_level > 2)
                printf ("        read RSWRST -> %08x\n", *data);
        } else {
            pic32->rswrst = *data;
            if (cpu->vm->debug_level > 2)
                printf ("        RSWRST := %08x\n", *data);

            if (syskey_unlock == 2 && (pic32->rswrst & 1))
                soft_reset (cpu);
        }
        break;
    case PIC32_NVMCON & 0xff0:             
        if (op_type == MTS_READ) {
            *data = pic32->nvmcon;
            if( *data & PIC32_NVMCON_WR ) {
		        pic32->nvmcon &= ~PIC32_NVMCON_WR;
			} 
        } else {
            pic32->nvmcon = write_op (pic32->nvmcon, *data, offset);
            if( pic32->nvmstate == 2 && pic32->nvmcon & PIC32_NVMCON_WR ) {
                switch( pic32->nvmcon & PIC32_NVMCON_NVMOP )
				{
					case PIC32_NVMCON_PAGE_ERASE:
					case PIC32_NVMCON_WORD_PGM:
						dev_flash_flash_op( pic32->flash_device, 
                                            pic32->nvmaddr, 
                                            pic32->nvmcon & PIC32_NVMCON_NVMOP, 
                                            pic32->nvmdata );
						break;
					default:
						fprintf(stderr, "Unsupported/unknown flash operation\n" );
						break;
				}
			}
        }
        break;
    case PIC32_NVMKEY & 0xff0:             
        if (op_type == MTS_READ) {
            *data = pic32->nvmkey;
        } else {
            pic32->nvmkey = write_op (pic32->nvmkey, *data, offset);
            if( pic32->nvmcon & PIC32_NVMCON_WREN 
                 && pic32->nvmkey == 0xAA996655 ) {
                pic32->nvmstate = 1;
                keep_nvmstate = 1;
                pic32->nvmcon &= ~PIC32_NVMCON_WR;
            } else if( pic32->nvmstate == 1 
                        && pic32->nvmcon & PIC32_NVMCON_WREN 
                        && pic32->nvmkey == 0x556699AA ) {
                pic32->nvmstate = 2;
                keep_nvmstate = 1;
            }
        }
        break;
    case PIC32_NVMADDR & 0xff0:             
        if (op_type == MTS_READ) {
            *data = pic32->nvmaddr;
        } else {
            pic32->nvmaddr = write_op (pic32->nvmaddr, *data, offset);
        }
        break;
    case PIC32_NVMDATA & 0xff0:             
        if (op_type == MTS_READ) {
            *data = pic32->nvmdata;
        } else {
            pic32->nvmdata = write_op (pic32->nvmdata, *data, offset);
        }
        break;
    case PIC32_NVMSRCADDR & 0xff0:             
        if (op_type == MTS_READ) {
            *data = pic32->nvmsrcaddr;
        } else {
            pic32->nvmsrcaddr = write_op (pic32->nvmsrcaddr, *data, offset);
        }
        break;
    default:
        ASSERT (0, "unknown syscon offset %x\n", offset);
    }
    *has_set_value = TRUE;
    if( !keep_nvmstate ) pic32->nvmstate = 0;
    return NULL;
}

void dev_pic32_syscon_reset (cpu_mips_t *cpu, struct vdevice *dev)
{
    pic32_t *pic32 = dev->priv_data;

    pic32->osccon = 0x01453320; /* from ubw32 board */
    pic32->osctun = 0;
    pic32->ddpcon = 0;
    pic32->devid = 0x04307053;  /* 795F512L */
    pic32->syskey = 0;
    pic32->rcon = 0;
    pic32->rswrst = 0;
    syskey_unlock = 0;
    pic32->nvmcon = 0;
    pic32->nvmkey = 0;
    pic32->nvmaddr = 0;
    pic32->nvmdata = 0;
    pic32->nvmsrcaddr = 0;
    pic32->nvmstate = 0;

}

int dev_pic32_syscon_init (vm_instance_t *vm, char *name, unsigned paddr)
{
    pic32_t *pic32 = (pic32_t *) vm->hw_data;

    pic32->sysdev = dev_create (name);
    if (! pic32->sysdev)
        return (-1);
    pic32->sysdev->priv_data = pic32;
    pic32->sysdev->phys_addr = paddr;
    pic32->sysdev->phys_len = SYSCON_REG_SIZE;
    pic32->sysdev->handler = dev_pic32_syscon_access;
    pic32->sysdev->reset_handler = dev_pic32_syscon_reset;
    pic32->sysdev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    pic32->flash_device = vm_find_device( vm, "Program flash" );
    if( !pic32->flash_device ) 
    {
        fprintf( stderr, "Unable to find Program Flash device\n");
        return (-1);
    }
    vm_bind_device (vm, pic32->sysdev);
    return (0);
}
