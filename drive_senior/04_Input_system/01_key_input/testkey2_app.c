#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/input.h>
#include <stdio.h>


int main(int argc,char *argv[])
{
	int fd = -1;
	struct input_event evt;

	if(argc < 2)
	{
		printf("The argument is too few\n");
		return 1;
	}

	fd = open(argv[1],O_RDONLY);
	if(fd < 0)
	{
		printf("open %s failed\n",argv[1]);
		return 3;
	}

	while(1)
	{
		read(fd, &evt, sizeof(evt));
		if(evt.type == EV_KEY && evt.code == KEY_2)
		{
			if(evt.value)
			{
				printf("Key2 is down!\n");
			}
			else
			{
				printf("Key2 is up!\n");
			}
		}
	}

	close(fd);
	fd = -1;
	return 0;
}
