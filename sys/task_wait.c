#include <stdlib.h>
#include "task.h"

static void
task_die (struct task *self, int ec)
{
	struct task *parent, *child, *tmp;
	struct wchan *wchan;

	assert_blocked ();

	/* orphan children */
	LIST_FOREACH_SAFE (child, &self->children, sibling, tmp) {
		child->ptid = -1;
		LIST_REMOVE (child, sibling);
		if (child->state == TZOMBIE)
			task_free (child);
	}

	if (self->ptid == -1) {
		task_free (self);
		return;
	}

	self->state = TZOMBIE;
	self->data.code = ec;

	parent = task_get (self->ptid);

	if (parent == NULL || parent->state != TWAIT)
		return;

	wchan = parent->data.wchan;
	if (wchan->type != WCHAN_WAIT)
		return;

	wchan->data.task = self;
	task_enqueue (parent);
}

void
task_exit (int ec)
{
	struct task *self;

	sys_block ();

	self = task_unlink_self ();
	task_die (self, ec);

	task_enter ();
}

void
task_do_wait (struct task *self, struct wchan *wchan)
{
	if (self->state == TRUN)
		task_unqueue (self);

	self->state = TWAIT;
	self->data.wchan = wchan;
	
	task_switch (self);
}

int
task_wait (int *wid)
{
	struct task *self, *child, *tmp;
	struct wchan *wchan;
	int tid;

	sys_block ();
	self = task_current ();

	if (LIST_EMPTY (&self->children))
		return -1;

	/* try finding an already terminated child */
	LIST_FOREACH_SAFE (child, &self->children, sibling, tmp) {
		sys_assert (child->ptid == self->tid);
		if (child->state != TZOMBIE)
			continue;

		tid = child->tid;
		if (wid != NULL)
			*wid = child->data.code;

		LIST_REMOVE (child, sibling);
		task_free (child);

		sys_unblock ();
		return tid;
	}

	/* otherwise wait until one dies */
	wchan = new (struct wchan);
	wchan->type = WCHAN_WAIT;
	task_do_wait (self, wchan);

	sys_block ();
	child = wchan->data.task;
	sys_assert (child != NULL);

	tid = child->tid;
	if (wid != NULL)
		*wid = child->data.code;

	LIST_REMOVE (child, sibling);
	task_free (child);
	return tid;
}
