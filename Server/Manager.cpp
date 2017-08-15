#include "logging.h"
#include "Manager.h"
#include "RequestHandler.h"

#include <signal.h>
#include "ServerConfig.h"

Manager* g_instance = nullptr;

Manager::Manager(unsigned threads /* = std::thread::hardware_concurrency() */)
: m_fcgi(new FcgiPoll<Request>)
, m_stop(true)
, m_workThreads(threads)
{
	g_instance = this;
}

Manager::~Manager()
{
}

bool Manager::Listen()
{
	return m_fcgi->Listen();
}

void Manager::Start()
{
	m_stop = false;
	m_fcgi->Start();
	if (!m_recvThread.joinable())
	{
		std::thread thread(&Manager::Receiver, this);
		m_recvThread.swap(thread);
	}
	for (auto& thread : m_workThreads)
	{
		if (!thread.joinable())
		{
			std::thread newThread(&Manager::Handler, this);
			thread.swap(newThread);
		}
	}
}

void Manager::Join()
{
	for (auto& thread : m_workThreads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}
	m_recvThread.join();
}

void Manager::Stop()
{
	m_stop = true;
	m_fcgi->Stop();
	m_semaphore.post(m_workThreads.size());
}

void Manager::SetupSignals()
{
	struct sigaction sigAction;
	sigAction.sa_handler = Manager::SignalHandler;
	sigemptyset(&sigAction.sa_mask);
	sigAction.sa_flags = 0;

	sigaction(SIGPIPE, &sigAction, NULL);
	sigaction(SIGUSR1, &sigAction, NULL);
	sigaction(SIGTERM, &sigAction, NULL);
}

void Manager::Receiver()
{
	while (!m_fcgi->IsStop())
	{
		auto request = m_fcgi->GetRequest();
		if (request)
		{
			std::lock_guard<std::mutex> lock(m_requestMutex);
			m_requestList.push_back(request);
			m_semaphore.post();
		}
	}
}

void Manager::Handler()
{
	std::unique_ptr<RequestHandler> requestHandler(new RequestHandler);
	while (!m_stop)
	{
		m_semaphore.wait();
		std::shared_ptr<Request> request(nullptr);
		{
			std::unique_lock<std::mutex> lock(m_requestMutex);
			if (!m_stop && !m_requestList.empty())
			{
				request = m_requestList.front();
				m_requestList.pop_front();
			}
		}
		if (request)
		{
			requestHandler->Handle(request);
		}
	}
}

void Manager::SignalHandler(int signum)
{
	switch (signum)
	{
	case SIGUSR1:
		// reopen log
		common::SetLogFile(ServerConfig::logPath.c_str());
		break;
	case SIGTERM:
		if (g_instance)
		{
			g_instance->Stop();
		}
		break;
	default:
		break;
	}
}