#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "task.h"

const char *task_name (void)
{
	return task_current ()->name;
}
int task_id (void)
{
	return task_current ()->tid;
}
int task_parent_id (void)
{
	return task_current ()->ptid;
}

void task_err (int ec, const char *fmt, ...)
{
	va_list ap;
	int e = errno;

	va_start (ap, fmt);
	task_dprintf (STDERR_FILENO, "%s(%d): error: ", task_name (), task_id ());
	task_vdprintf (STDERR_FILENO, fmt, ap);
	task_dprintf (STDERR_FILENO, ": %s\n", strerror (e));
	va_end (ap);
	task_exit (ec);
}

void task_errx (int ec, const char *fmt, ...)
{
	va_list ap;

	va_start (ap, fmt);
	task_dprintf (STDERR_FILENO, "%s(%d): error: ", task_name (), task_id ());
	task_vdprintf (STDERR_FILENO, fmt, ap);
	task_dprintf (STDERR_FILENO, "\n");
	va_end (ap);
	task_exit (ec);
}
void task_warn (const char *fmt, ...)
{
	va_list ap;
	int e = errno;

	va_start (ap, fmt);
	task_dprintf (STDERR_FILENO, "%s(%d): warn: ", task_name (), task_id ());
	task_vdprintf (STDERR_FILENO, fmt, ap);
	task_dprintf (STDERR_FILENO, ": %s\n", strerror (e));
	va_end (ap);
}

void task_warnx (const char *fmt, ...)
{
	va_list ap;

	va_start (ap, fmt);
	task_dprintf (STDERR_FILENO, "%s(%d): warn: ", task_name (), task_id ());
	task_vdprintf (STDERR_FILENO, fmt, ap);
	task_dprintf (STDERR_FILENO, "\n");
	va_end (ap);
}
