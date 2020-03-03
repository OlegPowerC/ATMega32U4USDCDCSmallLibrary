#define ENABLE_BIT_DEFINITIONS
#include "ioavr.h"
#include "USB_cdc_user_functions.h"
//Функция вызываеться при приходе данных по USB
//Принятые данные находяться в буффере USB_recived_buffer
//Количество принятых байт находиться в USB_recived_buffer_size
//Максимальный размер равен размеру RX Endpoint - 32 байта

#define CMD1 0x31
#define CMD2 0x32
#define CMD3 0x33
#define CMD4 0x34

void TimerDelayInit(void){
  tick = 0;
  OCR0A = 0xFF;
  TCCR0A = (1 <<WGM01);
  TCCR0B = (1 << CS00)|(1 << CS02);
  TIMSK0 = (1 << TOIE0);
}

void TimerDelayStop(void){
  OCR0A = 0xFF;
  TCCR0A = (1 <<WGM01);
  TCCR0B = 0;
  TIMSK0 = (1 << TOIE0);
  TCNT0 = 0;
}

unsigned int tick;

#pragma vector=TIMER0_OVF_vect
__interrupt void OnOVF1(void){
  if(tick == 10){
    //PORTA |= (1 << PINA0)|(1 << PINA1)|(1 << PINA2)|(1 << PINA3);
    TimerDelayStop();
  }else{
     tick++;
  }
}

void callback_USB_RXC(void)
{
  switch(USB_recived_buffer[0]){
  //case CMD1:PORTA &= ~(1 << PINA0);TimerDelayInit();break;
  //case CMD2:PORTA &= ~(1 << PINA1);TimerDelayInit();break;
  //case CMD3:PORTA &= ~(1 << PINA2);TimerDelayInit();break;
  //case CMD4:PORTA &= ~(1 << PINA3);TimerDelayInit();break;
  default : break;
  }

  //Для демонстрации нашего USB порта, сделаем эхо
  USB_send_buffer(USB_recived_buffer_size,&USB_recived_buffer[0]);
  
}

