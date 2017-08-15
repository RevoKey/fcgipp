#pragma once

#include "CommonDef.h"
#include "MySQLBase.h"

class CDataCenter : public MySQLBase
{
public:
	CDataCenter();
	~CDataCenter();

	bool GetUserById(int uid); // just an example

	bool DoUpdate(const std::string& sql, int* result = nullptr);

private:
	template<typename ... Args>
	void string_format(std::string& sql, const std::string& format, Args ... args);
	std::string MakeGoodSqlString(const std::string &strIn);
};