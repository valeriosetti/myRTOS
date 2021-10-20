#include "stdint.h"
#include "systick.h"
#include "stm32f103xb.h"
#include "kernel.h"
#include "debug_printf.h"

#define debug_msg(_format_, ...)	DebugPrintf("[Kernel] " _format_, ##__VA_ARGS__)

// Private variables
uint32_t tasks_count = 0;
struct TASK* active_tasks_list = NULL;  // list of active tasks (ordered based on priority)
struct TASK* dead_tasks_list = NULL;  // list of dead tasks (this is not ordered, of course)
struct TASK* active_task = NULL;  // pointer to the current active task (NULL if there's no active task)
ALLOCATE_TASK(kernel, 1024, 0, NULL)  // This is the stack used for the kernel

/* Exception return behavior */
#define HANDLER_MSP	0xFFFFFFF1
#define THREAD_MSP	0xFFFFFFF9
#define THREAD_PSP	0xFFFFFFFD

// Some global symbols taken from the linker file
extern uint32_t _modules_init_start;
extern uint32_t _modules_init_end;

// Private functions
static void kernel_add_task_to_list(struct TASK* input_task_ptr, struct TASK** task_list_ptr);
static struct TASK* kernel_get_next_task_to_run();
static void kernel_initialize_modules();

/********************************************************************/
/*	KERNEL - CONTEXT SWITCH	*/
/********************************************************************/
#define set_pendsv()		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk

uint8_t context_switch_direction; 
	#define SWITCH_TO_TASK		0
	#define SWITCH_TO_KERNEL	1
	
/*
 * Global interrupt control macros
 */
#define kernel_disable_configurable_interrupts()		//__asm("cpsid i")
#define kernel_enable_configurable_interrupts()			//__asm("cpsie i")
#define kernel_disable_all_interrupts()					//__asm("cpsid f")
#define kernel_enable_all_interrupts()					//__asm("cpsie f")

/*
 * Activate the specified task from the kernel
 * This function is called by the kernel for activating a specific task
 */	
static void kernel_activate_task(uint8_t* stack_ptr)
{
	active_task->status = TASK_STATE_RUNNING;
	context_switch_direction = SWITCH_TO_TASK;
	kernel_disable_configurable_interrupts();
	set_pendsv();
}

/*
 * Put the current task to sleep
 * This function is called by generic functions in order to give the control
 * back to the kernel. The sleep can be limited in time or infinite, which means
 * that the task must be resumed by some other event.
 */
void kernel_task_sleep(uint32_t sleep_ms)
{
	kernel_disable_configurable_interrupts();
	if (sleep_ms == SLEEP_FOREVER) {
		active_task->status = TASK_STATE_WAITING_FOR_RESUME;
	} else {
		active_task->resume_at_tickcount = systick_get_tick_count() + sleep_ms;
		active_task->status = TASK_STATE_SLEEPING;
	}
	__asm("svc 1");
}

/*
 * This is the return point for all the tasks which terminate their own main function.
 */
__attribute__((interrupt)) void kernel_task_unexpected_death()
{
	kernel_disable_configurable_interrupts();
	active_task->status = TASK_STATE_DEAD;
	__asm("svc 2");
}

/*
 * PendSV handler
 * Load the new context from R0
 *
 * NOTE: registers {r0,r1,r2,r3,r12,r14,r15,xPSR} are automatically saved by the hardware
 * 		when entering exceptions. As a consequence the first thing to do is to update the 
 *		stack pointer of the current task/kernel. Then the remaining registers must be 
 *		pushed onto the current stack, updating the stack pointer once again.
 *		Once context saving is completed, the new task can be loaded! The oppposite behavior
 *		is expected in this case: some registers are loaded manually from the stack, whereas
 *		the remaining ones are automatically loaded from the hardware.
 */
__attribute__((interrupt, naked)) void pendsv_handler(void)
{
	register uint32_t* _r0  __ASM("r0");
	// get a copy of the current stack pointer in R0
	if (context_switch_direction == SWITCH_TO_TASK) {
		kernel.curr_stack_ptr = (uint8_t*)__get_MSP();
		_r0 = (uint32_t*)kernel.curr_stack_ptr;
		// save current task's registers - only the registers which were not automatically save by the 
		// ARM core will be pushed here
		__asm("stmdb r0!, {r4, r5, r6, r7, r8, r9, r10, r11, lr}");
		kernel.curr_stack_ptr = (uint8_t*)_r0;
		__set_MSP((uint32_t)_r0);
		// get the new stack pointer in R0
		_r0 = (uint32_t*)active_task->curr_stack_ptr;
		// manually reload registers which are not automatically restored by the core on exception exit
		__asm("ldmia r0!, {r4, r5, r6, r7, r8, r9, r10, r11, lr}");
		// update the PSP stack pointer	
		__set_PSP((uint32_t)_r0);
		// exit the interrupt (all the other registers are automatically reloaded)
		__asm("bx lr");
	} else {
		active_task->curr_stack_ptr = (uint8_t*)__get_PSP();
		_r0 = (uint32_t*)active_task->curr_stack_ptr;
		// save current task's registers - only the registers which were not automatically save by the 
		// ARM core will be pushed here
		__asm("stmdb r0!, {r4, r5, r6, r7, r8, r9, r10, r11, lr}");
		active_task->curr_stack_ptr = (uint8_t*)_r0;
		// get the new stack pointer in R0
		_r0 = (uint32_t*)kernel.curr_stack_ptr;
		// manually reload registers which are not automatically restored by the core on exception exit
		__asm("ldmia r0!, {r4, r5, r6, r7, r8, r9, r10, r11, lr}");
		// update the MSP stack pointer	
		__set_MSP((uint32_t)_r0);
		// exit the interrupt (all the other registers are automatically reloaded)
		__asm("bx lr");
	}
	kernel_enable_configurable_interrupts();
}

