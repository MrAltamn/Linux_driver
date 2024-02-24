#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "mychar.h"
#include <stdio.h>

#include "mychar.h"
int main(int argc,char *argv[])
{
	int fd = -1;
	char buf[8] = "";
	int ret;

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

	ret = read(fd,buf,7);
	if(ret < 0)
	{
		printf("read data failed\n");
	}
	else
	{
		printf("buf=%s\n",buf);
	}

	close(fd);
	fd = -1;
	return 0;
}
