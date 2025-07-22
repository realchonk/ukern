#include <stdlib.h>
#include <signal.h>
#include "task.h"

static TAILQ_HEAD(sq_tq, task) sched_queue = TAILQ_HEAD_INITIALIZER (sched_queue);

/*
 * task to run, when there are no other tasks to run
 * This task has the following restrictions:
 * - cannot task_exit()
 * */
void
sched_task (void *arg)
{
	/* TODO: do something smart */
	(void)arg;
	exit (0);
	//while (1)
	//	task_yield ();
}

struct task *
task_current (void)
{
	struct task *task;

	task = TAILQ_FIRST (&sched_queue);
	if (task != NULL)
		return task;

	task = task_get (0);
	sys_assert (task != NULL);
	return task;
}

void
task_enqueue (struct task *task)
{
	assert_blocked ();

	sys_assert (task->state != TRUN);
	task->state = TRUN;
	TAILQ_INSERT_TAIL (&sched_queue, task, queue);
}

void
task_unqueue (struct task *task)
{
	assert_blocked ();

	sys_assert (task->state == TRUN);
	TAILQ_REMOVE (&sched_queue, task, queue);
	task->state = TIDL;
}

struct task *
task_unlink_self (void)
{
	struct task *self;

	assert_blocked ();
	self = task_current ();
	task_unqueue (self);

	return self;
}

void
task_enter (void)
{
	struct task *self;
	self = task_current ();
	sys_assert (self != NULL);
	sys_assert (self->state == TRUN);
	ctx_enter (self->ctx);
}

void
task_switch (struct task *old)
{
	struct task *self;
	self = task_current ();
	sys_assert (self != NULL);
	sys_assert (self->state == TRUN);
	ctx_switch (&old->ctx, self->ctx);
}

void
task_yield (void)
{
	struct task *self, *next;

	sys_block ();

	/* move self to the back of the queue */
	self = task_current ();
	if (self->tid != 0) {
		task_unqueue (self);
		task_enqueue (self);
	}

	/* skip the context switch, if we are the only runnable task */
	next = task_current ();
	if (self == next)
		return;

	task_switch (self);
}

void
sig_preempt (int sig)
{
	sys_assert (sig == SIGALRM);
	task_yield ();
}
