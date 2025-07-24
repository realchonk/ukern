#include <stdlib.h>
#include <string.h>
#include "task.h"

/* See: https://gitlab.freedesktop.org/libbsd/libbsd/-/issues?show=eyJpaWQiOiIzNCIsImZ1bGxfcGF0aCI6ImxpYmJzZC9saWJic2QiLCJpZCI6MTM1ODM0fQ%3D%3D */
#ifndef __unused
# define __unused __attribute__((unused))
#endif

static RB_HEAD(tasktree, task) tthead = RB_INITIALIZER (&task_tree);
static TAILQ_HEAD(tq_fl, task) free_list = TAILQ_HEAD_INITIALIZER (free_list);
static void task_do_free (struct task *);
static int cmp_task (struct task *t1, struct task *t2)
{
	if (t1->tid > t2->tid) {
		return 1;
	} else if (t1->tid < t2->tid) {
		return -1;
	} else {
		return 0;
	}
}

RB_PROTOTYPE_STATIC(tasktree, task, entry, cmp_task);
RB_GENERATE_STATIC(tasktree, task, entry, cmp_task);


static int
next_task_id (void)
{
	struct task *task;
	assert_blocked ();
	task = RB_MAX (tasktree, &tthead);
	return task != NULL ? task->tid + 1 : 0;
}

struct task *
task_get (int tid)
{
	struct task tmp;
	tmp.tid = tid;
	return RB_FIND (tasktree, &tthead, &tmp);
}

struct task *
task_alloc (void)
{
	struct task *task;
	
	assert_blocked ();

	task = TAILQ_FIRST (&free_list);
	if (task != NULL) {
		TAILQ_REMOVE (&free_list, task, queue);
		task_do_free (task);
	} else {
		task = new (struct task);
		task->tid = -1;
	}

	sys_assert (task->tid == -1);

	memset (task, 0, sizeof (*task));
	task->tid = next_task_id ();
	RB_INSERT (tasktree, &tthead, task);
	return task;
}

void
task_free (struct task *self)
{
	assert_blocked ();

	self->state = TFREE;
	RB_REMOVE (tasktree, &tthead, self);
	self->tid = -1;
	TAILQ_INSERT_TAIL (&free_list, self, queue);
}

static void
task_do_free (struct task *task)
{
	assert_blocked ();

	if (task == NULL)
		return;

	free (task->name);
	task->name = NULL;

	if (task->stack != NULL) {
		stack_free (task->stack, task->stack_size);
		task->stack = NULL;
		task->stack_size = 0;
	}
}
