#include <stdlib.h>
#include <string.h>
#include "task.h"

#define MAX_TASKS 128

static TAILQ_HEAD(tq_fl, task) free_list = TAILQ_HEAD_INITIALIZER (free_list);
static struct task *table[128];
static int rotor = -1;

static void task_do_free (struct task *);

static int
find_task_id (void)
{
	struct task *task;
	int end;

	for (end = rotor++; rotor != end; rotor = (rotor + 1) % MAX_TASKS) {
		task = table[rotor];
		if (task == NULL)
			return rotor;
	}
	return -1;
}

struct task *
task_get (int tid)
{
	return tid >= 0 && tid < MAX_TASKS ? table[tid] : NULL;
}

struct task *
task_alloc (void)
{
	struct task *task;
	int tid;
	
	assert_blocked ();

	tid = find_task_id ();
	if (tid < 0)
		return NULL;

	task = TAILQ_FIRST (&free_list);
	if (task != NULL) {
		TAILQ_REMOVE (&free_list, task, queue);

		task_do_free (task);
	} else {
		task = new (struct task);
	}

	table[tid] = task;

	memset (task, 0, sizeof (*task));
	task->tid = tid;
	return task;
}

void
task_free (struct task *self)
{
	assert_blocked ();

	self->state = TFREE;
	sys_assert (self == table[self->tid]);
	table[self->tid] = NULL;

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
