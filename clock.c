#include "stdint.h"
#include "stm32f103xb.h"
#include "clock.h"
#include "utils.h"
#include "kernel.h"

#define HSE 	12000000
#define HCLK 	72000000
#define PCLK1   36000000
#define PCLK2  	72000000

MODULE_INIT_FUNCTION(rcc_clock_init)
{
	SET_BITS(RCC->CR, RCC_CR_HSEON);
	while (!ARE_BITS_SET(RCC->CR, RCC_CR_HSERDY));

	// Enable Prefetch Buffer (CHECK!!!)
	SET_BITS(FLASH->ACR, FLASH_ACR_PRFTBE);
	// Flash 0 wait state
	MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY_Msk, FLASH_ACR_LATENCY_0);

	// Set the PLL input source from HSE
	SET_BITS(RCC->CFGR, RCC_CFGR_PLLSRC);
	// Set the PLL's multiplier to 6 (12MHz x 6 = 72MHz) - This will be HCLK
	MODIFY_REG(RCC->CFGR, RCC_CFGR_PLLMULL_Msk, RCC_CFGR_PLLMULL6);
	// Turn on the PLL and wait for it to stabilize
	SET_BITS(RCC->CR, RCC_CR_PLLON);
	while (!ARE_BITS_SET(RCC->CR, RCC_CR_PLLRDY));
	
	// AHB prescaler set to 1 --> HCLK=SYSCLK
	MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE_Msk, RCC_CFGR_HPRE_DIV1);
	// PCLK1 = SYSCLK/2 = 36MHz
	MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1_Msk, RCC_CFGR_PPRE1_DIV2);
	// PCLK2 = SYSCLK / 1 = 72MHz
	MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2_Msk, RCC_CFGR_PPRE2_DIV1);

	// Set HSE+PLL as system's clock
	MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL);
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

/*
 * Return the external crystal's frequency 
 */
uint32_t clock_get_HSE_freq()
{
	return HSE;
}

/*
 * Todo: replace it with something computed dynamically
 */
uint32_t clock_get_HCLK_freq()
{
	return HCLK;
}

/*
 * Todo: replace it with something computed dynamically
 */
uint32_t clock_get_PCLK1_freq()
{
	return PCLK1;
}

/*
 * Todo: replace it with something computed dynamically
 */
uint32_t clock_get_PCLK2_freq()
{
	return PCLK2;
}
