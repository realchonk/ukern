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
