#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "task.h"

static TAILQ_HEAD(, task) pending_io = TAILQ_HEAD_INITIALIZER(pending_io);
static size_t num_fds = 0;

void
sys_do_io (void)
{
	static struct pollfd *fds = NULL;
	static size_t old_num_fds = 0;

	struct task *task, *tmp;
	struct wchan *wchan;
	size_t i;
	int r;

	if (num_fds == 0)
		return;

	sys_block ();

	if (num_fds > old_num_fds) {
		fds = reallocarray (fds, num_fds, sizeof (struct pollfd));
		old_num_fds = num_fds;
	}

	i = 0;
	TAILQ_FOREACH (task, &pending_io, queue) {
		sys_assert (task->state == TWAIT);
		wchan = task->data.wchan;
		sys_assert (wchan->type == WCHAN_WAITIO);
		wchan->data.io.revents = 0;
		fds[i++] = wchan->data.io;
	}

	sys_assert (i == num_fds);

	r = poll (fds, num_fds, 0);
	if (r == 0) {
		sys_unblock ();
		return;
	}

	i = 0;
	TAILQ_FOREACH_SAFE (task, &pending_io, queue, tmp) {
		r = fds[i++].revents;
		if (r == 0)
			continue;

		wchan = task->data.wchan;
		wchan->data.io.revents = r;
		TAILQ_REMOVE (&pending_io, task, queue);
		--num_fds;
		task_enqueue (task);
	}

	sys_unblock ();
}

static int
task_wait_io (int fd, int events)
{
	struct task *self;
	struct wchan *wchan;
	int ret;

	wchan = new (struct wchan);
	wchan->type = WCHAN_WAITIO;
	wchan->data.io.fd = fd;
	wchan->data.io.events = events;
	wchan->data.io.revents = 0;

	sys_block ();
	self = task_unlink_self ();
	TAILQ_INSERT_TAIL (&pending_io, self, queue);
	++num_fds;
	task_do_wait (self, wchan);
	ret = wchan->data.io.revents;
	free (wchan);
	return ret;
}

int
task_read (int fd, void *buf, size_t nbytes)
{
	int revents, ret;

	revents = task_wait_io (fd, POLLIN);
	if (revents & (POLLIN | POLLHUP)) {
		sys_block ();
		ret = read (fd, buf, nbytes);
		sys_unblock ();
	} else {
		ret = -1;
	}

	return ret;
}

int
task_write (int fd, const void *buf, size_t nbytes)
{
	int revents, ret;

	revents = task_wait_io (fd, POLLOUT);
	if (revents & POLLOUT) {
		sys_block ();
		ret = write (fd, buf, nbytes);
		sys_unblock ();
	} else {
		ret = -1;
	}

	return ret;
}

#if defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__)
static int
impl_read (void *cookie, char *buf, int n)
{
	return task_read ((int)(size_t)cookie, buf, n);
}

static int
impl_write (void *cookie, const char *buf, int n)
{
	return task_write ((int)(size_t)cookie, buf, n);
}

static off_t
impl_seek (void *cookie, off_t off, int dir)
{
	return lseek ((int)(size_t)cookie, off, dir);
}

static FILE *
wrap_fd (int fd)
{
	return funopen (
		(void *)(size_t)fd,
		impl_read,
		impl_write,
		impl_seek,
		NULL
	);
}
#elif defined(__linux__)
static ssize_t
impl_read (void *cookie, char *buf, size_t n)
{
	return task_read ((int)(size_t)cookie, buf, n);
}

static ssize_t
impl_write (void *cookie, const char *buf, size_t n)
{
	return task_write ((int)(size_t)cookie, buf, n);
}

static int
impl_seek (void *cookie, off_t *off, int dir)
{
       off_t pos = lseek ((int)(size_t)cookie, *off, dir);
       if (pos < 0)
               return -1;
       *off = pos;
       return 0;
}

static FILE *
wrap_fd (int fd)
{
	cookie_io_functions_t funcs = {
		.read = impl_read,
		.write = impl_write,
		.seek = impl_seek,
		.close = NULL,
	};
	return fopencookie ((void *)(size_t)fd, "w+", funcs);
}
#else
# error Your platform does not support creating generic FILE objects
#endif

FILE *
task_fdopen (int fd)
{
	return wrap_fd (fd);
}

int
task_vdprintf (int fd, const char *fmt, va_list ap)
{
	FILE *wrapped;
	int ret;

	wrapped = wrap_fd (fd);
	ret = vfprintf (wrapped, fmt, ap);
	fclose (wrapped);
	return ret;
}

int
task_dprintf (int fd, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start (ap, fmt);
	ret = task_vdprintf (fd, fmt, ap);
	va_end (ap);

	return ret;
}

int
task_printf (const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start (ap, fmt);
	ret = task_vdprintf (STDOUT_FILENO, fmt, ap);
	va_end (ap);

	return ret;
}

int
task_connect (int fd, struct sockaddr *name, socklen_t namelen)
{
	int revents, ret, error, flags;
	socklen_t len = sizeof (error);

	flags = fcntl (fd, F_GETFL);
	if (fcntl (fd, F_SETFL, flags | O_NONBLOCK) == -1)
		return -1;

	while (1) {
		ret = connect (fd, name, namelen);
		if (ret == 0 || errno != EINTR)
			break;

		revents = task_wait_io (fd, POLLOUT);
		task_printf ("revents = %d\n", revents);
		if (getsockopt (fd, SOL_SOCKET, SO_ERROR, &error, &len) == -1)
			return -1;
		if (error != 0) {
			errno = error;
			return -1;
		}
		return 0;
	}

	error = errno;
	fcntl (fd, F_SETFL, flags);
	errno = error;
	return ret;
}

int
task_accept (int fd, struct sockaddr *addr, socklen_t *addrlen)
{
	int ret, flags, error;

	flags = fcntl (fd, F_GETFL);
	if (fcntl (fd, F_SETFL, flags | O_NONBLOCK) == -1)
		return -1;

	ret = task_wait_io (fd, POLLIN);
	if (!(ret & POLLIN)) {
		ret = -1;
		goto done;
	}

	ret = accept (fd, addr, addrlen);

done:
	error = errno;
	fcntl (fd, F_SETFL, flags);
	errno = error;
	return ret;
}
