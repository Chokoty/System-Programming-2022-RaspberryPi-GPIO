#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1

#define BUFFER_MAX 3 // GPIOExport
#define DIRECTION_MAX	45 // GPIODirection
#define VALUE_MAX 256 // GPIO read write


static int GPIOExport(int pin){

	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;
	
	fd = open("/sys/class/gpio/export", O_WRONLY); // export / O_WRONLY : 쓰기 전용으로 열기
	if(-1 == fd){
		fprintf(stderr, "Failed to open export for writing!\n");
		return(-1);
	}
	
	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

static int GPIOUnexport(int pin){

	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;
	
	fd = open("/sys/class/gpio/unexport", O_WRONLY); // unexport !!!
	if(-1 == fd){
		fprintf(stderr, "Failed to open unexport for writing!\n");
		return(-1);
	}
	
	bytes_written = snprintf(buffer, BUFFER_MAX, "%d",pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}


static int GPIODirection(int pin, int dir) {
	static const char s_directions_str[] = "in\0out";
	
	char path[DIRECTION_MAX]="/sys/class/gpio/gpio%d/direction"; // 해당 pin의 direction 
	int fd;
	
	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction",pin);
	
	fd = open(path, O_WRONLY);
	if(-1 == fd)
	{
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		return(-1);
	}
	
	if(-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3))
	{
		fprintf(stderr, "Failed to set direction!\n");
		close(fd);
		return(-1);
	}
	close(fd);
	return(0);
}

static int GPIORead(int pin){

	char path[VALUE_MAX];
	char value_str[3];
	int fd;
	
	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_RDONLY); // read

	if (-1 == fd)
	{
		fprintf(stderr, "Failed to open gpio value for reading!\n");
		return(-1);
	}
	
	if (-1 == read(fd, value_str, 3))
	{
		fprintf(stderr, "Failed to read value!\n");
		close(fd);
		return(-1);
	}

	close(fd);
	return(atoi(value_str)); // 숫자로 변환
}

static int GPIOWrite(int pin, int value){
	static const char s_values_str[] = "01";
	
	char path[VALUE_MAX];
	int fd;
	
	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY); // write
	if (-1 == fd)
	{
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		return(-1);
	}
	
	if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1))
	{
		fprintf(stderr, "Failed to write value!\n");
		close(fd);
		return(-1);
	}
	close(fd);
	return(0);
}
