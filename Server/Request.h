#pragma once

#include "CommonDef.h"
#include "fcgiapp.h"
#include "HttpEnv.h"

class Request : public FCGX_Request
{
public:
	Request(FCGX_Request* fcgi);
	~Request();

	void ParseHeader();
	void ParseBody();

	HttpEnv m_env;

private:
	bool m_headerParsed;
	bool m_bodyParsed;
};