#include "BaseHandler.h"
#include "Helper.h"
#include "fcgiapp.h"
#include "HttpSender.h"
#include "logging.h"
#include "md5.h"
#include "ServerConfig.h"

#include <string>

BaseHandler::BaseHandler()
{
	m_redisContext = m_RedisHelp.RedisConnect(ServerConfig::redisHost.c_str(), 6379);
	if (!(m_redisContext && m_RedisHelp.Auth(m_redisContext, ServerConfig::redisAuth)))
	{
		Log(ERROR, "redis init failed");
		exit(EXIT_FAILURE);
	}
	m_dc = new CDataCenter;
	if (!(m_dc && m_dc->Init(ServerConfig::dbHost, ServerConfig::dbAccount, ServerConfig::dbPwd, "santong")))
	{
		Log(ERROR, "DataCenter init failed");
		exit(EXIT_FAILURE);
	}
}

BaseHandler::~BaseHandler()
{
	m_RedisHelp.RedisFree(m_redisContext);
	m_redisContext = nullptr;
	if (m_dc)
	{
		delete m_dc;
		m_dc = nullptr;
	}
}

void BaseHandler::ResponseJson(RequestPtr request, Json::Value& retJson)
{
	retJson["time"] = time(NULL);
	Json::FastWriter fw;
	fw.omitEndingLineFeed();
	std::string sRet = fw.write(retJson);

	auto ret = FCGX_FPrintF(request->out,
		"Content-type: application/json;charset=UTF-8\r\n"
		"\r\n"
		"%s", sRet.c_str());
}

void BaseHandler::ResponseError(RequestPtr request, int errcode)
{
	Json::Value retJson;
	retJson["time"] = time(NULL);
	retJson["errcode"] = errcode;
	Json::FastWriter fw;
	fw.omitEndingLineFeed();
	std::string sRet = fw.write(retJson);

	auto ret = FCGX_FPrintF(request->out,
		"Content-type: application/json;charset=UTF-8\r\n"
		"\r\n"
		"%s", sRet.c_str());
}