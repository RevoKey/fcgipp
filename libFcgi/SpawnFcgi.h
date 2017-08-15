#pragma once

int SpawnFcgi(const char *addr, unsigned short port, const char *unixsocket, int forkCount, bool noFork);