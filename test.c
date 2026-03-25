#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
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
	(void)arg;

	sockfd = socket (AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		task_err (1, "socket()");

	setsockopt (sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof (optval));

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
	int delay = rand () % 10 + 1;

	while (1) {
		task_printf ("%s, delay: %d\n", str, delay);
		task_yield ();
		task_sleep (delay);
	}
}

void crashing_task (void *arg)
{
	int *ptr = arg;
	*ptr = 42;
	task_exit (0);
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

	(void)arg;

	puts ("Other task");

	for (i = 0; i < 3; ++i) {
		asprintf (&name, "TASK %c", 'A' + i);
		task_spawn (name, loop_task, name);
	}

	task_exit (42);
}

void ping_task (void *arg)
{
	struct sockaddr_in addr;
	int sock;

	memset (&addr, 0, sizeof (addr));
	addr.sin_family = AF_INET;
	addr.sin_port = 80;
	addr.sin_addr.s_addr = 0x01010101; // 1.1.1.1
	
	while (1) {
		sock = socket (AF_INET, SOCK_STREAM, 0);
		if (sock < 0)
			task_err (1, "socket()");

		task_printf ("connecting to 1.1.1.1\n");

		if (task_connect (sock, (struct sockaddr *)&addr, sizeof (addr)) != 0) {
			task_warn ("connect()");
			goto next;
		}

		task_printf ("connected to 1.1.1.1\n");

	next:
		close (sock);
		task_sleep (1);
	}
}

void main_task (void *arg)
{
	int i, tid, wid;

	(void)arg;

	puts ("Hello World");
	tid = task_spawn ("other", other_task, NULL);
	task_printf ("tid = %d\n", tid);

	task_spawn ("ping", ping_task, NULL);
	task_spawn ("io", io_task, NULL);
	task_spawn ("server", server_task, NULL);
	for (i = 0; i < 3; ++i) {
		task_sleep (1);
		task_spawn ("bad", crashing_task, NULL);
	}

	while (1) {
		task_printf ("main: waiting for task to finish...\n");
		tid = task_wait (&wid);
		if (tid == -1)
			break;
		task_printf ("main: task %d exited with %d\n", tid, wid);
	}
	task_printf ("main: no more tasks to wait for..., exiting\n");

	task_exit (0);
}

int main (void)
{
	task_start (main_task, NULL, 1000);
}
