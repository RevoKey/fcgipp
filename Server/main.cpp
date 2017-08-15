#include "logging.h"
#include "Manager.h"
#include "ServerConfig.h"
#include "SpawnFcgi.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/un.h>

int main(int argc, char *argv[])
{
	char *unixsocket = nullptr;
	char *addr = nullptr;
	char *endptr = nullptr;
	unsigned short port = 0;
	int fork_count = 1;
	int o;
	bool nofork = false;
	
	while (-1 != (o = getopt(argc, argv, "a:p:F:s:n")))
	{
		switch (o) 
		{
		case 'a':
			addr = optarg;
			break;
		case 'p':
			port = strtol(optarg, &endptr, 10);
			if (*endptr) 
			{
				fprintf(stderr, "spawn-fcgi: invalid port: %u/n", (unsigned int)port);
				return -1;
			}
			break;
		case 'F':
			fork_count = strtol(optarg, NULL, 10);
			break;
		case 's':
			unixsocket = optarg;
			break;
		case 'n':
			nofork = true;
			break;
		default:
			fprintf(stderr, "spawn-fcgi: invalid opt: %s/n", o);
			return -1;
		}
	}

	if (0 == port && nullptr == unixsocket)
	{
		fprintf(stderr, "spawn-fcgi: no socket given (use either -p or -s)\n");
		return -1;
	}
	else if (0 != port && nullptr != unixsocket)
	{
		fprintf(stderr, "spawn-fcgi: either a Unix domain socket or a TCP-port, but not both\n");
		return -1;
	}

	if (unixsocket && strlen(unixsocket) > sizeof(sockaddr_un::sun_path) - 1)
	{
		fprintf(stderr, "spawn-fcgi: path of the Unix domain socket is too long\n");
		return -1;
	}

	if (0 == SpawnFcgi(addr, port, unixsocket, fork_count, nofork))
	{
		ServerConfig::Load("server.cnf");
		common::SetLogLevel(ServerConfig::logLevel);
		if (!nofork)
		{
			common::SetLogFile(ServerConfig::logPath.c_str());
		}
		else
		{
			common::SetLogLevel("DEBUG");
		}
		common::StartLog();
		Manager manager;
		manager.SetupSignals();
		if (!manager.Listen())
		{
			return 0;
		}
		Log(INFO, "start");
		manager.Start();
		manager.Join();
	}
	return 0;
}