#include "stdint.h"
#include "uart.h"
#include "stm32f103xb.h"
#include "kernel.h"
#include "gpio.h"
#include "utils.h"
#include "clock.h"

#define BAUD_RATE		115200		// Desired baud rate

#define __DIV_SAMPLING16(_PCLK_, _BAUD_)            (((_PCLK_)*25)/(4*(_BAUD_)))
#define __DIVMANT_SAMPLING16(_PCLK_, _BAUD_)        (__DIV_SAMPLING16((_PCLK_), (_BAUD_))/100)
#define __DIVFRAQ_SAMPLING16(_PCLK_, _BAUD_)        (((__DIV_SAMPLING16((_PCLK_), (_BAUD_)) - (__DIVMANT_SAMPLING16((_PCLK_), (_BAUD_)) * 100)) * 16 + 50) / 100)
#define __UART_BRR_SAMPLING16(_PCLK_, _BAUD_)       ((__DIVMANT_SAMPLING16((_PCLK_), (_BAUD_)) << 4)|(__DIVFRAQ_SAMPLING16((_PCLK_), (_BAUD_)) & 0x0F))

/*
 *
 */
MODULE_INIT_FUNCTION(InitUART)
{
	// Enable GPIO peripheral's clock
	SET_BITS(RCC->APB2ENR, RCC_APB2ENR_IOPDEN);
	SET_BITS(RCC->APB2ENR, RCC_APB2ENR_AFIOEN);
	// Set pin PD5 (TX): output + alternate function + push pull
	MODIFY_REG(GPIOD->CRL, GPIO_CRL_MODE5_Msk, GPIO_MODE_OUTPUT_50MHz << GPIO_CRL_MODE5_Pos);
	MODIFY_REG(GPIOD->CRL, GPIO_CRL_CNF5_Msk, GPIO_CNF_OUTPUT_ALT_FUNC_PUSH_PULL << GPIO_CRL_CNF5_Pos);
	// Set pin PD6 (RX): input
	MODIFY_REG(GPIOD->CRL, GPIO_CRL_MODE6_Msk, GPIO_MODE_INPUT << GPIO_CRL_MODE6_Pos);
	MODIFY_REG(GPIOD->CRL, GPIO_CRL_CNF6_Msk, GPIO_CNF_INPUT_FLOATING << GPIO_CRL_CNF6_Pos);
	// Remap USART2 pins to PD5 and PD6
	SET_BITS(AFIO->MAPR, AFIO_MAPR_USART2_REMAP);
	
	// Enable UART peripheral's clock
	SET_BITS(RCC->APB1ENR, RCC_APB1ENR_USART2EN);
	// Enable UART1
	SET_BITS(USART2->CR1, USART_CR1_UE);
	// BRR (Baud Rate Register)
	USART2->BRR = __UART_BRR_SAMPLING16(clock_get_PCLK1_freq(), BAUD_RATE);
	// Enable TX for this UART
	SET_BITS(USART2->CR1, USART_CR1_TE);
}

/*
 *
 */
void UART_putc(char c)
{
	// If UART is disabled then exit immediately without doing nothing
	if (!ARE_BITS_SET(USART2->CR1,USART_CR1_UE)){
		return;
	}
	// Wait for TXE (Transmit Data Register Empty)
	while (!ARE_BITS_SET(USART2->SR, USART_SR_TXE));
	// write data to the output register
	USART2->DR = (uint8_t)c;
}