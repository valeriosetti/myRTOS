#ifndef _CLOCK_H_
#define _CLOCK_H_

uint32_t clock_get_HSE_freq(void);
uint32_t clock_get_HCLK_freq(void);
uint32_t clock_get_PCLK1_freq(void);
uint32_t clock_get_PCLK2_freq(void);

#endif // _CLOCK_H_