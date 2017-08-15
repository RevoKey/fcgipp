#pragma once

#include "BaseHandler.h"

class RequestHandler : public BaseHandler
{
public:
	RequestHandler();
	virtual ~RequestHandler();

	void Handle(RequestPtr request);
};