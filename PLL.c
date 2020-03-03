#define ENABLE_BIT_DEFINITIONS
#include "ioavr.h"
#include "PLL.h"
        
void init_pll(void)
{
//������� 8��� PINDIV  = 0
  PLLCSR = (1 << PLLE);
  while(!(PLLCSR & (1 << PLOCK)))
  {
    ;
  }
}

void stop_PLL(void)
{
  PLLCSR = 0x00;
}