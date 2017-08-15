#include "logging.h"
#include "md5.h"
#include "Request.h"

#include <algorithm>

Request::Request(FCGX_Request* fcgi)
: m_headerParsed(false)
, m_bodyParsed(false)
{
	if (fcgi)
	{
		requestId = fcgi->requestId;
		role = fcgi->role;
		in = fcgi->in;
		out = fcgi->out;
		err = fcgi->err;
		envp = fcgi->envp;

		paramsPtr = fcgi->paramsPtr;
		ipcFd = fcgi->ipcFd;
		isBeginProcessed = fcgi->isBeginProcessed;
		keepConnection = fcgi->keepConnection;
		appStatus = fcgi->appStatus;
		nWriters = fcgi->nWriters;
		flags = fcgi->flags;
		listen_sock = fcgi->listen_sock;
	}
}

Request::~Request()
{
}

void Request::ParseHeader()
{
	if (!m_headerParsed)
	{
		m_env.ParseHeader(envp);
		m_headerParsed = true;
	}
}

void Request::ParseBody()
{
	if (!m_bodyParsed)
	{
		m_env.ParsePostBuffer(in);
		m_bodyParsed = true;
	}
}