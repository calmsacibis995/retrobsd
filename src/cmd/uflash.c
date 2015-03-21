#include <stdio.h>
#include <sys/uflash.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <string.h>

void report_loaded()
{
	if( UFLASH_HEADPTR->magic != UFLASH_MAGIC )
	{
		printf("It appears that no userland program is loaded into flash\n" );
		exit(2);
	}

	printf( "%s is loaded into flash\n", &UFLASH_HEADPTR->name[0] );
}

#define UFLASH_DEV_PATH "/dev/uflash"

int main(int argc, char*argv[])
{
	if( argc < 2 )
	{
		report_loaded();
		exit(0);
	}

	if( argc > 2 )
	{
		printf( "useage: uflash loadable_executable_name" );
		exit(1);
	}

	int fd = open(UFLASH_DEV_PATH, O_RDWR);
	if( fd == -1 )
	{
		printf("Failed to open %s\n", UFLASH_DEV_PATH );
		exit(1);
	}
	
	char temp[1024];
	strcpy( temp, "/uflash/" );
	strcat( temp, argv[1] );
	strcat( temp, ".bin" );

	write(fd,temp,strlen(temp));

	int load_result = ioctl(fd,UFLASH_LOAD,0);


	if( load_result == -1 )
 	{
		printf("Failed to load %s into flash\n", argv[1] );
		close(fd);
		exit(1);
	}

	close(fd);
	printf( "Loaded %s into flash\n", argv[1] );
	return 0;
}

