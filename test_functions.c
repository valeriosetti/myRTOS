#include "kernel.h"
#include "debug_printf.h"
#include "systick.h"

#define debug_msg(_format_, ...)	DebugPrintf("[Test] " _format_, ##__VA_ARGS__)

void task1_func(void* arg)
{
	debug_msg("[#1] starting\n");
	while (1) {
		debug_msg("[#1] running\n");
		kernel_task_sleep(500);
	}
	debug_msg("[#1] terminating\n");
}
ALLOCATE_TASK(task1, 512, 1, &task1_func)

void task2_func(void* arg)
{
	while (1) {
		if (kernel_get_task_status(&task1) == TASK_STATE_DEAD) {
			debug_msg("[#2] resuming task 1\n");
			kernel_activate_task_immediately(&task1);
		}
		kernel_task_sleep(2000);
	}
}
ALLOCATE_TASK(task2, 512, 2, &task2_func)

void task3_func(void* arg)
{
	while (1) {
		if (kernel_get_task_status(&task1) != TASK_STATE_DEAD) {
			debug_msg("[#3] killing task 1\n");
			kernel_task_kill(&task1);
		}
		kernel_task_sleep(5000);
	}
}
ALLOCATE_TASK(task3, 512, 3, &task3_func)

/*
 * Initialization functions
 */
MODULE_INIT_FUNCTION(test_function1)
{
	kernel_init_task(&task1);
	kernel_activate_task_immediately(&task1);
}

MODULE_INIT_FUNCTION(test_function2)
{
	kernel_init_task(&task2);
	kernel_activate_task_immediately(&task2);
}

MODULE_INIT_FUNCTION(test_function3)
{
	kernel_init_task(&task3);
	kernel_activate_task_immediately(&task3);
}
