#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include "sys/task.h"

void client_task (void *arg)
{
	int clientfd = (int)(size_t)arg;
	char buf[64];
	int i, n, x;

	task_printf ("accepted client: %d\n", clientfd);
	while (1) {
		n = task_read (clientfd, buf, sizeof (buf));
		if (n == 0)
			break;
		if (n < 0)
			task_err (1, "read()");

		
		for (i = 0; i < n; i += x) {
			x = task_write (clientfd, buf + i, n - i);
			if (x <= 0)
				task_err (1, "write()");
		}
	}
	close (clientfd);
	task_exit (0);
}

void server_task (void *arg)
{
	struct sockaddr_in addr;
	int sockfd, clientfd, optval = 1;

	sockfd = socket (AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		task_err (1, "socket()");

	setsockopt (sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof (optval));

	addr.sin_len = sizeof (addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons (8000);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind (sockfd, (struct sockaddr *)&addr, sizeof (addr)) != 0)
		task_err (1, "bind()");

	if (listen (sockfd, 0) != 0)
		task_err (1, "listen()");

	while (1) {
		clientfd = task_accept (sockfd, NULL, NULL);
		if (clientfd < 0)
			task_err (1, "accept()");
		task_spawn ("client", client_task, (void *)(size_t)clientfd);
	}
}

void loop_task (void *arg)
{
	const char *str = arg;
	int delay = (rand () % 5 + 1) * 3;

	while (1) {
		task_printf ("%s, delay: %d\n", str, delay);
		task_yield ();
		task_sleep (delay);
	}
}

void io_task (void *arg)
{
	(void)arg;
	char buf[32];
	int n;

	while (1) {
		n = task_read (STDIN_FILENO, buf, sizeof (buf) - 1);

		if (n == 0)
			exit (0);

		if (buf[n - 1] == '\n')
			--n;
		buf[n] = '\0';
		task_printf ("task_read(): %s\n", buf);
	}
}

void other_task (void *arg)
{
	char *name;
	int i;

	puts ("Other task");

	for (i = 0; i < 3; ++i) {
		asprintf (&name, "TASK %c", 'A' + i);
		task_spawn (name, loop_task, name);
	}

	task_spawn ("io", io_task, NULL);
	task_spawn ("server", server_task, NULL);

	task_exit (42);
}

void main_task (void *arg)
{
	int tid, wid;
	puts ("Hello World");
	tid = task_spawn ("other", other_task, NULL);
	task_printf ("tid = %d\n", tid);

	task_printf ("waiting for task...\n");
	tid = task_wait (&wid);
	task_printf ("tid = %d, wid = %d\n", tid, wid);

	task_exit (0);
}

int main (void)
{
	task_start (main_task, NULL, 1000);
}
