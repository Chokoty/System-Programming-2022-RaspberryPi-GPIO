#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>

#define IN       0
#define OUT      1
#define LOW      0
#define HIGH     1
#define PIN      20
#define POUT     21

#define VALUE_MAX       256
#define DIRECTION_MAX   45
#define BUFFER_MAX 3
#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

static const char *DEVICE = "/dev/spidev0.0";
static uint8_t MODE = SPI_MODE_0;
static uint8_t BITS = 8;
static uint32_t CLOCK = 1000000;
static uint32_t DELAY = 5;

static int GPIOExport(int pin){
   char buffer[BUFFER_MAX];
   ssize_t bytes_written;
   int fd;
   
   fd = open("/sys/class/gpio/export", O_WRONLY); // export / O_WRONLY : 쓰기 전용으로 열기
   if(-1 == fd)
   {
      fprintf(stderr, "Failed to open export for writing!\n");
      return(-1);
   }
   
   bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
   write(fd, buffer, bytes_written);
   close(fd);
   return(0);
}

static int GPIOUnexport(int pin)
{
   char buffer[BUFFER_MAX];
   ssize_t bytes_written;
   int fd;
   
   fd = open("/sys/class/gpio/unexport", O_WRONLY); // unexport !!!
   if(-1 == fd)
   {
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
      fprintf(stderr, "Failed to open giop direction for writing!\n");
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


static int PWMExport(int pwmnum)
{
    
    char buffer[BUFFER_MAX];
    int bytes_written;
    int fd;
    
    fd = open("/sys/class/pwm/pwmchip0/unexport", O_WRONLY);
    if(-1 == fd){
        fprintf(stderr, "Failed to open in unexport!\n");
        return(-1);
    }
    
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
    write(fd, buffer, bytes_written);
    close(fd);

    sleep(1);
    fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
    if(-1 == fd){
        fprintf(stderr, "Failed to open in export!\n");
        return(-1);
    }
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
    write(fd, buffer, bytes_written);
    close(fd);
    sleep(1);
    return(0);
}


static int PWMEnable(int pwmnum)
{
    static const char s_unenable_str[] = "0";
    static const char s_enable_str[] = "1";

    char path[DIRECTION_MAX];
    int fd;

    snprintf(path,DIRECTION_MAX,"/sys/class/pwm/pwmchip0/pwm%d/enable",pwmnum);
    fd = open(path, O_WRONLY);
    if(-1 == fd){
        fprintf(stderr, "Failed to open in enable!\n");
        return -1;
    }

    write(fd, s_unenable_str, strlen(s_unenable_str));
    close(fd);

    fd = open(path, O_WRONLY);
    if (-1 == fd){
        fprintf(stderr, "Failed to open in enable!\n");
        return -1;
    }

    write(fd, s_enable_str, strlen(s_enable_str));
    close(fd);
    return(0);

}

static int PWMWritePeriod(int pwmnum, int value)
{
    char s_values_str[VALUE_MAX];
    char path[VALUE_MAX];
    int fd, byte;

    snprintf(path,DIRECTION_MAX,"/sys/class/pwm/pwmchip0/pwm%d/period",pwmnum);
    fd = open(path, O_WRONLY);
    if (-1 == fd){
        fprintf(stderr, "Failed to open in period!\n");
        return(-1);
    }
    byte = snprintf(s_values_str,VALUE_MAX,"%d",value);

    if (-1 == write(fd, s_values_str, byte)){
        fprintf(stderr, "Failed to write value in period!\n");
        close(fd);
        return(-1);
    }

    close(fd);
    return(0);
}

static int PWMWriteDutyCycle(int pwmnum, int value)
{

    char path[VALUE_MAX];
    char s_values_str[VALUE_MAX];
    int fd, byte;

    snprintf(path,DIRECTION_MAX,"/sys/class/pwm/pwmchip0/pwm%d/duty_cycle",pwmnum);
    fd = open(path, O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open in duty_cycle!\n");
        return(-1);
    }

    byte = snprintf(s_values_str,VALUE_MAX,"%d",value);

    if (-1 == write(fd, s_values_str, byte)) {
        fprintf(stderr, "Failed to write value! in duty_cycle\n");
        close(fd);
        return(-1);
    }

    close(fd);
    return(0);
}


static int prepare(int fd)
{
    
    if(ioctl(fd, SPI_IOC_WR_MODE, &MODE) == -1){
        perror("Can't set MODE");
        return -1;
    }

    if(ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &BITS) == -1){
        perror("Can't set number of BITS");
        return -1;
    }

    if(ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &CLOCK) == -1){
        perror("Can't set write CLOCK");
        return -1;
    }

    if(ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &CLOCK) == -1){
        perror("Can't set read CLOCK");
        return -1;
    }

    return 0;
}

uint8_t control_bits_differential(uint8_t channel)
{
    return (channel & 7) << 4;
}

uint8_t control_bits(uint8_t channel){
    return 0x8 | control_bits_differential(channel);
}
// 조도센서 데이터 읽어오기
int readadc(int fd, uint8_t channel){
    uint8_t tx[] = {1, control_bits(channel), 0};
    uint8_t rx[3];

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = ARRAY_SIZE(tx),
        .delay_usecs = DELAY,
        .speed_hz = CLOCK,
        .bits_per_word = BITS,
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == 1){
        perror("IO Error");
        abort();
    }

    return ((rx[1] << 8) & 0x300) | (rx[2] & 0xFF);
}


int main(int argc, char *argv[])
 {

   int state = 1;
   int prev_state = 1;
   int light1 = 0;
   int light2 = 0;

    // bcm 18
    PWMExport(0);
    PWMWritePeriod(0, 20000000); 
    PWMWriteDutyCycle(0, 0);
    PWMEnable(0);
    
    // bcm 27
    PWMExport(1);
    PWMWritePeriod(1, 20000000);
    PWMWriteDutyCycle(1, 10000);
    PWMEnable(1);

    int fd = open(DEVICE, O_RDWR);
    if (fd <= 0){
        printf("Device %s not found\n", DEVICE);
        return -1;
    }

    if (prepare(fd) == -1){
        return -1;
    }

   // enable GPIO pins : pin 20-in, 21-out
   if(-1 == GPIOExport(POUT) || -1 == GPIOExport(PIN) )
      return(1);
   
   // set GPIO directions 
   if(-1 == GPIODirection(POUT, OUT) || -1 == GPIODirection(PIN, IN))
      return(2);
   
   do
  {
      if(-1 == GPIOWrite(POUT,1)) //버튼 활성화
         return(3);
      //printf("GPIORead : %d from pin %d\n", GPIORead(PIN), PIN);
      
      // trigger 
      if (0 == GPIORead(PIN))       // 21 value 체크해서 led 켜기
      {
         light1 = readadc(fd, 0); //조도센서에서 읽어온 데이터
         light2 = readadc(fd, 1);
         printf("light1: %d\n",light1);
         printf("light2: %d\n",light2);
         PWMWriteDutyCycle(0, light1*7500);//pwm0
         PWMWriteDutyCycle(1, light2*7500);//pwm1
      } 
      usleep(1000); // 1초
   }
   while(1);

   // disable GPIO pins : 
   if(-1 == GPIOUnexport(POUT) || -1 == GPIOUnexport(PIN))
      return(4);

    close(fd);
    return(0);
}
