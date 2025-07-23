#include <sys/time.h>
#include "task.h"

static TAILQ_HEAD(, task) sleeping = TAILQ_HEAD_INITIALIZER(sleeping);

void
sys_wakeup_sleeping (void)
{
	struct task *task, *tmp;
	struct wchan *wchan;
	struct timespec now;

	sys_block ();
	clock_gettime (CLOCK_REALTIME, &now);
	TAILQ_FOREACH_SAFE (task, &sleeping, queue, tmp) {
		sys_assert (task->state == TWAIT);
		wchan = task->data.wchan;
		sys_assert (wchan->type == WCHAN_SLEEP);

		if (timespeccmp (&now, &wchan->data.until, >=)) {
			TAILQ_REMOVE (&sleeping, task, queue);
			task_enqueue (task);
		}
	}
	sys_unblock ();
}

static void
task_sleep_until (struct timespec *until)
{
	struct task *self;
	struct wchan *wchan;

	wchan = new (struct wchan);
	wchan->type = WCHAN_SLEEP;
	wchan->data.until = *until;

	sys_block ();
	self = task_unlink_self ();
	// TODO: implement a priority queue, sorted by who wakes up first
	TAILQ_INSERT_TAIL (&sleeping, self, queue);
	task_do_wait (self, wchan);
	free (wchan);
}

void
task_sleep (unsigned int seconds)
{
	struct timespec ts;
	clock_gettime (CLOCK_REALTIME, &ts);
	ts.tv_sec += seconds;
	task_sleep_until (&ts);
}
