#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <pthread.h>
#include "gpio.h"

// led
#define PWM 0
#define POUT2 17   
// ultra  
#define POUT 23    
#define PIN  24

int isCheck = 0;
int light = 0;

int sock;
struct sockaddr_in serv_addr;
char msg2[2];
char msg[4];
char on[2] = "1";
int str_len;
double distance = 0;


void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

static int PWMExport(int pwmnum){

  char buffer[BUFFER_MAX];
  int bytes_written;
  int fd;
  
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

static int PWMUnexport(int pwmnum){

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

    return(0);
}

static int PWMEnable(int pwmnum){

    static const char s_unenable_str[] = "0";
    static const char s_enable_str[] = "1";

    char path[DIRECTION_MAX];
    int fd;

    snprintf(path,DIRECTION_MAX,"/sys/class/pwm/pwmchip0/pwm%d/enable", pwmnum);
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

static int PWMUnable(int pwmnum){

    static const char s_unenable_str[] = "0";
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
    return(0);

}

static int PWMWritePeriod(int pwmnum, int value){

    char s_values_str[VALUE_MAX];
    char path[VALUE_MAX];
    int fd, byte;

    snprintf(path,DIRECTION_MAX,"/sys/class/pwm/pwmchip0/pwm%d/period",pwmnum);
    fd = open(path, O_WRONLY);
    if (-1 == fd){
        fprintf(stderr, "Failed to open in period!\n");
        return(-1);
    }
    byte = snprintf(s_values_str,10,"%d",value);
    printf("%d\n", byte);
    if (-1 == write(fd, s_values_str, byte)){
        fprintf(stderr, "Failed to write value in period!\n");
        close(fd);
        return(-1);
    }

    close(fd);
    return(0);
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

void *ultrawave_thd(){
  clock_t start_t, end_t;
  double time;
  if (-1 == GPIOExport(POUT) || -1 == GPIOExport(PIN)){
      printf("gpio export err\n");
      exit(0);
  }
  usleep(100000);

  if (-1 == GPIODirection(POUT, OUT) || -1 == GPIODirection(PIN, IN)){
      printf("gpio direction err\n");
      exit(0);
  }
  GPIOWrite(POUT, 0);
  usleep(10000);

  int idx = 0;
  while (1){
    if (-1 == GPIOWrite(POUT, 1)){
      printf("gpio write\trigger err\n");
      exit(0);
    }
    usleep(10);
    GPIOWrite(POUT, 0);

    while (GPIORead(PIN) == 0){
        start_t = clock();
    }
    while (GPIORead(PIN) == 1){
        end_t = clock();
    }
    time = (double)(end_t - start_t) / CLOCKS_PER_SEC;
    distance = time / 2 * 34000;

    if (distance > 900)
        distance = 900;

    // printf("time : %.4lf\n", time);
    //printf("distance : %.2lfcm\n", distance);
    
    
    //======> 초음파 요청 조건 설정
    int y = 0;
    if(distance < 15){
      y = 1;
      printf("서버에 요청하는중... \n");
      snprintf(msg2, 2, "%d\n", y);
      write(sock, msg2, sizeof(msg2));
    }else if(distance >= 15){
      y = 0;
      snprintf(msg2, 2, "%d\n", y);
      write(sock, msg2, sizeof(msg2));
    }
    
    usleep(500 * 1000);

  }
}

void *led_thd(){
    int target_bright = 0;
    int prev_bright = 0;

    
    PWMExport(PWM);
    PWMWritePeriod(PWM, 20000000);
    PWMWriteDutyCycle(PWM, 0);
    PWMEnable(PWM);



    while(1){
        
        // ======> led 쓰레드 서버 메시지 읽기
        str_len = read(sock, msg, sizeof(msg));
        if (str_len == -1)
            error_handling("read() error");

        // light 값 계산
        light = atoi(msg);
        printf("light: %d\n", light);

        target_bright = 10000000 - (light * 30000);
        if(target_bright > 10000000){
            target_bright = 10000000;
        } else if(target_bright < 0) {
            target_bright = 0;
        }

        printf("targetbright: %d\n", target_bright);
        PWMWriteDutyCycle(PWM, target_bright);
  }

    exit(0);
}

int main(int argc, char *argv[])
{

    pthread_t p_ultra;
    pthread_t p_led;
    int thr_id;
    int thr_id2;
    int status;
    char pM[] = "thread_m";
    char pN[] = "thread_n";


    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    // Enable GPIO pins
    if (-1 == GPIOExport(POUT))
        return (1);
    // Set GPIO directions
    if (-1 == GPIODirection(POUT, OUT))
        return (2);


    // ======> 클라이언트 소캣 오픈 
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    // ======> 클라이언트 쓰레드 생성 - ultra, led 
    thr_id2 = pthread_create(&p_ultra, NULL, ultrawave_thd, NULL);
    if (thr_id2 < 0){
        perror("thread create error : ");
        exit(0);
    }
    usleep(10000);
    
    thr_id = pthread_create(&p_led, NULL, led_thd, NULL);
    if (thr_id < 0){
        perror("thread create error : ");
        exit(0);
    }

    usleep(100000 * 60);


    // ultrawave_thd((void *)pM);
    // led_thd((void *)pN);
    pthread_join(p_ultra, (void **)&status);
    pthread_join(p_led, (void **)&status);
   
    // while (1){
    //     str_len = read(sock, msg, sizeof(msg));
    //     if (str_len == -1)
    //         error_handling("read() error");

    //     printf("Receive message from Server : %s\n", msg);
    //     if (strncmp(on, msg, 1) == 0)
    //         light = 1;
    //     else
    //         light = 0;
    //     GPIOWrite(POUT, light);
    // }

    close(sock);
    
    // Disable PWM
    PWMUnexport(PWM);

    // Disable GPIO pins
    if (-1 == GPIOUnexport(POUT))
        return (4);

    return (0);
}