/*
 * SVC handler
 * Stack contains eight 32-bit values:
 * r0, r1, r2, r3, r12, r14, return address, xPSR
 * 1st argument = r0 = svc_args[0]
 * 2nd argument = r1 = svc_args[1]
 * 7th argument = return address = svc_args[6]
 */
__attribute__((interrupt)) void svc_handler()
{
	uint8_t svc_number = ((uint8_t*)((struct EXCEPTION_CONTEXT*)__get_PSP())->pc)[-2];
		
	switch (svc_number) {
		case 1:  // Set current task to sleep		
			break;
		case 2:	// the active task reached the end of its main function
			debug_msg("Task %s terminated\n", active_task->name);
			break;
		default:  // Unknown operation
			break;
	}
	
	context_switch_direction = SWITCH_TO_KERNEL;
	set_pendsv();
}

/********************************************************************/
/*	KERNEL - PRIVATE FUNCTIONS	*/
/********************************************************************/
/*
 * Add a the task to the specified list
 */
static void kernel_add_task_to_list(struct TASK* input_task_ptr, struct TASK** task_list_ptr)
{
	// first task in the list
	if (*task_list_ptr == NULL) {
		*task_list_ptr = input_task_ptr;
	} else { // following elements
		// look for the right place on the list based on the task's priority
		struct TASK* curr_task_ptr = *task_list_ptr;
		struct TASK* prev_task_ptr = NULL;
		while ((curr_task_ptr != NULL) && ((input_task_ptr->priority)>(curr_task_ptr->priority))) {
			prev_task_ptr = curr_task_ptr;
			curr_task_ptr = curr_task_ptr->next_task;
		}
		
		if (prev_task_ptr == NULL) {	
			// replace the first element in the list
			input_task_ptr->next_task = curr_task_ptr;
			*task_list_ptr = input_task_ptr;
		} else if (curr_task_ptr == NULL) {
			// add to the end of the list
			prev_task_ptr->next_task = input_task_ptr;
		} else {
			// add the element in the middle of the list
			prev_task_ptr->next_task = input_task_ptr;
			input_task_ptr->next_task = curr_task_ptr;
		}
	}
	input_task_ptr->id = tasks_count;
	tasks_count ++;
}

/*
 * Remove the selected task from the specified list
 */
static int32_t kernel_remove_task_from_list(struct TASK* task_ptr, struct TASK** task_list_ptr)
{
	// If the list is empty there's no task which can be removed
	if (*task_list_ptr == NULL) {
		return -1;
	}
	
	struct TASK* prev_ptr = NULL;
	struct TASK* curr_ptr = *task_list_ptr;
	
	// Find the specified task
	while (curr_ptr != task_ptr) {
		if (curr_ptr->next_task != NULL) {
			// Move to the next element
			prev_ptr = curr_ptr;
			curr_ptr = curr_ptr->next_task;
		} else {
			// End of the list reached but the task was not found!
			return -1;
		}
	}
	
	if (prev_ptr == NULL) {	// The task is at the beginning of the list
		*task_list_ptr = curr_ptr->next_task;
	} else { // The task is in the middle or at the end of the list
		prev_ptr->next_task = curr_ptr->next_task;
	}
	
	return 0;
}

/*
 * Returns a pointer to the next active task which should be set on execution.
 * A NULL value is returned if there's no ready task to run.
 */
static struct TASK* kernel_get_next_task_to_run()
{
	uint32_t current_tick_count = systick_get_tick_count();
	struct TASK* curr_task_ptr = active_tasks_list;
	
	while (curr_task_ptr != NULL) {
		if (curr_task_ptr->status == TASK_STATE_SLEEPING) {
			if (current_tick_count >= curr_task_ptr->resume_at_tickcount)
				return curr_task_ptr;
		} 
		curr_task_ptr = curr_task_ptr->next_task;
	}
	return NULL;
}

/*
 * This simply kills a task
 */
