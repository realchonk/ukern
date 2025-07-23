#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "task.h"

const char *
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


void
sys_panic (const char *fn, const char *msg)
{
	char buffer[128], ibuf[32];
	struct task *task;

	sys_block ();
	
	/* cannot use *printf() because they are not signal-safe */
	task = task_current ();
	if (task != NULL) {
		strlcpy (buffer, "TASK ", sizeof (buffer));
		strlcat (buffer, xitoa (ibuf, task->tid, 10), sizeof (buffer));
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
