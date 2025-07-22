#include <sys/time.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <err.h>
#include "ukern.h"

static struct task *free_list = NULL;
static struct task *table[MAX_TASKS];

/* scheduling list */
static struct task *head = NULL, *current = NULL;

/* tasks that have exited, but not yet waited */
static struct task *zombies = NULL;

static sigset_t default_sigset;

void block (void)
{
	sigset_t blocked;
	sigfillset (&blocked);
	sigprocmask (SIG_SETMASK, &blocked, NULL);
}

void unblock (void)
{
	sigprocmask (SIG_SETMASK, &default_sigset, NULL);
}

static bool
is_blocked (void)
{
	sigset_t set;
	sigprocmask (SIG_SETMASK, NULL, &set);
	return sigismember (&set, SIGALRM);
}

static const char *
xitoa (char *buf, int x, int base)
{
	static const char alph[] = "0123456789ABCDEF";
	bool neg = false;
	size_t i = 0, j;
	char tmp;

	//assert (base >= 2 && base <= 16);

	if (x == 0)
		return "0";

	if (x < 0) {
		neg = true;
		x = -x;
	}

	while (x > 0) {
		buf[i++] = alph[x % base];
		x /= base;
	}
	
	if (neg)
		buf[i++] = '-';

	buf[i] = '\0';

	for (j = 0, --i; i > j; ++j, --i) {
		tmp = buf[j];
		buf[j] = buf[i];
		buf[i] = tmp;
	}

	return buf;
}

static void
xpanic (const char *fn, const char *msg)
{
	char buffer[128], ibuf[32];

	block ();
	
	/* cannot use *printf() because they are not signal-safe */
	if (current != NULL) {
		strlcpy (buffer, "TASK ", sizeof (buffer));
		strlcat (buffer, xitoa (ibuf, current->tid, 10), sizeof (buffer));
	} else {
		strlcpy (buffer, "NOTASK", sizeof (buffer));
	}
	strlcat (buffer, ": ", sizeof (buffer));
	strlcat (buffer, fn, sizeof (buffer));
	strlcat (buffer, "(): ", sizeof (buffer));
	strlcat (buffer, msg, sizeof (buffer));
	strlcat (buffer, "\n", sizeof (buffer));
	write (STDERR_FILENO, buffer, strlen (buffer));
	abort ();
}

#define panic(msg) (xpanic (__func__, (msg)))
#define assert_blocked() (is_blocked () ? 0 : (panic ("signals must be blocked")))
#define assert(cond) ((cond) ? 0 : (panic ("assertion failed: " #cond), 0))

static void
task_do_free (struct task *task)
{
	assert_blocked ();

	free (task->name);
	task->name = NULL;

	if (task->stack != NULL) {
		free_stack (task->stack, task->stack_size);
		task->stack = NULL;
		task->stack_size = 0;
	}
}

static void
task_free (struct task *task)
{
	assert_blocked ();

	task->state = TDEAD;

	if (task->tid != -1)
		table[task->tid] = NULL;
	task->next = free_list;
	free_list = task;
}

static int
find_free_id (void)
{
	int i;

	assert_blocked ();

	for (i = 1; i < MAX_TASKS; ++i) {
		if (table[i] == NULL)
			return i;
	}
	return -1;
}

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

	if (free_list != NULL) {
		task = free_list;
		free_list = free_list->next;
		task_do_free (task);
	} else {
		task = new (struct task);
	}

	task->state = TRUN;
	task->tid = find_free_id ();
	if (task->tid == -1) {
		task_free (task);
		return NULL;
	}
	table[task->tid] = task;
	
	task->ptid = parent;
	task->next = NULL;
	task->prev = NULL;
	task->name = name != NULL ? strdup (name) : NULL;
	task->stack = alloc_stack (stack_size);
	if (task->stack == NULL) {
		task_free (task);
		return NULL;
	}

	task->stack_size = stack_size;
	task->ctx = ctx_new (task->stack, stack_size, entry, arg);
	return task;
}

const char *
task_name (void)
{
	assert (current != NULL);
	return current->name;
}

int
task_id (void)
{
	assert (current != NULL);
	return current->tid;
}

int
task_parent_id (void)
{
	assert (current != NULL);
	return current->ptid;
}

static struct task *
unlink_self (void)
{
	struct task *old;
	
	assert_blocked ();

	/* unlink the current task from the scheduler */
	if (current->prev != NULL) {
		current->prev->next = current->next;
	} else {
		head = current->next;
	}
	if (current->next != NULL)
		current->next->prev = current->prev;
	old = current;
	current = current->next;
	if (current == NULL)
		current = head;
	old->next = old->prev = NULL;

	//assert (head != NULL && current != NULL);
	return old;
}

static void
link_task (struct task *task)
{
	assert_blocked ();

	task->next = head;
	task->prev = NULL;

	if (head != NULL) {
		assert (head->prev == NULL);
		head->prev = task;
	}
	head = task;
}

