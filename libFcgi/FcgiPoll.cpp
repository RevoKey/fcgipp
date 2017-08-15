#include "FcgiPoll.h"
#include "fastcgi.h"
#include "fcgimisc.h"
#include "fcgios.h"

#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

FcgiPollBase::FcgiPollBase()
{
	m_poll = epoll_create1(0);
	m_refreshListeners = false;
	m_accept = false;
	m_stop = true;
	m_waking = false;
	m_block = false;

	socketpair(AF_UNIX, SOCK_STREAM, 0, m_wakeSockets);
	PollAdd(m_wakeSockets[1]);
}

FcgiPollBase::~FcgiPollBase()
{
	close(m_poll);
	close(m_wakeSockets[0]);
	close(m_wakeSockets[1]);
	for (const auto& listener : m_listeners)
	{
		::shutdown(listener, SHUT_RDWR);
		::close(listener);
	}
}

bool FcgiPollBase::Listen()
{
	const int listen = 0;

	fcntl(listen, F_SETFL, fcntl(listen, F_GETFL) | O_NONBLOCK);

	if (m_listeners.find(listen) == m_listeners.end())
	{
		if (::listen(listen, 100) < 0)
		{
			return false;
		}
		m_listeners.insert(listen);
		m_refreshListeners = true;
		return true;
	}
	else
	{
		return false;
	}
}

void FcgiPollBase::Start()
{
	m_stop = false;
	m_accept = true;
}

void FcgiPollBase::Stop()
{
	m_stop = true;
	if (m_accept)
	{
		m_accept = false;
		m_refreshListeners = true;
		Wake();
	}
}

void FcgiPollBase::Wake()
{
	std::lock_guard<std::mutex> lock(m_wakingMutex);
	if (!m_waking)
	{
		m_waking = true;
		char x = 0;
		write(m_wakeSockets[0], &x, 1);
	}
}

bool FcgiPollBase::PollAdd(const int socket)
{
	epoll_event event;
	event.data.fd = socket;
	event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
	return epoll_ctl(m_poll, EPOLL_CTL_ADD, socket, &event) != -1;
}

bool FcgiPollBase::PollDel(const int socket)
{
	return epoll_ctl(m_poll, EPOLL_CTL_DEL, socket, nullptr) != -1;
}

bool FcgiPollBase::CreateSocket(const int listener)
{
	sockaddr_un addr;
	socklen_t addrlen = sizeof(sockaddr_un);
	const int socket = ::accept(listener, reinterpret_cast<sockaddr*>(&addr), &addrlen);
	if (fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK) < 0)
	{
		close(socket);
		return false;
	}

	if (m_accept)
	{
		m_sockets.insert(socket);
		return PollAdd(socket);
	}
	else
	{
		close(socket);
	}
	return false;
}

FCGX_Request* FcgiPollBase::PollRequest()
{
	FCGX_Request* request = new FCGX_Request;
	FCGX_InitRequest(request, 0, 0);
	while (!m_stop)
	{
		int pollResult;
		epoll_event epollEvent;
		const auto& pollIn = EPOLLIN;
		const auto& pollErr = EPOLLERR;
		const auto& pollHup = EPOLLHUP;
		const auto& pollRdHup = EPOLLRDHUP;

		while (m_listeners.size() + m_sockets.size() > 0)
		{
			if (m_refreshListeners)
			{
				for (auto& listener : m_listeners)
				{
					PollDel(listener);
					if (m_accept && !PollAdd(listener))
					{
						continue;
					}
				}
				m_refreshListeners = false;
			}

			pollResult = epoll_wait(m_poll, &epollEvent, 1, m_block ? -1 : 0);
			const auto& socketId = epollEvent.data.fd;
			const auto& events = epollEvent.events;

			if (pollResult<0)
			{
				if (errno == EINTR)
				{
					continue;
				}
			}
			else if (pollResult>0)
			{
				if (m_listeners.find(socketId) != m_listeners.end())
				{
					if (events == pollIn)
					{
						CreateSocket(socketId);
						continue;
					}
				}
				else if (socketId == m_wakeSockets[1])
				{
					if (events == pollIn)
					{
						std::lock_guard<std::mutex> lock(m_wakingMutex);
						char x[256];
						read(m_wakeSockets[1], x, 256);
						m_waking = false;
						m_block = false;
						continue;
					}
				}
				else
				{
					const auto socket = m_sockets.find(socketId);
					if (socket == m_sockets.end())
					{
						PollDel(socketId);
						close(socketId);
						continue;
					}
					request->ipcFd = socketId;
				}
			}
			break;
		}

		if (request->ipcFd >= 0)
		{
			if (ReadRequest(request))
			{
				break;
			}
			else
			{
				PollDel(request->ipcFd);
				FCGX_Free(request, 1);
			}
		}

		m_block = true;
	}

	if (m_stop)
	{
		return nullptr;
	}
	else
	{
		SetRequestWriter(request);
		return request;
	}
}