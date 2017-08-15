#pragma once

#include "mysql_connection.h"
#include "cppconn/driver.h"
#include "cppconn/driver.h"
#include "cppconn/exception.h"
#include "cppconn/resultset.h"
#include "cppconn/statement.h"
#include "cppconn/prepared_statement.h"

class MySQLBase
{
public:
	MySQLBase();
	virtual ~MySQLBase();
	bool Init(const std::string& hostName, const std::string& userName, const std::string& password, const std::string& dbName);
	static void LogSqlException(const sql::SQLException& e, const std::string& prefix, const std::string& sql);
protected:
	bool Reconnect();
	bool HandleException(sql::SQLException &e);
	sql::PreparedStatement* PrepareStmt(const std::string& sql);
	sql::ResultSet* ExecuteQuery(const std::string& sql);
	int ExecuteUpdate(const std::string& sql);
	sql::Statement* CreateStatement();
private:
	sql::Driver* m_sqlDriver;
	sql::Connection* m_sqlConn;
	std::string m_host;
	std::string m_userName;
	std::string m_password;
	std::string m_dbName;
};