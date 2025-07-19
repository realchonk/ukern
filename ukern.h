#pragma once
#include <stdlib.h>

#define new(T) ((T *)calloc (1, sizeof (T)))
#define MAX_TASKS 16

struct cpu_context;

struct task {
	struct task *next, *prev;
	struct cpu_context *ctx;
	int id;				/* task id */

	char *name;			/* task name (optional) */
	void *stack;
	size_t stack_size;
};

/* TASK */

/* start the multitasking system */
void task_start (void(*entry)(void *), void *);

/* get the current task id */
int task_id (void);

/* exit the current task */
void task_exit (void);

/* spawn a new task */
int task_spawn (const char *name, void(*entry)(void *), void *arg);

/* yield to another task */
void task_yield (void);

/* MISC */
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

