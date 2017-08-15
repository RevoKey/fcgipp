#include "Helper.h"
#include "ServerConfig.h"

#include <fstream>
#include <thread>

#define STR(x) #x

std::string ServerConfig::logPath = "Server.log";
std::string ServerConfig::logLevel = "INFO";
int ServerConfig::threadCount = std::thread::hardware_concurrency();

std::string ServerConfig::redisAuth = "";
std::string ServerConfig::redisHost = "";
std::string ServerConfig::dbAccount = "";
std::string ServerConfig::dbPwd = "";
std::string ServerConfig::dbHost = "";

bool ServerConfig::GetStr(const Json::Value& config, const std::string& name, std::string& value)
{
	if (config.isMember(name) && config[name].isString())
	{
		value = config[name].asString();
		return true;
	}
	return false;
}

bool ServerConfig::GetInt(const Json::Value& config, const std::string& name, int& value)
{
	if (config.isMember(name) && config[name].isInt())
	{
		value = config[name].asInt();
		return true;
	}
	return false;
}

bool ServerConfig::Load(const std::string& fileName)
{
	std::string filePath = Helper::GetCurrentDir() + "/" + fileName;
	std::ifstream in(filePath.c_str());
	std::string config((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	Json::Value root;
	Json::Reader reader;
	if (reader.parse(config.c_str(), root) && root.isObject())
	{
		GetInt(root, STR(threadCount), threadCount);
		GetStr(root, STR(logPath), logPath);
		GetStr(root, STR(logLevel), logLevel);
		GetStr(root, STR(dbAccount), dbAccount);
		GetStr(root, STR(dbHost), dbHost);
		GetStr(root, STR(dbPwd), dbPwd);
		GetStr(root, STR(redisAuth), redisAuth);
		GetStr(root, STR(redisHost), redisHost);
		return true;
	}
	else
	{
		return false;
	}
}