#include <unistd.h>
#include <stdio.h>
#include "sys/task.h"

void loop_task (void *arg)
{
	const char *str = arg;
	int delay = (rand () % 5 + 1) * 3;

	while (1) {
		task_printf ("%s, delay: %d\n", str, delay);
		task_yield ();
		task_sleep (delay);
	}
}

void io_task (void *arg)
{
	(void)arg;
	char buf[32];
	int n;

	while (1) {
		n = task_read (STDIN_FILENO, buf, sizeof (buf) - 1);

		if (n == 0)
			exit (0);

		if (buf[n - 1] == '\n')
			--n;
		buf[n] = '\0';
		task_printf ("task_read(): %s\n", buf);
	}
}

void other_task (void *arg)
{
	char *name;
	int i;

	puts ("Other task");

	for (i = 0; i < 3; ++i) {
		asprintf (&name, "TASK %c", 'A' + i);
		task_spawn (name, loop_task, name);
	}

	task_spawn ("io", io_task, NULL);

	task_exit (42);
}

void main_task (void *arg)
{
	int tid, wid;
	puts ("Hello World");
	tid = task_spawn ("other", other_task, NULL);
	task_printf ("tid = %d\n", tid);

	task_printf ("waiting for task...\n");
	tid = task_wait (&wid);
	task_printf ("tid = %d, wid = %d\n", tid, wid);

	task_exit (0);
}

int main (void)
{
	task_start (main_task, NULL, 1000);
}
