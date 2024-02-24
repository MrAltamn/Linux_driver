#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unisted.h>


int main(int argc, char *argv[])
{
	int fd = -1;

	if(argc < 2)	
	{
		printf("The argument is too few\n");
		return 1;
	}

	fd = open(argv[1], O_RDONLY);
	if(fd < 0)
	{
		printf("open %s failed\n", argv[1]);
		return 2;
	}

	close(fd);
	fd = -1;
	return 0;
}





































