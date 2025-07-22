#include <unistd.h>
#include <stdio.h>
#include "ukern.h"

void loop_task (void *arg)
{
	const char *name = task_name ();
	int delay;

	(void)arg;

	block ();
	delay = rand () % 15 + 1;
	unblock ();

	while (1) {
		printf ("%s (tid=%d, delay=%d)\n", name, task_id (), delay);

		task_sleep (delay);
	}
	task_exit (0);
}

void random_killer_task (void *arg)
{
	int tid;
	(void)arg;
	while (1) {
		task_sleep (1);

		block ();
		tid = rand () % MAX_TASKS;
		unblock ();

		printf ("killing %d...\n", tid);
		task_kill (tid);
	}
}

void other_task (void *arg)
{
	(void)arg;
	puts ("HELLO WORLD");
	task_spawn ("killer", random_killer_task, NULL);
	task_exit (42);
}

void main_task (void *arg)
{
	(void)arg;
	char *name;
	int i, tid, wst;


	task_spawn ("other", other_task, NULL);
	for (i = 0; i < 26; ++i) {
		asprintf (&name, "Task %c", 'A' + i);
		tid = task_spawn (name, loop_task, NULL);
		free (name);
	}

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
	srand (0);
	task_start (main_task, NULL, 1000);
	return 0;
}
