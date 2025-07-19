#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ukern.h"

static struct task *free_list = NULL;
static struct task *table[MAX_TASKS];

/* scheduling list */
static struct task *head = NULL, *current = NULL;

static void
task_do_free (struct task *task)
{
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
	if (task->tid != -1)
		table[task->tid] = NULL;
	task->next = free_list;
	free_list = task;
}

static int
find_free_id (void)
{
	int i;

	for (i = 1; i < MAX_TASKS; ++i) {
		if (table[i] == NULL)
			return i;
	}
	return -1;
}

static struct task *
task_new (
	const char *name,
	size_t stack_size,
	void(*entry)(void *),
	void *arg
) {
	struct task *task;

	if (free_list != NULL) {
		task = free_list;
		free_list = free_list->next;
		task_do_free (task);
	} else {
		task = new (struct task);
	}

	task->tid = find_free_id ();
	if (task->tid == -1) {
		task_free (task);
		return NULL;
	}
	table[task->tid] = task;
	
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

void
task_start (void(*entry)(void *), void *arg)
{
	assert (head == NULL);
	assert (current == NULL);

	memset (table, 0, sizeof (table));

	head = current = task_new ("main", 4096, entry, arg);
	ctx_enter (head->ctx);
	while (1);
}

int
task_id (void)
{
	assert (current != NULL);
	return current->tid;
}

static struct task *
unlink_self (void)
{
	struct task *old;

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
	task->next = head;
	task->prev = NULL;

	if (head != NULL) {
		assert (head->prev == NULL);
		head->prev = task;
	}
	head = task;
}

void
task_exit (void)
{
	assert (current != NULL);

	unlink_self ();

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
task_spawn (const char *name, void(*entry)(void *), void *arg)
{
	struct task *task;

	/* cannot run outside the runtime */
	assert (head != NULL);

	task = task_new (name, 4096, entry, arg);
	if (task == NULL)
		return -1;

	link_task (task);

	return task->tid;
}

void
task_yield (void)
{
	struct task *old;

	assert (head != NULL && current != NULL);

	printf ("%d: yielding...\n", task_id ());

	old = current;
	current = current->next;
	if (current == NULL)
		current = head;

	if (old == current)
		return;

	ctx_switch (&old->ctx, current->ctx);
}

