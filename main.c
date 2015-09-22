#include "minitask.h"
#include <stdio.h>

struct args {
	int start;
	int delay;
};

void task(void *args) {
	int i = ((struct args*)args)->start;
	int delay_ms = ((struct args*)args)->delay;
	printf("initialising task %d\n", minitask_active_task_id());
	while (i > 0) {
		printf("task %d: %d\n", minitask_active_task_id(), i);
		i--;
		minitask_sleep_ms(delay_ms);
	}
	printf("task %d - done!\n", minitask_active_task_id());
}

int main(int argc, char *argv[]) {
	struct args a1 = { 20, 100 };
	struct args a2 = { 10, 50 };
	struct args a3 = { 8, 500 };
	printf("a1: %p\n", &a1);
	printf("a2: %p\n", &a2);
	printf("a3: %p\n", &a3);
	minitask_init();
	minitask_spawn(&task, &a1);
	minitask_spawn(&task, &a2);
	minitask_spawn(&task, &a3);
	minitask_start();
	return 0;
}