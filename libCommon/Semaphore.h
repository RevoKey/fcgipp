#pragma once

#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>

#ifndef WAIT_OBJECT_0
#define WAIT_OBJECT_0  0
#endif

#ifndef WAIT_TIMEOUT
#define WAIT_TIMEOUT 0x00000102
#endif

enum 
{
	SEMAPHORE_SUCCESS = WAIT_OBJECT_0,
	SEMAPHORE_TIME_OUT = WAIT_TIMEOUT,
	SEMAPHORE_UNKNOWN_FAILD = -1
};

class Semaphore
{
public:
	Semaphore(int semaphore = 5000, const char*name = NULL);
	~Semaphore();

	int post(int sem_num = 1);
	int wait(int millisecond = -1);
private:
	sem_t  sem_;
	bool sem_ok_;
};


