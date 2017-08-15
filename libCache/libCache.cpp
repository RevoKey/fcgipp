#include "libCache.h"
#include "Helper.h"
#include "logging.h"
#include "SimpleProfiler.h"

#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <memory>
#include <algorithm>

using namespace Helper;
using namespace std;

#define  PAGE_SIZE 20

CDataCenter::CDataCenter()
{
}

CDataCenter::~CDataCenter()
{
}

bool CDataCenter::GetUserById(int uid)
{
	try
	{
		static __thread std::shared_ptr<sql::PreparedStatement> stmt(PrepareStmt(
			"select * from user where id=?"));
		stmt->setInt(1, uid);
		shared_ptr<sql::ResultSet> pRes(stmt->executeQuery());
		while (pRes->next())
		{
			pRes->getInt("id");
		}
	}
	catch (sql::SQLException &e)
	{
		HandleException(e);
	}
	return false;
}

bool CDataCenter::DoUpdate(const std::string& sql, int* result /* = nullptr */)
{
	bool ret = false;
	try
	{
		int n = ExecuteUpdate(sql);
		if (result)
		{
			*result = n;
		}
		ret = true;
	}
	catch (sql::SQLException &e)
	{
		LogSqlException(e, "DoUpdate()", sql);
		HandleException(e);
	}
	return ret;
}

template<typename ... Args>
void CDataCenter::string_format(string& sql, const std::string& format, Args ... args)
{
	size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
	unique_ptr<char[]> buf(new char[size]);
	snprintf(buf.get(), size, format.c_str(), args ...);
	sql = string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}
std::string CDataCenter::MakeGoodSqlString(const std::string &strIn)
{
	auto out = strIn;
	Helper::ReplaceStringInPlace(out, "\\", "\\\\");
	Helper::ReplaceStringInPlace(out, "\'", "\\\'");
	Helper::ReplaceStringInPlace(out, "\"", "\\\"");
	return out;
}