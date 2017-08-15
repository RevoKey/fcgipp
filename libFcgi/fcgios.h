#pragma once

#include <sys/time.h>
#include <sys/types.h>

#define OS_Errno errno
#define OS_SetErrno(err) errno = (err)

/* This is the initializer for a "struct timeval" used in a select() call
* right after a new request is accept()ed to determine readablity.  Its
* a drop-dead timer.  Its only used for AF_UNIX sockets (not TCP sockets).
* Its a workaround for a kernel bug in Linux 2.0.x and SCO Unixware.
* Making this as small as possible, yet remain reliable would be best.
* 2 seconds is very conservative.  0,0 is not reliable.  The shorter the
* timeout, the faster request processing will recover.  The longer the
* timeout, the more likely this application being "busy" will cause other
* requests to abort and cause more dead sockets that need this timeout. */
#define READABLE_UNIX_FD_DROP_DEAD_TIMEVAL 2,0

#ifndef STDIN_FILENO
#define STDIN_FILENO  0
#endif

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#ifndef X_OK
#define X_OK       0x01
#endif

typedef void *ClientData;

typedef void(*OS_AsyncProc) (ClientData clientData, int len);

int OS_LibInit(int stdioFds[3]);
void OS_LibShutdown(void);
int OS_CreateLocalIpcFd(const char *bindPath, int backlog);
int OS_FcgiConnect(char *bindPath);
int OS_Read(int fd, char * buf, size_t len);
int OS_Write(int fd, char * buf, size_t len);
int OS_SpawnChild(char *execPath, int listenFd);
int OS_AsyncReadStdin(void *buf, int len, OS_AsyncProc procPtr, ClientData clientData);
int OS_AsyncRead(int fd, int offset, void *buf, int len, OS_AsyncProc procPtr, ClientData clientData);
int OS_AsyncWrite(int fd, int offset, void *buf, int len, OS_AsyncProc procPtr, ClientData clientData);
int OS_Close(int fd);
int OS_CloseRead(int fd);
int OS_DoIo(struct timeval *tmo);
int OS_Accept(int listen_sock, int fail_on_intr, const char *webServerAddrs);
int OS_IpcClose(int ipcFd);
int OS_IsFcgi(int sock);
void OS_SetFlags(int fd, int flags);

void OS_ShutdownPending(void);