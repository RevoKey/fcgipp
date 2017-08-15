#include "SpawnFcgi.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#define FCGI_LISTENSOCK_FILENO 0

namespace
{
	int bind_socket(const char *addr, unsigned short port, int backlog)
	{
		int fcgi_fd, socket_type, val;
		struct sockaddr_un fcgi_addr_un;
		struct sockaddr_in fcgi_addr_in;
		struct sockaddr *fcgi_addr;
		socklen_t servlen;

		memset(&fcgi_addr_in, 0, sizeof(fcgi_addr_in));
		fcgi_addr_in.sin_family = AF_INET;
		fcgi_addr_in.sin_port = htons(port);

		servlen = sizeof(fcgi_addr_in);
		socket_type = AF_INET;
		fcgi_addr = (struct sockaddr *) &fcgi_addr_in;

		if (addr == NULL) {
			fcgi_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
		}
		else if (1 != inet_pton(AF_INET, addr, &fcgi_addr_in.sin_addr)) {
			fprintf(stderr, "spawn-fcgi: '%s' is not a valid IP address\n", addr);
			return -1;
		}

		if (-1 == (fcgi_fd = socket(socket_type, SOCK_STREAM, 0))) {
			fprintf(stderr, "spawn-fcgi: couldn't create socket: %s\n", strerror(errno));
			return -1;
		}

		val = 1;
		if (setsockopt(fcgi_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
			fprintf(stderr, "spawn-fcgi: couldn't set SO_REUSEADDR: %s\n", strerror(errno));
			close(fcgi_fd);
			return -1;
		}

		if (-1 == bind(fcgi_fd, fcgi_addr, servlen)) {
			fprintf(stderr, "spawn-fcgi: bind failed: %s\n", strerror(errno));
			close(fcgi_fd);
			return -1;
		}

		if (-1 == listen(fcgi_fd, backlog)) {
			fprintf(stderr, "spawn-fcgi: listen failed: %s\n", strerror(errno));
			close(fcgi_fd);
			return -1;
		}

		return fcgi_fd;
	}

	int bind_unix_socket(const char *unixsocket, mode_t mode, int backlog)
	{
		int fcgi_fd, socket_type, val;
		struct sockaddr_un fcgi_addr_un;
		struct sockaddr_in fcgi_addr_in;
		struct sockaddr *fcgi_addr;
		socklen_t servlen;

		memset(&fcgi_addr_un, 0, sizeof(fcgi_addr_un));

		fcgi_addr_un.sun_family = AF_UNIX;
		if (strlen(unixsocket) > sizeof(fcgi_addr_un.sun_path) - 1) return -1;
		strcpy(fcgi_addr_un.sun_path, unixsocket);

		servlen = SUN_LEN(&fcgi_addr_un);
		socket_type = AF_UNIX;
		fcgi_addr = (struct sockaddr *) &fcgi_addr_un;

		if (-1 == (fcgi_fd = socket(socket_type, SOCK_STREAM, 0))) {
			fprintf(stderr, "spawn-fcgi: couldn't create socket: %s\n", strerror(errno));
			return -1;
		}

		if (0 == connect(fcgi_fd, fcgi_addr, servlen)) {
			fprintf(stderr, "spawn-fcgi: socket is already in use, can't spawn\n");
			close(fcgi_fd);
			return -1;
		}

		if (-1 == unlink(unixsocket)) {
			switch (errno) {
			case ENOENT:
				break;
			default:
				fprintf(stderr, "spawn-fcgi: removing old socket failed: %s\n", strerror(errno));
				close(fcgi_fd);
				return -1;
			}
		}

		close(fcgi_fd);

		if (-1 == (fcgi_fd = socket(socket_type, SOCK_STREAM, 0))) {
			fprintf(stderr, "spawn-fcgi: couldn't create socket: %s\n", strerror(errno));
			return -1;
		}

		val = 1;
		if (setsockopt(fcgi_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
			fprintf(stderr, "spawn-fcgi: couldn't set SO_REUSEADDR: %s\n", strerror(errno));
			close(fcgi_fd);
			return -1;
		}

		if (-1 == bind(fcgi_fd, fcgi_addr, servlen)) {
			fprintf(stderr, "spawn-fcgi: bind failed: %s\n", strerror(errno));
			close(fcgi_fd);
			return -1;
		}

		if (-1 == chmod(unixsocket, mode)) {
			fprintf(stderr, "spawn-fcgi: couldn't chmod socket: %s\n", strerror(errno));
			close(fcgi_fd);
			unlink(unixsocket);
			return -1;
		}

		if (-1 == listen(fcgi_fd, backlog)) {
			fprintf(stderr, "spawn-fcgi: listen failed: %s\n", strerror(errno));
			close(fcgi_fd);
			return -1;
		}

		return fcgi_fd;
	}

	int spawn(int fcgi_fd, int forkCount, bool noFork)
	{
		int status = 0;
		int rc = 0;
		struct timeval tv = { 0, 100 * 1000 };

		pid_t child;

		while (forkCount--)
		{
			if (!noFork)
			{
				child = fork();
			}
			else
			{
				child = 0;
			}

			switch (child)
			{
			case 0:
			{
					  char cgi_childs[64];
					  int max_fd = 0;
					  int i = 0;
					  if (fcgi_fd != FCGI_LISTENSOCK_FILENO)
					  {
						  close(FCGI_LISTENSOCK_FILENO);
						  dup2(fcgi_fd, FCGI_LISTENSOCK_FILENO);
						  close(fcgi_fd);
					  }

					  if (!noFork)
					  {
						  setsid();
						  max_fd = open("/dev/null", O_RDWR);
						  if (-1 != max_fd)
						  {
							  if (max_fd != STDOUT_FILENO) dup2(max_fd, STDOUT_FILENO);
							  if (max_fd != STDERR_FILENO) dup2(max_fd, STDERR_FILENO);
							  if (max_fd != STDOUT_FILENO && max_fd != STDERR_FILENO) close(max_fd);
						  }
						  else
						  {
							  fprintf(stderr, "spawn-fcgi: couldn't open and redirect stdout/stderr to '/dev/null': %s\n", strerror(errno));
						  }
					  }
			}
				return child;
			case -1:
				fprintf(stderr, "spawn-fcgi: fork failed: %s\n", strerror(errno));
				break;
			default:
				select(0, NULL, NULL, NULL, &tv);
				switch (waitpid(child, &status, WNOHANG))
				{
				case 0:
					fprintf(stdout, "spawn-fcgi: child spawned successfully: PID: %d\n", child);
					break;
				case -1:
					break;
				default:
					if (WIFEXITED(status))
					{
						fprintf(stderr, "spawn-fcgi: child exited with: %d\n", WEXITSTATUS(status));
						rc = WEXITSTATUS(status);
					}
					else if (WIFSIGNALED(status))
					{
						fprintf(stderr, "spawn-fcgi: child signaled: %d\n", WTERMSIG(status));
						rc = 1;
					}
					else
					{
						fprintf(stderr, "spawn-fcgi: child died somehow: exit status = %d\n", status);
						rc = status;
					}
					break;
				}
				break;
			}
		}
		close(fcgi_fd);
		return 1;
	}

	int SpawnFcgiAddr(const char *addr, unsigned short port, int forkCount, bool noFork)
	{
		int fcgi_fd = -1;

		if (-1 == (fcgi_fd = bind_socket(addr, port, 1024)))
		{
			return -1;
		}

		return spawn(fcgi_fd, forkCount, noFork);
	}

	int SpawnFcgiUnix(const char *unixsocket, int forkCount, bool noFork)
	{
		mode_t mask = umask(0);
		umask(mask);
		mode_t sockmode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) & ~mask;
		int fcgi_fd = -1;

		if (-1 == (fcgi_fd = bind_unix_socket(unixsocket, sockmode, 1024)))
		{
			return -1;
		}

		return spawn(fcgi_fd, forkCount, noFork);
	}
}

int SpawnFcgi(const char *addr, unsigned short port, const char *unixsocket, int forkCount, bool noFork)
{
	if (unixsocket)
	{
		return SpawnFcgiUnix(unixsocket, forkCount, noFork);
	}
	else
	{
		return SpawnFcgiAddr(addr, port, forkCount, noFork);
	}
}