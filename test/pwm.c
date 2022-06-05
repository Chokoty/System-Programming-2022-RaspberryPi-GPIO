#include <wiringPi.h>
#include <stdio.h>

#define LED 13

int bright = 0;

int main(){
  if( wiringPiSetup() == -1){
    return 1;
  }

  pinMode(LED, PWM_OUTPUT);
  digitalWrite(LED, LOW);

  while(1){
    for(bright=0; bright < 100; ++bright){
      pwmWrite(LED, bright);
      delay(50);
    }
    for(bright=100; bright >= 0; --bright){
      pwmWrite(LED, bright);
      delay(50);
    }
  }
  return 0;
}