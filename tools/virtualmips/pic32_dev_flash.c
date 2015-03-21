/*
 * Internal Flash memory of PIC32 microcontroller.

 * Copyright (C) 2011 Serge Vakulenko <serge@vak.ru>
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>

#include "device.h"
#include "mips_memory.h"
#include "pic32.h"

#define FLASH_PAGE_SIZE 4096

/* flash private data */
typedef struct flash_data {
    struct vdevice *dev;
    m_uint8_t *flash_ptr;
    m_uint32_t flash_size;
    m_uint8_t *flash_file_name;
} flash_data_t;

int dev_flash_flash_op( struct vdevice *dev, m_uint32_t addr, u_int op, m_uint32_t extra )
{
	flash_data_t *d = dev->priv_data;
	m_uint32_t offset = addr - dev->phys_addr;
	
	if( offset >= d->flash_size ) {
		fprintf(stderr, "Flash operation out-of-bounds addr=%d offset=%d\n", addr, offset );
		return -1;
	}
	switch( op )
	{
		case PIC32_NVMCON_PAGE_ERASE:
			{
				if( offset != (offset & ~(FLASH_PAGE_SIZE - 1)) )
				{
					fprintf(stderr, "Flash operation misaligned\n" );
					return -1;
				}
				
				int i;
				for( i = 0; i < FLASH_PAGE_SIZE; ++i )
				{
					*(unsigned *) (d->flash_ptr + offset + i) = ~0U;
				}
				return 0;
			}
		case PIC32_NVMCON_WORD_PGM:
			{
				if( offset != (offset & ~3) )
				{
					fprintf(stderr, "Flash operation misaligned\n" );
					return -1;
				}
			    * (unsigned *) (d->flash_ptr + offset) = *(unsigned *)(d->flash_ptr + offset) & extra;
				return 0;
			}
		case PIC32_NVMCON_ROW_PGM:
			{
				fprintf(stderr, "Flash operation unsupported\n" );
				return -1;
			}
	}
	fprintf(stderr, "Unknown Flash Operation\n" );
	return -1;
}

void *dev_flash_access (cpu_mips_t * cpu, struct vdevice *dev,
    m_uint32_t offset, u_int op_size, u_int op_type,
    m_uint32_t * data, m_uint8_t * has_set_value)
{
    flash_data_t *d = dev->priv_data;

    if (offset >= d->flash_size) {
        printf ("-- flash: access %08x -- out of memory\n", dev->phys_addr + offset);
        *data = 0xff;
        *has_set_value = TRUE;
        return NULL;
    }
    if (op_type == MTS_READ) {
#if 0
        printf ("-- flash: read %08x -> %08x\n", dev->phys_addr + offset,
                *(unsigned *) (d->flash_ptr + offset));
#endif
        return d->flash_ptr + offset;
    }
    if (op_type == MTS_WRITE) {
        printf ("-- flash: write %08x ***\n", dev->phys_addr + offset);
        return NULL;
    }
    assert (0);
}

static int dev_flash_load (char *flash_file_name, unsigned flash_len,
    unsigned char **flash_data_hp)
{
    int fd;
    struct stat sb;

    fd = open (flash_file_name, O_RDONLY);
    if (fd < 0) {
        fprintf (stderr, "%s does not exist.\n", flash_file_name);
        return (-1);
    }
    fstat (fd, &sb);
    if (flash_len < sb.st_size) {
        fprintf (stderr,
            "Too large flash file.\nFlash len: %d kbytes, file name %s, "
            "file legth: %d bytes.\n", flash_len / 1024, flash_file_name,
            (int) sb.st_size);
        return (-1);
    }
    if (sb.st_size <= 0) {
        fprintf (stderr, "%s: empty flash file.\n", flash_file_name);
        return (-1);
    }
    char* tm = mmap (NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (tm == MAP_FAILED) {
        fprintf (stderr, "%s: ", flash_file_name);
        perror ("mmap");
        return (-1);
    }
    *flash_data_hp = malloc( flash_len );
    memcpy( *flash_data_hp, tm, sb.st_size );
    munmap( tm, sb.st_size );
    close( fd );
    return 0;
}

static void dev_flash_reset (cpu_mips_t * cpu, struct vdevice *dev)
{
    //flash_data_t *d = dev->priv_data;

    //d->state = 0;
}

/*
 * Initialize a NOR Flash zone
 */
int dev_pic32_flash_init (vm_instance_t * vm, char *name,
    unsigned flash_kbytes, unsigned flash_address, char *filename)
{
    flash_data_t *d;
    unsigned char *flash_data_hp;

    /* load rom data */
    if (dev_flash_load (filename, flash_kbytes * 1024, &flash_data_hp) < 0)
        return (-1);

    /* allocate the private data structure */
    d = malloc (sizeof (*d));
    if (!d) {
        fprintf (stderr, "FLASH: unable to create device.\n");
        return (-1);
    }

    memset (d, 0, sizeof (*d));
    d->flash_ptr = flash_data_hp;
    d->flash_size = flash_kbytes * 1024;

    d->dev = dev_create (name);
    if (!d->dev) {
        free (d);
        return (-1);
    }
    d->dev->priv_data = d;
    d->dev->phys_addr = flash_address;
    d->dev->phys_len = flash_kbytes * 1024;
    d->dev->handler = dev_flash_access;
    d->dev->reset_handler = dev_flash_reset;
    d->dev->flags = VDEVICE_FLAG_NO_MTS_MMAP;

    /* Map this device to the VM */
    vm_bind_device (vm, d->dev);
    return (0);
}
