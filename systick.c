#include <stdint.h>
#include "systick.h"
#include "stm32f103xb.h"
#include "clock.h"

/* 1 ms per tick. */
#define TICK_RATE_HZ	1000

uint32_t tick_count = 0;

/*
 * Initialize the SysTick module in order to have 1 interrupt every millisecond
 */
void systick_init()
{
  SysTick->LOAD  = (uint32_t)((clock_get_HCLK_freq()/(8*TICK_RATE_HZ)) - 1UL);                         
  NVIC_SetPriority (SysTick_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); 
  SysTick->VAL   = 0UL;                                            
  SysTick->CTRL  = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;  
}

/*
 * Return the current tick count
 */
uint32_t systick_get_tick_count()
{
	return tick_count;
}

/*
 * Wait until the desired amount of time is expired 
 */
void systick_blocking_delay(uint32_t ticks)
{
	uint32_t start_tick = systick_get_tick_count();
	while (systick_get_tick_count()-start_tick < ticks);
}

/*
 * SysTick handler - Just increment the counter
 */
__attribute__((interrupt)) void systick_handler()
{
	tick_count++;
}