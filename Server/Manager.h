#pragma once

#include "FcgiPoll.h"
#include "Request.h"
#include "Semaphore.h"

#include <list>
#include <mutex>
#include <thread>
#include <vector>

class Manager
{
public:
	Manager(unsigned threads = std::thread::hardware_concurrency());
	~Manager();

	bool Listen();
	void Start();
	void Join();
	void Stop();
	void SetupSignals();

	static void SignalHandler(int signum);

private:
	void Receiver();
	void Handler();

	std::shared_ptr<FcgiPoll<Request>> m_fcgi;
	std::thread m_recvThread;
	std::vector<std::thread> m_workThreads;
	std::mutex m_requestMutex;
	std::list<std::shared_ptr<Request>> m_requestList;
	Semaphore m_semaphore;
	bool m_stop;
};

