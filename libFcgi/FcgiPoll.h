#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <set>

#include "fcgiapp.h"

class FcgiPollBase
{
public:
	FcgiPollBase();
	~FcgiPollBase();

	bool Listen();
	void Start();
	void Wake();
	void Stop();
	bool IsStop(){ return m_stop; }

protected:
	FCGX_Request* PollRequest();
	bool PollAdd(const int socket);
	bool PollDel(const int socket);
	bool CreateSocket(const int listener);

	std::set<int> m_listeners;
	std::set<int> m_sockets;
	std::atomic_bool m_refreshListeners;
	std::atomic_bool m_accept;
	std::atomic_bool m_stop;
	std::mutex m_wakingMutex;
	int m_poll;
	int m_wakeSockets[2];
	bool m_waking;
	bool m_block;
};

template<class RequestT>
class FcgiPoll : public FcgiPollBase, public std::enable_shared_from_this<FcgiPoll<RequestT>>
{
public:
	std::shared_ptr<RequestT> GetRequest()
	{
		RequestT* request = new RequestT(PollRequest());
		std::shared_ptr<FcgiPoll> self(this->shared_from_this());
		return std::shared_ptr<RequestT>(request, Deleter(self));
	}
protected:
	friend class Deleter;
	class Deleter
	{
	public:
		Deleter(std::shared_ptr<FcgiPoll> pool) : m_pool(pool)
		{
		}
		void operator()(RequestT* request)
		{
			m_pool->FinishRequest(request);
		}
	private:
		std::shared_ptr<FcgiPoll> m_pool;
	};

	void FinishRequest(RequestT* request)
	{
		if (request)
		{
			if (!request->keepConnection)
			{
				PollDel(request->ipcFd);
			}
			FCGX_Finish_r(request);
			delete request;
		}
	}
};
