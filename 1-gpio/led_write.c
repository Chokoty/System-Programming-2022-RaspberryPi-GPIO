#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#include "gpio.h"

#define POUT	17


int main(int argc, char *argv[])
{
	int repeat = 20; // 반복 횟수

	// enable GPIO pins : gpio/export에 핀번호 전달 => 활성화
	if(-1 == GPIOExport(POUT)) // pin 17 활성화
		return(1);
	
	// set GPIO directions : 활성화한 pin의 direction을 out으로 설정
	if(-1 == GPIODirection(POUT, OUT)) // pin 17 -> out(1)
		return(2);
	
	do
	{
		if(-1 == GPIOWrite(POUT,repeat % 2)) // pin의 value에 0, 1 번갈아가며 5번 쓰기 - led : 0/1 off/on 
			return(3); // value == 0 이면 종료
		usleep(300 * 1000); // 실행늦추기: usleep 마이크로초 _sleep 밀리초 sleep 초
		// 500 * 1000 : 0.5초
	}
	while(repeat--);

	// disable GPIO pins : 
	if(-1 == GPIOUnexport(POUT)) // pin 17 비활성화
		return(4);
	
	return(0);
	
}

