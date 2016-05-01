#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, const char *argv[])
{
	int fd;
	int l1,l2;

	if ( argc <2 )
	{
		printf(">[%s] can't find argv[1]... \n",argv[0]);
		exit (EXIT_FAILURE);
	}

	fd = open( argv[1] , O_RDWR );
	if ( fd < 0 )
	{
		printf(">file open fail...\n");
		return errno;
	}
	
	l1=read ( fd , "1" , 1);
	l2=write ( fd , "1" , 1 );

	printf("open => %d\nread => %d\nwrite => %d\n",fd,l1,l2);
	return 0;
}
