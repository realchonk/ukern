#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include "task.h"

extern sigset_t default_sigset;
extern void sig_preempt (int);
extern void sched_task (void *);

static struct task *
task_new (
	const char *name,
	int parent,
	size_t stack_size,
	void(*entry)(void *),
	void *arg
) {
	struct task *task;

	assert_blocked ();

	task = task_alloc ();
	if (task == NULL)
		return NULL;

	task->state = TIDL;
	task->ptid = parent;
	task->name = name != NULL ? strdup (name) : NULL;
	task->stack_size = stack_size;
	task->stack = stack_alloc (stack_size);
	if (task->stack == NULL)
		goto fail;

	task->ctx = ctx_new (task->stack, stack_size, entry, arg);
	if (task->ctx == NULL)
		goto fail;

	LIST_INIT (&task->children);

	return task;

fail:
	task_free (task);
	return NULL;
}

int
task_spawn (const char *name, void(*entry)(void *), void *arg)
{
	struct task *self, *child;
	int tid;

	sys_block ();

	/* cannot run outside the runtime */
	self = task_current ();
	sys_assert (self != NULL);

	child = task_new (name, self->tid, 4096, entry, arg);
	if (child == NULL)
		return -1;

	LIST_INSERT_HEAD (&self->children, child, sibling);

	task_enqueue (child);
	tid = child->tid;

	sys_unblock ();
	return tid;
}

void
task_start (
	void(*entry)(void *),
	void *arg,
	int freq
) {
	struct itimerval timer;
	sigset_t sigblock;
	struct task *task;

	/* make sure task_start() is called on a dead system */
	//assert (head == NULL);
	//assert (current == NULL);

	/* block all signals and save the default sigmask */
	sigfillset (&sigblock);
	sigprocmask (SIG_SETMASK, &sigblock, &default_sigset);

	/* create the "idle" task */
	task = task_new ("sched", -1, 4096, sched_task, NULL);
	task->state = TRUN;
	sys_assert (task->tid == 0);

	/* intialize process table and spawn first task */
	task = task_new ("main", 0, 4096, entry, arg);
	task_enqueue (task);
	sys_assert (task->tid == 1);

	/* set up preemption */
	signal (SIGALRM, sig_preempt);
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 1000000 / freq;
	timer.it_interval = timer.it_value;
	setitimer (ITIMER_REAL, &timer, NULL);

	/* enter "userspace" */
	task_enter ();
	while (1);
}

