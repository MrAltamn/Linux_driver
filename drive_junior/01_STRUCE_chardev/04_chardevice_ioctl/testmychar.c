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
	ioctl(fd,MYCHAR_IOCTL_GET_MAXLEN,&max);
	printf("max len is %d\n",max);

	write(fd,"hello mydevchar",15);
	
	ioctl(fd,MYCHAR_IOCTL_GET_CURLEN,&cur);
	printf("cur len is %d\n",cur);
	
	read(fd,buf,17);
	printf("buf=%s\n",buf);

	close(fd);
	fd = -1;
	return 0;
}
