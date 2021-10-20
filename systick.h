#ifndef _SYSTICK_H_
#define _SYSTICK_H_

void systick_init(void);
uint32_t systick_get_tick_count(void);
void systick_blocking_delay(uint32_t ticks);

void systick_handler(void);

#endif // _SYS_TICK_H_