#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>

#define IN 0  // 여기서는 안씀
#define OUT 1 // 여기서는 안씀
#define PWM 2 // 여기서는 안씀

#define LOW	0   // 여기서는 안씀
#define HIGH	1 // 여기서는 안씀
#define VALUE_MAX	256

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

int main(){

// gpio 설정
  PWMExport(0); // pwm0 is gpio18
  PWMWritePeriod(0, 20000000);
  PWMWriteDutyCycle(0, 0);
  PWMEnable(0);

// 동작 수행 - 천천히 켜지고 꺼짐 
  while(1){
    for(int i = 0; i < 1000; i++){
      PWMWriteDutyCycle(0, i * 10000);
      usleep(1000);
    }
    for(int i = 1000; i > 0; i--){
      PWMWriteDutyCycle(0, i * 10000);
      usleep(1000);
    }
  }
}