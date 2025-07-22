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
	task_exit ();
}

void main_task (void *arg)
{
	(void)arg;

	task_spawn ("Task A", loop_task, "A");
	task_spawn ("Task B", loop_task, "B");
	task_spawn ("Task C", loop_task, "C");
	task_spawn ("Task D", loop_task, "D");
	task_spawn ("Task E", loop_task, "E");
	task_spawn ("Task F", loop_task, "F");

	task_exit ();
}

int main (void)
{
	task_start (main_task, NULL, 1000);
	return 0;
}
