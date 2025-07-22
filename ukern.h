#pragma once
#include <stdlib.h>

#define new(T) ((T *)calloc (1, sizeof (T)))
#define MAX_TASKS 128

struct cpu_context;

enum task_state {
	TRUN,				/* currently runnable */
	TWAIT,				/* waiting for something */
	TDEAD,				/* almost dead */
};

struct task {
	struct task *next, *prev;
	struct cpu_context *ctx;
	enum task_state state;
	int tid;			/* task id */
	int ptid;			/* parent task id */
	int data;

	char *name;			/* task name (optional) */
	void *stack;
	size_t stack_size;
};

/* TASK */

/* start the multitasking system */
void task_start (void(*entry)(void *), void *, int freq);

/* get the current tasks' name */
const char *task_name (void);

/* get the current task id */
int task_id (void);

/* get the parent task id */
int task_parent_id (void);

/* exit the current task */
void task_exit (int);

/* spawn a new task */
int task_spawn (const char *name, void(*entry)(void *), void *arg);

/* yield to another task */
void task_yield (void);

/* wait for a child task to exit */
int task_wait (int *wid);

/* kill a task */
int task_kill (int tid);

void task_sleep (int sec);

/* MISC */

/* block signals */
void block (void);
/* unblock signals */
void unblock (void);

void *alloc_stack (size_t size);
void free_stack (void *ptr, size_t size);

/* CONTEXT */

/* initialize a new CPU context on the stack */
struct cpu_context *ctx_new (
	void *stack,
	size_t stack_size,
	void(*entry)(void *),
	void *arg
);

/* enter new context without saving the current one */
void ctx_enter (struct cpu_context *ctx);

/* save old context and enter new context */
void ctx_switch (struct cpu_context **old, struct cpu_context *ctx);

