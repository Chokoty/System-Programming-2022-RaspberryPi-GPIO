#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<stdint.h>
#include<getopt.h>
#include<sys/ioctl.h>
#include<linux/types.h>
#include<linux/spi/spidev.h>

#define IN 0
#define OUT 1
#define PWM 2

#define LOW	0
#define HIGH	1

#define PIN 20
#define POUT 21

#define VALUE_MAX	256

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

static const char *DEVICE = "/dev/spidev0.0";
static uint8_t MODE = SPI_MODE_0;
static uint8_t BITS = 8;
static uint32_t CLOCK = 1000000;
static uint16_t DELAY = 5;

static int PWMExport(int pwmnum){

#define BUFFER_MAX 3
    char buffer[BUFFER_MAX];
    int bytes_written;
    int fd;

// echo해서 export하기 - 쓰기만가능
// unexport 열기 - 쓰기만
    fd = open("/sys/class/pwm/pwmchip0/unexport", O_WRONLY);     // unexport
    if (-1 == fd){
        fprintf(stderr, "Failed to open in unexport!\n");
        return (-1);
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
    write(fd, buffer, bytes_written);
    close(fd);
    sleep(1);

// export 열기 - 쓰기만
    fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);     // export
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open in export!\n");
        return (-1);
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
    write(fd, buffer, bytes_written);
    close(fd);
    sleep(1);

    return (0);
}

static int PWMEnable(int pwmnum){

  static const char s_unenable_str[] = "0";
  static const char s_enable_str[] = "1";

#define DIRECTION_MAX 45
  char path[DIRECTION_MAX];
  int fd;

// cd /sys/class/pwm/pwmchip0/pwm%d/
// echo 1 > enable 실행하기
  snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm%d/enable", pwmnum); // Q1.
  
// unenable 열기 - 쓰기만
  fd = open(path, O_WRONLY);  // unenable
  if(-1 == fd){
    fprintf(stderr, "Failed to open in unenable!\n");
    return -1;
  }

  write(fd, s_unenable_str, strlen(s_unenable_str));
  close(fd);

// enable 열기 - 쓰기만
  fd = open(path, O_WRONLY);  // enable
  if(-1 == fd){
    fprintf(stderr, "Failed to open in enable!\n");
    return -1;
  }

  write(fd, s_enable_str, strlen(s_enable_str));
  close(fd);

  return 0;
}


static int PWMWritePeriod(int pwmnum, int value){

  char s_values_str[VALUE_MAX];
  char path[VALUE_MAX];
  int fd, byte;

// cd /sys/class/pwm/pwmchip0/pwm%d/
// echo 20000000 > peroiod 실행하기
  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/period", pwmnum); // Q2. 
  
// period 열기 - 쓰기만
  fd = open(path, O_WRONLY);
  if(-1 == fd){
    fprintf(stderr, "Failed to open in period!\n");
    return -1;
  }

// value 문자열로 바꾸고 결과 btye에 넣기
  byte = snprintf(s_values_str, VALUE_MAX, "%d", value); // Q3.

// 문자열로 바꾼 value를 period에 쓰기
  if(-1 == write(fd, s_values_str, byte)){
    fprintf(stderr, "Failed to write value in period!\n");
    close(fd);
    return -1;
  }
  close(fd);
  
  return 0;
}

static int PWMWriteDutyCycle(int pwmnum, int value){

  char s_values_str[VALUE_MAX];
  char path[VALUE_MAX];
  int fd, byte;

// cd /sys/class/pwm/pwmchip0/pwm%d/
// echo 80000000 > duty_cycle 실행하기
  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", pwmnum); // Q4. 

// duty_cycle 열기 - 쓰기만
  fd = open(path, O_WRONLY);
  if(-1 == fd){
    fprintf(stderr, "Failed to open in duty_cycle!\n");
    return -1;
  }

// value 문자열로 바꾸고 결과 btye에 넣기
  byte = snprintf(s_values_str, VALUE_MAX, "%d", value); // Q5. 
  
// 문자열로 바꾼 value를 period에 쓰기
  if(-1 == write(fd, s_values_str, byte)){
    fprintf(stderr, "Failed to write value in duty_cycle!\n");
    close(fd);
    return -1;
  }
  close(fd);

  return 0;
}

// ensure all settings are correct for the ADC
static int prepare(int fd){

  if (ioctl(fd, SPI_IOC_WR_MODE, &MODE) == -1){
    perror("Can't set MODE");
    return -1;
  }
  if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &BITS) == -1){
    perror("Can't set number of BITS");
    return -1;
  }
  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &MODE) == -1){
    perror("Can't set write CLOCK");
    return -1;
  }
  if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &MODE) == -1){
    perror("Can't set read CLOCK");
    return -1;
  }

  return 0;
}

// (SGL/DIF = 0, D2=D1=D0=0)
uint8_t control_bits_differential(uint8_t channel){
  return (channel & 7) << 4;
}

// (SGL/DIF = 1, D2=D1=D0=0)
uint8_t control_bits(uint8_t channel){
  return 0x8 | control_bits_differential(channel);
}

/*
 *  given a prep'd descriptor, and an ADC channel, fech the 
 *  raw ADC value for teh given channel.
*/
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

int main(int argc, char **argv){


  int light = 0;

  PWMExport(0); // pwm0 is gpio18
  PWMWritePeriod(0, 20000000);
  PWMWriteDutyCycle(0, 0);
  PWMEnable(0);
  
  // 실행부분
  int fd = open(DEVICE, O_RDWR);
  if (fd <= 0 ){
    printf("DEVICE %s no found\n", DEVICE);
    return -1;
  }

  if (prepare(fd) == -1){
    return -1;
  }

  while(1){
    light = readadc(fd, 0); // 300
    printf("%d\n", light);
    PWMWriteDutyCycle(0, light * 7500); // 펄스 조절하는부분
    usleep(50000);
  }
  close(fd);

  return 0;
    
}