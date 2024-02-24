#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <stdio.h>

#include "mychar.h"
int main(int argc,char *argv[])
{
	int fd = -1;
	char buf[8] = "";
	int max = 0;
	int cur = 0;

	if(argc < 2)
	{
		printf("The argument is too few\n");
		return 1;
	}

	fd = open(argv[1],O_RDWR);
	if(fd < 0)
	{
		printf("open %s failed\n",argv[1]);
		return 2;
	}

	write(fd,"hello",6);
	

	read(fd,buf,8);
	printf("buf=%s\n",buf);

	close(fd);
	fd = -1;
	return 0;
}
