#pragma once
#include <sys/socket.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <poll.h>

#define new(T) ((T *)calloc (1, sizeof (T)))
#define panic(msg) (sys_panic (__func__, (msg)))
#define sys_assert(cond) ((cond) ? 0 : (panic ("assertion failed: " #cond), 0))
#define assert_blocked() (sys_assert (sys_is_blocked ()))

struct cpu_context;
struct wchan;

enum task_state {
	TFREE,		/* in the free list */
	TIDL,		/* task being created */
	TRUN,		/* currently runnable */
	TWAIT,		/* waiting for something */
	TZOMBIE,	/* waiting to be collected */
};

struct task {
	TAILQ_ENTRY(task) queue;
	enum task_state state;

	/* list of children */
	LIST_ENTRY(task) sibling;
	LIST_HEAD(, task) children;

	/* metadata */
	int tid;			/* task id */
	int ptid;			/* parent task id */
	char *name;			/* task name (optional) */

	/* context */
	struct cpu_context *ctx;
	void *stack;
	size_t stack_size;

	union {
		struct wchan *wchan;	/* TWAIT */
		int code;		/* TZOMBIE */
	} data;
};

enum wchan_type {
	WCHAN_WAIT,			/* task_wait() */
	WCHAN_SLEEP,			/* task_sleep() */
	WCHAN_WAITIO,			/* task_read(), task_write() */
};

/* wait channel */
struct wchan {
	enum wchan_type type;

	union {
		struct task *task;	/* WAIT */
		struct timespec until;	/* SLEEP */
		struct pollfd io;	/* WAITIO */
	} data;
};


/* SIGNAL HANDLING */
void sys_block (void);
void sys_unblock (void);
bool sys_is_blocked (void);

/* STACK ALLOCATION */
void *stack_alloc (size_t);
void stack_free (void *, size_t);

/* LOW-LEVEL CONTEXT */
struct cpu_context *ctx_new (void *, size_t, void(*)(void *), void *);
void ctx_enter (struct cpu_context *ctx);
void ctx_switch (struct cpu_context **old, struct cpu_context *ctx);

/* ALLOCATION */
struct task *task_get (int tid);
struct task *task_alloc (void);
void task_free (struct task *);

/* SCHEDULER */
void task_yield (void);
void task_enter (void);
void task_switch (struct task *);
void task_enqueue (struct task *);
void task_unqueue (struct task *);
struct task *task_unlink_self (void);

void task_exit (int);
int task_wait (int *);
void task_do_wait (struct task *, struct wchan *);

void task_sleep (unsigned int);

/* IO */
FILE *task_fdopen (int fd);
int task_read (int fd, void *buf, size_t nbytes);
int task_write (int fd, const void *buf, size_t nbytes);
int task_printf (const char *, ...);
int task_dprintf (int fd, const char *, ...);
int task_vdprintf (int, const char *, va_list);
int task_connect (int fd, struct sockaddr *name, socklen_t namelen);
int task_accept (int fd, struct sockaddr *name, socklen_t *namelen);

/* ERRORS */
void task_err (int ec, const char *fmt, ...);
void task_errx (int ec, const char *fmt, ...);
void task_warn (const char *fmt, ...);
void task_warnx (const char *fmt, ...);

/* MISC */
const char *task_name (void);
int task_id (void);
int task_parent_id (void);
void sys_panic (const char *filename, const char *msg);
const char *xitoa (char *buf, int x, int base);

struct task *task_current (void);
void task_start (void(*entry)(void *), void *arg, int freq);
int task_spawn (const char *name, void(*entry)(void *), void *arg);
