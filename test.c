#include <stdio.h>
#include "task.h"

void loop_task (void *arg)
{
	const char *str = arg;
	int delay = rand () % 5 + 1;

	while (1) {
		printf ("%s, delay: %d\n", str, delay);
		task_yield ();
		task_sleep (delay);
	}
}

void other_task (void *arg)
{
	char *name;
	int i;

	puts ("Other task");

	for (i = 0; i < 10; ++i) {
		asprintf (&name, "TASK %c", 'A' + i);
		task_spawn (name, loop_task, name);
	}

	task_exit (42);
}

void main_task (void *arg)
{
	int tid, wid;
	puts ("Hello World");
	tid = task_spawn ("other", other_task, NULL);
	printf ("tid = %d\n", tid);

	printf ("waiting for task...\n");
	tid = task_wait (&wid);
	printf ("tid = %d, wid = %d\n", tid, wid);

	task_exit (0);
}

int main (void)
{
	task_start (main_task, NULL, 1000);
}
