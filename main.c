#include <unistd.h>
#include <stdio.h>
#include "ukern.h"

void loop_task (void *arg)
{
	while (1) {
		block ();
		printf ("%s: %d %d\n", (const char *)arg, task_id (), task_parent_id ());
		unblock ();
		sleep (1);
	}
	task_exit (0);
}

void other_task (void *arg)
{
	puts ("HELLO WORLD");
	task_exit (42);
}

void main_task (void *arg)
{
	(void)arg;
	int tid, wst;

	task_spawn ("other", other_task, NULL);
	task_spawn ("Task A", loop_task, "A");
	task_spawn ("Task B", loop_task, "B");
	task_spawn ("Task C", loop_task, "C");

	puts ("waiting...");
	tid = task_wait (&wst);
	printf ("%d exited with %d.\n", tid, wst);

	task_exit (0);
}

int main (void)
{
	task_start (main_task, NULL, 100);
	return 0;
}
