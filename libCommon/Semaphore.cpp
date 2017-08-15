#include "Semaphore.h"
#include <stdio.h>
#include <time.h>

Semaphore::Semaphore(int semaphore, const char*name)
{
	sem_ok_ = true;
	int ret = sem_init(&sem_, 0, 0);
	if (ret) 
	{
		sem_ok_ = false;
	}
}
Semaphore::~Semaphore()
{
	sem_destroy(&sem_);
}

int Semaphore::post(int sem_num)
{
	for (int i = 0; i < sem_num; ++i) 
	{
		sem_post(&sem_);
	}
	return sem_num;
}

int Semaphore::wait(int millisecond)
{
	if (millisecond != -1)
	{
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		long secs = millisecond / 1000;
		long msecs = millisecond % 1000;

		long add = 0;
		msecs = msecs * 1000 * 1000 + ts.tv_nsec;
		add = msecs / (1000 * 1000 * 1000);
		ts.tv_sec += (add + secs);
		ts.tv_nsec = msecs % (1000 * 1000 * 1000);
		int ret = sem_timedwait(&sem_, &ts);
		if (ret != 0)
		{
			if (errno == ETIMEDOUT)
			{
				return SEMAPHORE_TIME_OUT;
			}
			return SEMAPHORE_UNKNOWN_FAILD;
		}
		return SEMAPHORE_SUCCESS;
	}
	else
	{
		int ret = sem_wait(&sem_);
		return ret == 0 ? SEMAPHORE_SUCCESS : SEMAPHORE_UNKNOWN_FAILD;
	}

}
