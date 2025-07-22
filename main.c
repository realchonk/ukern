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
	(void)arg;
	puts ("HELLO WORLD");
	task_exit (42);
}

void main_task (void *arg)
{
	(void)arg;
	int i, tid, wst;

	task_spawn ("other", other_task, NULL);
	task_spawn ("Task A", loop_task, "A");
	task_spawn ("Task B", loop_task, "B");
	task_spawn ("Task C", loop_task, "C");
	task_spawn ("Task D", loop_task, "D");
	task_spawn ("Task E", loop_task, "E");
	tid = task_spawn ("Task D", loop_task, "F");

	task_yield ();
	printf ("killing %d...\n", tid);
	task_kill (tid);

	for (i = 0; i < 2; ++i) {
		puts ("waiting...");
		tid = task_wait (&wst);
		printf ("%d exited with %d.\n", tid, wst);
	}

	puts ("exiting...");

	task_exit (0);
}

int main (void)
{
	task_start (main_task, NULL, 100);
	return 0;
}
