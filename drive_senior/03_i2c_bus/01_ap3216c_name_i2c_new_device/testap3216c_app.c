#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "ap3216c.h"

int main(int argc, const char *argv[])
{
	int fd;
	union ap3216c_data data; 
	
	fd = open("/dev/ap3216c", O_RDWR);
	if (fd < 0) {
		perror("open");
		exit(1);
	}

	while(1) {
		ioctl(fd, GET_ALS, &data);
		printf("als data: als = %04x\n", data.als);

		ioctl(fd, GET_PS, &data);
		printf("ps data: ps = %04x\n", data.ps);
		
		ioctl(fd, GET_LED, &data);
		printf("led data：led = %04x\n",data.led);
		printf("\n");
		sleep(1);
	}

	close(fd);

	return 0;
}

