#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <arpa/inet.h>

#include<linux/spi/spidev.h>
#include<linux/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "gpio.h"

#define PIN 20
#define POUT 21


// 조도센서 
#define PIN 20
#define POUT 21
#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

socklen_t clnt_addr_size;

static const char *DEVICE = "/dev/spidev0.0";
static uint8_t MODE = SPI_MODE_0;
static uint8_t BITS = 8;
static uint32_t CLOCK = 1000000;
static uint16_t DELAY = 5;


// (SGL/DIF = 0, D2=D1=D0=0)
uint8_t control_bits_differential(uint8_t channel){
  return (channel & 7) << 4;
}

// (SGL/DIF = 1, D2=D1=D0=0)
uint8_t control_bits(uint8_t channel){
  return 0x8 | control_bits_differential(channel);
}

void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}


// check spi is prepared 
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

// read spi
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


int main(int argc, char *argv[]){
    int state = 1;
    int prev_state = 1;
    int light = 0;

    int serv_sock, clnt_sock = -1;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    // char msg2[2];
    char msg[4];
    char on[2] = "1";
    int str_len;

    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
    }

    // ======> 서버 소캣 오픈
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");
        
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    if (clnt_sock < 0){
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1)
            error_handling("accept() error");
    }
    if(clnt_sock > 0){
      printf("%d found client!!!\n", clnt_addr.sin_addr);
    }


    // ======> 조도센서 세팅부분
    int fd = open(DEVICE, O_RDWR);
    if (fd <= 0 ){
        printf("DEVICE %s no found\n", DEVICE);
        return -1;
    }
    if (prepare(fd) == -1){
        return -1;
    }
    int seq = 0;


    //======> 소캣 통신
    while (1){

      char msg2[2];
       if (clnt_sock < 0){
            clnt_addr_size = sizeof(clnt_addr);
            clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        }
        if (clnt_sock == -1)
            error_handling("accept() error");
       
        //======> ultra 쓰레드 메시지 읽기
      str_len = read(clnt_sock, msg2, sizeof(msg2));
      if (str_len == -1)
        error_handling("read() error");

        if (strncmp(on, msg2, 1) == 0){

            // send light value to client
            light = readadc(fd, 0);
            printf("light: %d\n", light);
            snprintf(msg, 4, "%d", light);
            write(clnt_sock, msg, sizeof(msg));
        }

        usleep(500 * 1000);
    }



    close(clnt_sock);
    close(serv_sock);

    // Disable GPIO pins
    if (-1 == GPIOUnexport(PIN) || -1 == GPIOUnexport(POUT))
        return (4);
    return (0);
}