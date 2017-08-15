#include "MySQLBase.h"
#include "SimpleProfiler.h"
#include "logging.h"

#include <mutex>

#define LONG_QUERY_MICROSECONDS 800000

namespace
{
	static std::mutex g_mutex;
}

void MySQLBase::LogSqlException(const sql::SQLException& e, const std::string& prefix, const std::string& sql)
{
	Log(ERROR, "%s.mysql ERROR %d (%s):%s.sql:%s", prefix.c_str(), e.getErrorCode(), e.getSQLStateCStr(), e.what(), sql.c_str());
}

MySQLBase::MySQLBase()
: m_sqlDriver(nullptr)
, m_sqlConn(nullptr)
{
}

MySQLBase::~MySQLBase()
{
	if (m_sqlConn)
	{
		if (!m_sqlConn->isClosed())
		{
			m_sqlConn->close();
		}
		delete m_sqlConn;
		m_sqlConn = nullptr;
	}
}

bool MySQLBase::Init(const std::string& hostName, const std::string& userName, const std::string& password, const std::string& dbName)
{
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		m_sqlDriver = get_driver_instance();// not thread safe
	}
	if (m_sqlDriver)
	{
		m_sqlConn = m_sqlDriver->connect(hostName, userName, password);
	}
	if (m_sqlConn)
	{
		m_sqlConn->setSchema(dbName);
		sql::Statement *stmt;
		stmt = m_sqlConn->createStatement();
		stmt->execute("set names utf8");
		delete stmt;

		m_host = hostName;
		m_userName = userName;
		m_password = password;
		m_dbName = dbName;
	}

	return m_sqlConn ? 1 : 0;
}

bool MySQLBase::Reconnect()
{
	if (m_sqlConn)
	{
		delete m_sqlConn;
		m_sqlConn = NULL;
	}

	try
	{
		m_sqlConn = m_sqlDriver->connect(m_host, m_userName, m_password);
		if (m_sqlConn)
		{
			m_sqlConn->setSchema(m_dbName);
			Log(INFO, "reconnect mysql DB Ok.");
		}
		else
		{
			Log(ERROR, "reconnect mysql DB failed.");
		}
	}
	catch (sql::SQLException &e)
	{
		Log(ERROR, "MySQLBase::Reconnect() throw exception err %d sqlstate %s reason %s host %s user %s pwd %s", 
			e.getErrorCode(), e.getSQLStateCStr(), e.what(), m_host.c_str(), m_userName.c_str(), m_password.c_str());
	}
	
	return m_sqlConn ? 1 : 0;
}

bool MySQLBase::HandleException(sql::SQLException &e)
{
	if (2006 == e.getErrorCode())
	{
		Reconnect();
	}
}

sql::PreparedStatement* MySQLBase::PrepareStmt(const std::string& sql)
{
	return m_sqlConn->prepareStatement(sql);
}

sql::ResultSet* MySQLBase::ExecuteQuery(const std::string& sql)
{
	CSimpleProfiler sp("ExecuteSQL " + sql, true, LONG_QUERY_MICROSECONDS);
	std::unique_ptr<sql::Statement> pstmt(m_sqlConn->createStatement());
	return pstmt->executeQuery(sql);
}

int MySQLBase::ExecuteUpdate(const std::string& sql)
{
	CSimpleProfiler sp("ExecuteSQL " + sql, true, LONG_QUERY_MICROSECONDS);
	std::unique_ptr<sql::Statement> pstmt(m_sqlConn->createStatement());
	return pstmt->executeUpdate(sql);
}

sql::Statement* MySQLBase::CreateStatement()
{
	return m_sqlConn->createStatement();
}