static void
task_orphan (struct task *task)
{
	task->ptid = 0;

	if (task->state == TDEAD)
		task_free (task);
}

static void
task_wakeup (int tid)
{
	struct task *task;
	assert_blocked ();
	assert (tid <= MAX_TASKS);

	task = table[tid];
	assert (task != NULL);

	if (task->state == TWAIT) {
		task->state = TRUN;
		link_task (task);
	}
}

static void
task_die (struct task *task, int ec)
{
	assert_blocked ();

	switch (task->state) {
	case TRUN:
		if (task->next != NULL)
			task->next->prev = task->prev;
		if (task->prev != NULL)
			task->prev->next = task->next;
		if (head == task)
			head = task->next;
		if (current == task)
			current = head;
		break;
	case TWAIT:
		break;
	case TDEAD:
		break;
	}

	if (task->ptid == 0) {
		task_free (task);
		return;
	}

	task->state = TDEAD;
	task->data = ec;
	task->next = zombies;
	task->prev = NULL;
	if (task->next != NULL)
		task->next->prev = task;
	zombies = task;
	task_wakeup (task->ptid);
}

void
task_exit (int ec)
{
	assert (current != NULL);
	struct task *task;
	size_t i;

	block ();

	/* orphan all children */
	for (i = 0; i < MAX_TASKS; ++i) {
		task = table[i];
		if (task != NULL && task->ptid == current->tid)
			task_orphan (task);
	}

	task = unlink_self ();
	task->state = TDEAD;
	task_die (task, ec);

	if (current == NULL) {
		current = head;
		if (current == NULL) {
			puts ("no other tasks to run, exiting...");
			exit (0);
		}
	}

	ctx_enter (current->ctx);
}

int
task_kill (int tid)
{
	struct task *task;

	block ();

	assert (tid <= MAX_TASKS);
	task = table[tid];
	if (task == NULL)
		return -1;

	task_die (task, 169);

	unblock ();
	return 0;
}

static void
task_enter_sleep (void)
{
	struct task *task;

	assert_blocked ();

	task = unlink_self ();
	task->state = TWAIT;

	ctx_switch (&task->ctx, current->ctx);
}

int
task_wait (int *wid)
{
	struct task *task;
	int tid;

	while (1) {
		block ();

		/* try finding a dead child */
		for (task = zombies; task != NULL; task = task->next) {
			if (task->ptid == current->tid) {
				tid = task->tid;

				if (task->prev != NULL)
					task->prev->next = task->next;
				if (task->next != NULL)
					task->next->prev = task->prev;
				if (task == zombies)
					zombies = task->next;
				task->next = task->prev = NULL;

				if (wid != NULL)
					*wid = task->data;

				task_free (task);
				return tid;
			}
		}

		task_enter_sleep ();
	}


	unblock ();
	return -1;
}

int
task_spawn (const char *name, void(*entry)(void *), void *arg)
{
	struct task *task;

	block ();

	/* cannot run outside the runtime */
	assert (current != NULL);

	task = task_new (name, current->tid, 4096, entry, arg);
	if (task == NULL)
		return -1;

	link_task (task);

	unblock ();
	return task->tid;
}

void
task_yield (void)
{
	struct task *old;

	block ();

	assert (head != NULL && current != NULL);
	old = current;
	current = current->next;
	if (current == NULL)
		current = head;

	if (old == current)
		return;

	assert (current->state == TRUN);

	ctx_switch (&old->ctx, current->ctx);
}

static void
task_sleep_until (struct timespec *until)
{
	struct timespec now;
	while (1) {
		clock_gettime (CLOCK_REALTIME, &now);

		if (timespeccmp (&now, until, >=))
			break;

		task_yield ();
	}
}

void
task_sleep (int sec)
{
	struct timespec ts;

	clock_gettime (CLOCK_REALTIME, &ts);
	ts.tv_sec += sec;
	task_sleep_until (&ts);
}

static void preempt (int sig)
{
	assert (sig == SIGALRM);
	task_yield ();
}

void
task_start (
	void(*entry)(void *),
	void *arg,
	int freq
) {
	struct itimerval timer;
	sigset_t sigblock;

	/* make sure task_start() is called on a dead system */
	assert (head == NULL);
	assert (current == NULL);

	/* block all signals and save the default sigmask */
	sigfillset (&sigblock);
	sigprocmask (SIG_SETMASK, &sigblock, &default_sigset);

	/* intialize process table and spawn first task */
	memset (table, 0, sizeof (table));
	head = current = task_new ("main", 0, 4096, entry, arg);

	/* set up preemption */
	signal (SIGALRM, preempt);
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 1000000 / freq;
	timer.it_interval = timer.it_value;
	setitimer (ITIMER_REAL, &timer, NULL);

	/* enter "userspace" */
	ctx_enter (head->ctx);
	while (1);
}

