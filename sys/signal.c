#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "task.h"

sigset_t default_sigset;

void sys_block (void)
{
	sigset_t blocked;
	sigfillset (&blocked);
	sigprocmask (SIG_SETMASK, &blocked, NULL);
}

void sys_unblock (void)
{
	sigprocmask (SIG_SETMASK, &default_sigset, NULL);
}

bool
sys_is_blocked (void)
{
	sigset_t set;
	sigprocmask (SIG_SETMASK, NULL, &set);
	return sigismember (&set, SIGALRM);
}

void
catch_crash (int sig)
{
	/* TODO: detect if the crash happened in kernel space and abort accordingly */
	const char *name;
	char msg[128], buf[32];

	strlcpy (msg, "ERROR: Task ", sizeof (msg));
	xitoa (buf, task_id (), 10);
	if ((name = task_name ()) != NULL) {
		strlcat (msg, name, sizeof (msg));
		strlcat (msg, " (", sizeof (msg));
		strlcat (msg, buf, sizeof (msg));
		strlcat (msg, ")", sizeof (msg));
	} else {
		strlcat (msg, buf, sizeof (msg));
	}
	strlcat (msg, " crashed by signal ", sizeof (msg));
	xitoa (buf, sig, 10);
	strlcat (msg, buf, sizeof (msg));
	strlcat (msg, ": ", sizeof (msg));
	strlcat (msg, strsignal (sig), sizeof (msg));
	strlcat (msg, "\n", sizeof (msg));
	write (STDERR_FILENO, msg, strlen (msg));
	task_exit (169);
}

