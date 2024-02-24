#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/select.h>
#include <stdio.h>
#include <signal.h>

#include "mychar.h"


int fd = -1;
void input_handler(int signo);

int main(int argc,char *argv[])
{
	int ret;
	int flags;

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

	fcntl(fd, F_SETOWN, getpid());
	
	flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flags | FASYNC);

	signal(SIGIO, input_handler);
	
	while(1)
	{
	}

	close(fd);
	fd = -1;
	return 0;
}

void input_handler(int signo)
{
	char buf[8] = "";
	read(fd, buf, 8);
	printf("buf=%s\n",buf);
}



