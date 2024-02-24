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

#include "mychar.h"



int main(int argc,char *argv[])
{
	int fd = -1;
	char buf[8] = "";
	int ret;
	fd_set rfds;

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
	while(1)
	{

		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		ret = select(fd + 1, &rfds, NULL,NULL, NULL);
		if(ret < 0)
		{
			if(errno == EINTR)
			{
				continue;
			}
			else
			{
				printf("select error\n");
				break;
			}
		}
		if(FD_ISSET(fd, &rfds))
		{
			read(fd, buf, 8);
			printf("buf=%s\n",buf);
		}
	}

	close(fd);
	fd = -1;
	return 0;
}
