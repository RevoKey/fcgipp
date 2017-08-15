#pragma once

#include "jsoncpp/json/json.h"

#include <string>

class ServerConfig
{
public:
	static bool Load(const std::string& fileName);
	static inline bool GetStr(const Json::Value& config, const std::string& name, std::string& value);
	static inline bool GetInt(const Json::Value& config, const std::string& name, int& value);

	static std::string logPath;
	static std::string logLevel;
	static std::string redisAuth;
	static std::string redisHost;
	static std::string dbAccount;
	static std::string dbPwd;
	static std::string dbHost;
	static int threadCount;
};

