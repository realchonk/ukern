#include <stdio.h>
#include "ukern.h"

void other_task (void *arg)
{
	int *ptr = arg;

	printf ("%d: *ptr = %d\n", task_id (), *ptr);

	task_yield ();
	task_yield ();
	task_yield ();
	task_yield ();
	task_yield ();
	task_yield ();

	printf ("%d: exiting...\n", task_id ());
	task_exit ();
}

void main_task (void *arg)
{
	(void)arg;
	int *x;

	printf ("%d: Hello World!\n", task_id ());

	x = new (int);
	*x = 42;
	task_spawn ("other", other_task, x);
	task_yield ();
	task_yield ();
	
	printf ("%d: exiting...\n", task_id ());
	task_exit ();
}

int main (void)
{
	task_start (main_task, NULL);
	return 0;
}
