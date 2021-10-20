#ifndef _KERNEL_H_
#define _KERNEL_H_

#include "stdint.h"

#define NULL	(void*)0
#define FALSE	(0)
#define TRUE 	(!FALSE)

// Allowed task states
#define TASK_STATE_DEAD 					0x00
#define TASK_STATE_RUNNING 					0x01
#define TASK_STATE_SLEEPING					0x02
#define TASK_STATE_WAITING_FOR_RESUME		0x04

// Sleep options
#define SLEEP_FOREVER		0xFFFFFFFF

// These are the registers automatically pushed on the current stack 
// by the Cortex core on exception entering
struct EXCEPTION_CONTEXT {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	union {
		uint32_t r12;
		uint32_t ip;
	};
	union {
		uint32_t r14;
		uint32_t lr;
	}; 
	union {
		uint32_t r15;
		uint32_t pc;
	};
	uint32_t xpsr;
};

// These are the remaining registers which must be saved manually for
// a complete context switch
struct CONTEXT {
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	union {
		uint32_t r7;
		uint32_t fp;
	};	
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	union {
		uint32_t r14;
		uint32_t lr;
	}; 
	struct EXCEPTION_CONTEXT exc;
};

struct TASK {
	uint8_t* curr_stack_ptr;	// pointer to the current stack location
	uint8_t* total_stack_ptr;	// pointer to the beginning of the stack
	uint32_t stack_size;	// NOTE: the stack size is expressed in words (32 bits)
	uint8_t status;		// status of the task
	uint8_t priority;
	uint16_t id;
	char* name;
	void (*func)(void* arg); 
	uint32_t resume_at_tickcount;
	struct TASK* next_task;
};

#define ALLOCATE_TASK(_name_, _size_, _priority_, _main_func_)	\
	uint8_t __attribute__((aligned(4))) _name_##_stack[_size_];	\
	struct TASK _name_ = {	\
		.total_stack_ptr = (_name_##_stack) + sizeof(_name_##_stack) - 1,	\
		.curr_stack_ptr = (_name_##_stack) + sizeof(_name_##_stack) - 1,	\
		.stack_size = _size_,	\
		.priority = _priority_, \
		.resume_at_tickcount = 0,	\
		.id = 0,	\
		.name = #_name_, \
		.func = _main_func_, \
		.next_task = NULL,	\
		.status = TASK_STATE_DEAD,	\
	};

// Core functions
void kernel_main(void);
void pendsv_handler(void);
void svc_handler(void);

// General purpose functions
void kernel_init_task(struct TASK* task_ptr);
void kernel_activate_task_after_ms(struct TASK* task, uint32_t delay);
void kernel_activate_task_immediately(struct TASK* task);
void kernel_task_sleep(uint32_t sleep_time);
uint8_t kernel_get_task_status(struct TASK* task_ptr);
void kernel_task_kill(struct TASK* task_ptr);

// This macro must be used to define a module's initialization function
#define MODULE_INIT_FUNCTION(name)   \
	void name##_module_init(void);   \
	void (*name##_module_init_ptr)(void) __attribute__((section(".modules_init"))) = name##_module_init;   \
	void name##_module_init()

#endif // _KERNEL_H_