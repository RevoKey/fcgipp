#pragma once

#include "libCache.h"
#include "RedisHelp.h"
#include "Request.h"

#include <memory>

typedef std::shared_ptr<Request> RequestPtr;

class BaseHandler
{
public:
	BaseHandler();
	virtual ~BaseHandler();

	void ResponseJson(RequestPtr request, Json::Value& retJson);
	void ResponseError(RequestPtr request, int errcode);

protected:
	CDataCenter* m_dc;
	redisContext* m_redisContext;
	RedisHelp m_RedisHelp;
};