void kernel_task_kill(struct TASK* task_ptr)
{
	task_ptr->status = TASK_STATE_DEAD;
	if (kernel_remove_task_from_list(task_ptr, &active_tasks_list) >= 0) {
		kernel_add_task_to_list(task_ptr, &dead_tasks_list);
	}
}

/*
 * Run through all the init functions included in the "init" section
 */
//typedef void module_init_func_ptr(void);
static void kernel_initialize_modules()
{
	uint32_t* func_ptr = &_modules_init_start;
	
	while (func_ptr < &_modules_init_end) {
		((void (*)(void)) *func_ptr)();
		func_ptr++;
	}
}

/*
 * Initialize the specified task's stack
 */
static void kernel_prepare_task_stack(struct TASK* task_ptr)
{
	struct CONTEXT* context_ptr = (struct CONTEXT*)(task_ptr->total_stack_ptr - (uint8_t*)sizeof(struct CONTEXT) + 1);
	context_ptr->lr = (uint32_t) THREAD_PSP;
	context_ptr->exc.lr = (uint32_t) kernel_task_unexpected_death;
	context_ptr->exc.pc = (uint32_t) task_ptr->func;
	context_ptr->exc.xpsr = (uint32_t) 0x01000000; /* PSR Thumb bit */
	// Set the task's structure properties
	task_ptr->curr_stack_ptr = (uint8_t*)context_ptr;
}

/*
 * Fill the specified task's stack with a pattern byte
 */
#define STACK_PATTERN 		0xAA
static void kernel_fill_stack_with_pattern(struct TASK* task)
{
	uint8_t* stack_start = task->total_stack_ptr;
	uint8_t* stack_end = task->total_stack_ptr - task->stack_size + 1;
	
	uint8_t* ptr = stack_start;
	
	while(ptr >= stack_end) {
		*ptr = STACK_PATTERN;
		ptr--;
	}
}

/*
 * Go through the task's stack and check how many pattern bytes 
 * are still unchanged
 */
__attribute__((unused)) static uint32_t kernel_get_stack_usage(struct TASK* task)
{
	uint8_t* stack_start = task->total_stack_ptr;
	uint8_t* stack_end = task->total_stack_ptr - task->stack_size + 1;
	
	uint8_t* ptr = stack_end;
	
	while ((ptr <= stack_start) && (*ptr == STACK_PATTERN)) {
		ptr++;
	}
	return (uint32_t)(stack_start - ptr);
}

/*
 * This is the first kernel function called after reset and it includes the scheduler.
 * The function is "naked" because we don't need any prologue/epilogue as we're never 
 * supposed to exit from this looping function.
 */
__attribute__((naked)) void kernel_main(void)
{ 
	// initialize all the modules
	kernel_initialize_modules();
	// fill the kernel stack with the predefined pattern
	kernel_fill_stack_with_pattern(&kernel);
	// set the MSP to the beginning of the kernel's stack
	__set_MSP((uint32_t)kernel.total_stack_ptr);
	// Configure PendSV priority
	NVIC_SetPriority(PendSV_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL);
	// Configure SysTick
	systick_init();
	debug_msg("Initialization completed. Launching scheduler\n");
	// Scheduler loop
	while (1) {
		active_task = kernel_get_next_task_to_run();
		if (active_task != NULL) {
			kernel_activate_task(active_task->curr_stack_ptr);
			// Execution will return here once the task has released the control
			// debug_msg("Task %s - stack usage %d/%d\n", active_task->name, kernel_get_stack_usage(active_task), active_task->stack_size);
			active_task = NULL;
		}
	}
}

/********************************************************************/
/*	KERNEL - PUBLIC FUNCTIONS	*/
/********************************************************************/
/*
 * Initialize the task's stack and add it to the tasks' list
 */
void kernel_init_task(struct TASK* task_ptr)
{
	kernel_fill_stack_with_pattern(task_ptr);	// for debug purposes
	kernel_prepare_task_stack(task_ptr);
	kernel_add_task_to_list(task_ptr, &dead_tasks_list);
}

/*
 * The selected task will be enabled and scheduled after the desired amount of milliseconds
 */
void kernel_activate_task_after_ms(struct TASK* task_ptr, uint32_t delay)
{
	if (task_ptr != NULL) {
		if (task_ptr->status == TASK_STATE_DEAD) {
			kernel_prepare_task_stack(task_ptr);
		}
		task_ptr->status = TASK_STATE_SLEEPING;
		task_ptr->resume_at_tickcount = systick_get_tick_count() + delay;
		if (kernel_remove_task_from_list(task_ptr, &dead_tasks_list) >= 0) {
			kernel_add_task_to_list(task_ptr, &active_tasks_list);
		}
	}
}

/*
 * The selected task will be enabled and scheduled as soon as possible (based on its priority)
 */
void kernel_activate_task_immediately(struct TASK* task_ptr)
{
	kernel_activate_task_after_ms(task_ptr, 0UL);
}

/*
 * Return the status of the selected task
 */
uint8_t kernel_get_task_status(struct TASK* task_ptr)
{
	return task_ptr->status;
}
