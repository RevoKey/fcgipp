#include "Helper.h"
#include "logging.h"
#include "RedisHelp.h"
#include "SimpleProfiler.h"

#include <iomanip>
#include <stdarg.h>

using namespace std;

string RedisHelp::m_ip;
string RedisHelp::m_Auth;
int RedisHelp::m_port;

#define LOG_ERROR(pRC) do{if(pRC){LOG_ERROR_CONTEXT(pRC);}else{LOG_ERROR_NULL_CONTEXT();}}while(0)

#define LOG_ERROR_CONTEXT(pRC)\
	Log(ERROR, "[%s:%d] --- error type:%d,%s", __FUNCTION__, __LINE__, pRC->err, pRC->errstr);

#define LOG_REPLY_ERROR(pRC, reply)\
	Log(ERROR, "[%s:%d] redis reply type:%d str:%s error type:%d,%s", __FUNCTION__, __LINE__, reply->type, reply->str ? reply->str : "no str", pRC->err, pRC->errstr);

#define LOG_ERROR_NULL_CONTEXT()\
	Log(ERROR, "[%s:%d] --- redis context is null", __FUNCTION__, __LINE__);

#define LONG_QUERY_MICROSECONDS 100000

RedisHelp::RedisHelp()
{
}

RedisHelp::~RedisHelp()
{
}

redisContext* RedisHelp::RedisConnectWithTimeout(const char *ip, int port, const struct timeval timeout)
{
	CSimpleProfiler sp("RedisConnectWithTimeout");
	redisContext *tmpRedisContext = NULL;
	tmpRedisContext = redisConnectWithTimeout(ip, port, timeout);
	if (NULL == tmpRedisContext)
	{
		LOG_ERROR_NULL_CONTEXT();
	}
	else if (0 != tmpRedisContext->err)
	{
		LOG_ERROR(tmpRedisContext);
		redisFree(tmpRedisContext);
		tmpRedisContext = NULL;
	}
	return tmpRedisContext;
}

redisContext* RedisHelp::RedisConnect(const char *ip, int port)
{
	CSimpleProfiler sp("RedisConnect");
	m_ip = ip;
	m_port = port;
	redisContext *tmpRedisContext = NULL;
	tmpRedisContext = redisConnect(ip, port);
	if (NULL == tmpRedisContext)
	{
		LOG_ERROR_NULL_CONTEXT();
	}
	else if (0 != tmpRedisContext->err)
	{
		LOG_ERROR(tmpRedisContext);
		redisFree(tmpRedisContext);
		tmpRedisContext = NULL;
	}
	return tmpRedisContext;
}

void RedisHelp::RedisFree(redisContext* pRC)
{
	if (pRC)
	{
		redisFree(pRC);
	}
	return;
}

bool RedisHelp::Auth(redisContext* &pRC, const std::string& password)
{
	if (!pRC)
	{
		LOG_ERROR_NULL_CONTEXT();
		return false;
	}
	CSimpleProfiler sp("Auth");
	m_Auth = password;
	bool bret = false;
	redisReply* reply = reply = (redisReply*)redisCommand(pRC, "AUTH %s", password.c_str());
	if (reply)
	{
		if (REDIS_REPLY_STATUS == reply->type)
		{
			if (strcmp(reply->str, "OK") == 0)
			{
				bret = true;
			}
			else
			{
				LOG_ERROR(pRC);
			}
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::SelectDB(redisContext* &pRC, unsigned int redisDBType)
{
	if (!pRC)
	{
		LOG_ERROR_NULL_CONTEXT();
		return false;
	}
	bool bRetCode = true;
	static std::string cmd("SELECT %d");
	redisReply* reply = (redisReply*)redisCommandMy(pRC, cmd.c_str(), redisDBType);
	if (reply)
	{
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
		bRetCode = false;
	}
	return bRetCode;
}

bool RedisHelp::DelKey(redisContext* &pRC, const std::string& key)
{
	bool bRetCode = true;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "DEL %s", key.c_str());
	if (reply)
	{
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
		bRetCode = false;
	}
	return bRetCode ;
}

bool RedisHelp::SetKeyTTL(redisContext* &pRC, const std::string& key, unsigned int time)
{
	std::stringstream newstr;
	newstr << time;
	redisReply* reply = (redisReply*)redisCommandMy(pRC,"expire %s %s",key.c_str(),newstr.str().c_str());
	if (reply)
	{
		freeReplyObject(reply);
		return true;
	}
	else
	{
		LOG_ERROR(pRC);
		return false;
	}
}

bool RedisHelp::GetKeyTTL(redisContext* &pRC, const std::string& key, bool& bKeyExist, int& time)
{
	bool retCode = true;
	bKeyExist = true;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "TTL %s", key.c_str());
	if (reply)
	{
		if (REDIS_REPLY_INTEGER == reply->type)
		{
			if (-1 == reply->integer)
			{
				time = 0;
			}
			else if (-2 == reply->integer)
			{
				bKeyExist = false;
			}
			else
			{
				time = reply->integer;
			}
		}
		else
		{
			LOG_ERROR(pRC);
			retCode = false;
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
		retCode = false;
	}

	return retCode;
}

bool RedisHelp::GetAllKeys(redisContext* &pRC,const std::string& keyList, std::set<std::string>& value, bool& keyExist)
{
	bool bret = false;
	keyExist = false;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "KEYS %s", keyList.c_str());
	if (reply)
	{
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			for (unsigned int j = 0; j < reply->elements; ++j)
			{
				value.insert(reply->element[j]->str);
			}
			keyExist = true;
			bret = true;
		}
		else
		{
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::SetKey(redisContext* &pRC, const std::string& key)
{
	redisReply* reply = (redisReply*)redisCommandMy(pRC, key.c_str());
	if (reply)
	{
		freeReplyObject(reply);
		return true;
	}
	else
	{
		LOG_ERROR(pRC);
		return false;
	}
}

bool RedisHelp::GetStrValue(redisContext* &pRC, const std::string& key, std::string& value, bool& keyExist)
{
	keyExist = false;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "GET %s", key.c_str());
	if (reply)
	{
		if (reply->type == REDIS_REPLY_STRING)
		{
			value = reply->str;
			keyExist = true;
		}
		freeReplyObject(reply);
		return true;
	}
	else
	{
		LOG_ERROR(pRC);
		return false;
	}
}

bool RedisHelp::GetBatchStrValue(redisContext* &pRC, const std::set<std::string>& setKeys, std::map<std::string, std::string>& outMapInfo)
{
	std::map<unsigned int, std::vector<unsigned long long> > missingUidBids;
	std::string command("MGET ");
	unsigned int count = 0;
	std::set<std::string>::const_iterator itSet;
	for (itSet = setKeys.begin(); itSet != setKeys.end(); ++itSet)
	{
		command += *itSet + " ";
		count++;
	}
	if (0 == count)
	{
		return true;
	}
	redisReply* reply = (redisReply*)redisCommandMy(pRC, command.c_str());
	if (reply)
	{
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			unsigned int i = 0;
			for (itSet = setKeys.begin(); itSet != setKeys.end(); ++itSet)
			{
				if (reply->element[i]->type == REDIS_REPLY_STRING)
				{
					outMapInfo[*itSet] = reply->element[i]->str;
				}
				++i;
			}
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return true;
}

bool RedisHelp::SetStrValue(redisContext* &pRC, const std::string& key, const std::string& value)
{
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "SET %s %s", key.c_str(), value.c_str());
	if (reply)
	{
		freeReplyObject(reply);
		return true;
	}
	else
	{
		LOG_ERROR(pRC);
		return false;
	}
}
		
bool RedisHelp::IsKeyExist(redisContext* &pRC, const std::string& key, bool& keyExist)
{
	bool bret = false;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "EXISTS %s", key.c_str());
	if (reply)
	{
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			if (reply->integer == REDIS_KEY_EXIST)
			{
				keyExist = true;
				bret = true;
			}
			else if (reply->integer == REDIS_KEY_NOT_EXIST)
			{
				keyExist = false;
				bret = true;
			}
			else
			{
				keyExist = false;
				LOG_REPLY_ERROR(pRC, reply);
			}
		}
		else
		{
			LOG_REPLY_ERROR(pRC, reply);
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::IsSetMember(redisContext* &pRC, const std::string& key, const std::string& value, bool& isMem)
{
	bool bRet = false;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "SISMEMBER %s %s", key.c_str(), value.c_str());
	if (reply)
	{
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			if (reply->integer == REDIS_MEM_IN_KEY)
			{
				isMem = true;
				bRet = true;
			}
			else if (reply->integer == REDIS_MEM_NOT_IN_KEY)
			{
				isMem = false;
				bRet = true;
			}
			else
			{
				isMem = false;
				bRet = false;
			}
		}
		else
		{
			LOG_ERROR(pRC);
			bRet = false;
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bRet;
}

bool RedisHelp::GetSetValue(redisContext* &pRC, const std::string& key, std::set<std::string>& value, bool& keyExist)
{
	keyExist = false;
	if (false == IsKeyExist(pRC, key, keyExist))
	{
		return false;
	}
	if (keyExist == false)
	{
		return true;
	}
	bool bret = false;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "SMEMBERS %s", key.c_str());
	if (reply)
	{
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			for (unsigned int j = 0; j < reply->elements; ++j)
			{
				value.insert(reply->element[j]->str);
			}
			keyExist = true;
			bret = true;
		}
		else
		{			
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::mGetSetInterValue(redisContext* &pRC, const std::string &set1key, std::string &set2key, std::vector<std::string>&value, bool& keyExist)
{
	keyExist = false;
	if (false == IsKeyExist(pRC, set1key, keyExist))
	{
		return false;
	}
	if (keyExist == false)
		return true;

	if (false == IsKeyExist(pRC, set1key, keyExist))
	{
		return false;
	}
	if (keyExist == false)
		return true;

	bool bret = false;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "SINTER %s %s", set1key.c_str(), set2key.c_str());
	if (reply)
	{
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			for (unsigned int i = 0; i < reply->elements; ++i)
			{
				value.push_back(reply->element[i]->str);
			}
			keyExist = true;
			bret = true;
		}
		else
		{
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::AddSetValue(redisContext* &pRC, const std::string& key, const std::string& value)
{
	bool bret = false;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "SADD %s %s", key.c_str(), value.c_str());
	if (reply)
	{
		freeReplyObject(reply);
		bret = true;
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::SetSetValue(redisContext* &pRC, const std::string& key, std::set<std::string>& value)
{
	bool result = true;
	for (std::set<std::string>::iterator it = value.begin(); it != value.end(); ++it)
	{
		std::string itValue = *it;
		if (false == AddSetValue(pRC, key, itValue))
		{
			result = false;
			DelKey(pRC, key);
			break;
		}
	}
	return result;
}

bool RedisHelp::RemSetValue(redisContext* &pRC, const std::string& key, std::string& value)
{
	bool bret = false;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "SREM %s %s", key.c_str(), value.c_str());
	if (reply)
	{
		freeReplyObject(reply);
		bret = true;
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::batchAddSetValue(redisContext* &pRC, const std::string &key, const std::set<std::string> &value, unsigned int & addCount)
{
	addCount = 0;
	std::vector<const char *> argv(value.size() + 2);
	std::vector<size_t> argvlen(value.size() + 2);
	unsigned int j = 0;
	static char sremcmd[] = "SADD";
	argv[j] = sremcmd;
	argvlen[j] = sizeof(sremcmd) - 1;
	++j;
	argv[j] = key.c_str();
	argvlen[j] = key.size();
	++j;
	for (std::set<std::string>::const_iterator i = value.begin(); i != value.end(); ++i, ++j)
	{
		argv[j] = i->c_str();
		argvlen[j] = i->size();
	}

	bool bret = false;
	redisReply* reply = (redisReply*)redisCommandArgv(pRC, argv.size(), &(argv[0]), &(argvlen[0]));
	if (reply)
	{
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			addCount = reply->integer;
			bret = true;
		}
		else
		{
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::batchRemSetValue(redisContext* &pRC, const std::string &key, const std::set<std::string> &value, unsigned int & rmCount)
{
	bool bret = false;
	rmCount = 0;
	std::vector<const char *> argv(value.size() + 2);
	std::vector<size_t> argvlen(value.size() + 2);

	unsigned int j = 0;
	static char sremcmd[] = "SREM";
	argv[j] = sremcmd;
	argvlen[j] = sizeof(sremcmd) - 1;

	++j;
	argv[j] = key.c_str();
	argvlen[j] = key.size();

	++j;
	for (std::set<std::string>::const_iterator i = value.begin(); i != value.end(); ++i, ++j)
	{
		argv[j] = i->c_str();
		argvlen[j] = i->size();
	}
	redisReply* reply = (redisReply*)redisCommandArgv(pRC, argv.size(), &(argv[0]), &(argvlen[0]));
	if (reply)
	{
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			rmCount = reply->integer;
			bret = true;
		}
		else
		{
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::AddHashValue(redisContext* &pRC,  const std::string& key, std::string field, std::string strvalue)
{
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "HSET %s %s %s", key.c_str(), field.c_str(), strvalue.c_str());
	if (reply)
	{
		freeReplyObject(reply);
		return true;
	}
	else
	{
		LOG_ERROR(pRC);
		return false;
	}
}

bool RedisHelp::delHashValue(redisContext* &pRC, const std::string& key, std::string field)
{
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "HDEL %s %s ", key.c_str(), field.c_str());
	if (reply)
	{
		freeReplyObject(reply);
		return true;
	}
	else
	{
		LOG_ERROR(pRC);
		return false;
	}
}

bool RedisHelp::incrHashValue(redisContext* &pRC, const std::string& key, std::string field, std::string value)
{
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "HINCRBY %s %s %s", key.c_str(), field.c_str(), value.c_str());
	if (reply)
	{
		freeReplyObject(reply);
		return true;
	}
	else
	{
		LOG_ERROR(pRC);
		return false;
	}
}

bool RedisHelp::isMemInHash(redisContext* &pRC, const std::string& key, std::string field, bool& isMem)
{	
	isMem = false;
	bool retCode = false;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "HEXISTS %s %s", key.c_str(), field.c_str());
	if (reply)
	{
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			if (reply->integer == REDIS_MEM_IN_KEY)
			{
				isMem = true;
				retCode = true;
			}
			else if (reply->integer == REDIS_MEM_NOT_IN_KEY)
			{
				isMem = false;
				retCode = true;
			}
			else
			{
				isMem = false;
				retCode = false;
			}
		}
		else
		{
			LOG_ERROR(pRC);
			retCode = false;
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
		retCode = false;
	}
	return retCode;
}

bool RedisHelp::getHashAllValue(redisContext* &pRC, const std::string& key, std::map<std::string, std::string>& valueMap, bool& keyExist)
{
	keyExist = false;
	std::string strField;
	std::string strValue;
	std::string newValue;

	bool bret = false;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "HGETALL %s", key.c_str());
	if (reply)
	{
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			for (unsigned int j = 0; j < reply->elements; ++j)
			{
				strField = reply->element[j]->str;
				if (j < reply->elements)  
				{
					j++;
					strValue = reply->element[j]->str;
					valueMap[strField] = strValue;
				}
				keyExist = true;
			}
			bret = true;
		}
		else
		{
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::GetHashFieldValue(redisContext* &pRC, const std::string& key, std::string field, std::string &strValue, bool& fieldExist)
{
	fieldExist = false;
	bool bret =false;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "HGET %s %s", key.c_str(), field.c_str());
	if (reply)
	{
		if (reply->type == REDIS_REPLY_STRING)
		{
			strValue = reply->str;
			fieldExist = true;
			bret = true;
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			// field not exists
			bret = true;
		}
		else
		{		
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::mSetHashValue(redisContext* &pRC, const std::string& key, std::map<std::string, std::string>& fieldValue)
{
	std::vector<const char *> argv(fieldValue.size() * 2 + 2);
	std::vector<size_t> argvlen(fieldValue.size() * 2 + 2);

	unsigned int  j = 0;
	static char msethash[] = "HMSET";
	argv[j] = msethash;
	argvlen[j] = sizeof(msethash) - 1;

	++j;
	argv[j] = key.c_str();
	argvlen[j] = key.size();

	++j;
	for (std::map<std::string, std::string>::const_iterator i = fieldValue.begin(); i != fieldValue.end(); i++, j++)
	{
		argv[j] = i->first.c_str();
		argvlen[j] = i->first.size();

		j++;
		argv[j] = i->second.c_str();
		argvlen[j] = i->second.size();
	}

	bool bret = false;
	redisReply* reply = (redisReply*)redisCommandArgv(pRC, argv.size(), &(argv[0]), &(argvlen[0]));
	if (reply)
	{
		if (REDIS_REPLY_STATUS == reply->type)
		{
			if (strcmp(reply->str, "OK") == 0)
			{
				bret = true;
			}
			else
			{
				LOG_ERROR(pRC);
			}
		}
		else
		{
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::mGetHashValue(redisContext* &pRC, const std::string& key, std::vector<std::string>& fields, std::map<std::string, std::string>& values, std::vector<std::string>& nonExistingFields)
{
	if (fields.empty())
	{
		LOG_ERROR(pRC);
		return false;
	}
	std::stringstream cmd;
	cmd << "HMGET " << key;
	for (std::vector<std::string>::iterator it = fields.begin(); it != fields.end(); ++it)
	{
		cmd << " " << *it;
	}

	bool bret = false;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, cmd.str().c_str());
	if (reply)
	{
		if (REDIS_REPLY_ARRAY == reply->type)
		{
			for (unsigned int j = 0; j < reply->elements; ++j)
			{
				if (REDIS_REPLY_NIL != reply->element[j]->type)
				{
					//field exists
					values[fields[j]] = reply->element[j]->str;
				}
				else
				{
					nonExistingFields.push_back(fields[j]);
				}
			}
			bret = true;
		}
		else
		{
			LOG_ERROR(pRC);	
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::getListLen(redisContext* &pRC, const std::string& key, unsigned int & len, bool& keyExist)
{
	len = 0;
	keyExist = false;
	if (false == IsKeyExist(pRC, key, keyExist))
	{
		return false;
	}
	if (keyExist == false)
	{
		return true;
	}
	bool bret = false;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "LLEN %s", key.c_str());
	if (reply)
	{
		if (reply->type == REDIS_REPLY_STRING)
		{
			std::string slen = reply->str;
			len = atoi(slen.c_str());
			keyExist = true;
			bret = true;
		}
		else if (reply->type == REDIS_REPLY_INTEGER)
		{
			len = reply->integer;
			keyExist = true;
			bret = true;
		}
		else
		{
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::LPUSHList(redisContext* &pRC, const std::string& key, const std::string& value)
{
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "LPUSH %s %s", key.c_str(), value.c_str());
	if (reply)
	{
		freeReplyObject(reply);
		return true;
	}
	else
	{
		LOG_ERROR(pRC);
		return false;
	}
}

bool RedisHelp::RPUSHList(redisContext* &pRC, const std::string& key, std::string& value)
{
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "RPUSH %s %s", key.c_str(), value.c_str());
	if (reply)
	{
		freeReplyObject(reply);
		return true;
	}
	else
	{
		LOG_ERROR(pRC);
		return false;
	}
}

bool RedisHelp::RPUSHListAndGetLen(redisContext* &pRC, const std::string& key, std::string& value, unsigned int & len)
{
	len = 0;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "RPUSH %s %s", key.c_str(), value.c_str());

	bool bret = false;
	if (reply)
	{
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			len = reply->integer;
			bret = true;
		}
		else
		{
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::RPUSHListBinMem(redisContext* &pRC, const std::string& key, std::string& binMem)
{
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "RPUSH %s %b", key.c_str(), binMem.c_str(), binMem.size());
	if (reply)
	{
		bool retCode = false;
		if (REDIS_REPLY_INTEGER == reply->type)
		{
			retCode = true;
		}
		else if (REDIS_REPLY_ERROR == reply->type)
		{
			LOG_ERROR(pRC);
			retCode = false;
		}
		else
		{
		    LOG_ERROR(pRC);
			retCode = false;
		}
		freeReplyObject(reply);
		return retCode;
	}
	else
	{
		LOG_ERROR(pRC);
		return false;
	}
}

bool RedisHelp::LPUSHXList(redisContext* &pRC, const std::string& key, std::string& value)
{
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "LPUSHX %s %s", key.c_str(), value.c_str());
	if (reply)
	{
		freeReplyObject(reply);
		return true;
	}
	else
	{
		LOG_ERROR(pRC);
		return false;
	}
}

bool RedisHelp::RPUSHXList(redisContext* &pRC, const std::string& key, std::string& value)
{
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "RPUSHX %s %s", key.c_str(), value.c_str());
	if (reply)
	{
		freeReplyObject(reply);
		return true;
	}
	else
	{
		LOG_ERROR(pRC);
		return false;
	}
}

bool RedisHelp::batchRPUSHList(redisContext* &pRC, const std::string& key, std::vector<std::string>& values)
{
	redisReply* reply;
	std::vector<std::string>::iterator it;
	for (it = values.begin(); it != values.end(); ++it)
	{
		std::string itValue = *it;
		reply = (redisReply*)redisCommandMy(pRC, "RPUSH %s %s", key.c_str(), itValue.c_str());
		if (reply)
		{
			freeReplyObject(reply);
		}
		else
		{
			LOG_ERROR(pRC);
			return false;
		}
	}
	return true;
}

bool RedisHelp::batchLPUSHList(redisContext* &pRC, const std::string& key, std::vector<std::string>& values)
{
	redisReply* reply;
	std::vector<std::string>::iterator it;
	for (it = values.begin(); it != values.end(); ++it)
	{
		std::string itValue = *it;
		reply = (redisReply*)redisCommandMy(pRC, "LPUSH %s %s", key.c_str(), itValue.c_str());
		if (reply)
		{
			freeReplyObject(reply);
		}
		else
		{
			LOG_ERROR(pRC);
			return false;
		}
	}
	return true;
}

bool RedisHelp::RPOPList(redisContext* &pRC, const std::string& key, std::string& value, bool& keyExist)
{
	keyExist = false;

	redisReply* reply = (redisReply*)redisCommandMy(pRC, "RPOP %s", key.c_str());

	bool bret = false;
	if (reply)
	{
		if (reply->type == REDIS_REPLY_STRING)
		{
			value = reply->str;
			keyExist = true;
			bret = true;
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			bret = true;
		}
		else
		{
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::LPOPList(redisContext* &pRC, const std::string& key, std::string& value, bool& keyExist)
{	
	keyExist = false;
	bool bret = false;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "LPOP %s", key.c_str());
	if (reply)
	{
		if (reply->type == REDIS_REPLY_STRING)
		{
			value = reply->str;
			keyExist = true;
			bret = true;
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			bret = true;
		}
		else
		{
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::LPOPListBinMem(redisContext* &pRC, const std::string& key, std::string& binMem, bool& keyExist)
{
	keyExist = false;
	bool bret = false;
	redisReply* reply = (redisReply*)redisCommandMy(pRC, "LPOP %s", key.c_str());
	if (reply)
	{
		if (reply->type == REDIS_REPLY_STRING)
		{
			binMem.assign(reply->str, reply->len);
			keyExist = true;
			bret = true;
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			bret = true;
		}
		else
		{
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
	}
	return bret;
}

bool RedisHelp::LREMList(redisContext* &pRC, const std::string& key, std::string& value, std::string count)
{
	redisReply* reply = (redisReply*)redisCommandMy(pRC,
		"LREM %s %s %s", 
		key.c_str(),
		count.c_str(),
		value.c_str());
	if (reply)
	{
		freeReplyObject(reply);
		return true;
	}
	else
	{
		LOG_ERROR(pRC);
		return false;
	}
}

bool RedisHelp::LRANGEList(redisContext* &pRC, const std::string& key, std::string  startPos, std::string endPos, std::vector<std::string>& result, bool& keyExist)
{
	keyExist = false;
	if (false == IsKeyExist(pRC, key, keyExist))
	{
		LOG_ERROR(pRC);
		return false;
	}
	if (keyExist == false)
	{
		return true;
	}
	redisReply* reply = (redisReply*)redisCommandMy(pRC,
		"LRANGE %s %s %s", 
		key.c_str(),
		startPos.c_str(),
		endPos.c_str());
	if (reply)
	{
		if (reply->type == REDIS_REPLY_ARRAY)
		{
			for (unsigned int j = 0; j < reply->elements; j++)
			{
				result.push_back(reply->element[j]->str);
			}
			keyExist = true;
		}

		freeReplyObject(reply);
		return true;
	}
	else
	{
		LOG_ERROR(pRC);
		return false;
	}
}

bool RedisHelp::addBinMemToSortedSet(redisContext* &pRC, const std::string& key, const std::string& binMem, const unsigned int& score)
{
	bool retCode = true;
	std::ostringstream cmdStr;
	cmdStr << "ZADD " << key << " " << score << " %b";
	redisReply *reply = (redisReply*)redisCommandMy(pRC, cmdStr.str().c_str(), binMem.c_str(), binMem.size());
	if (reply)
	{
		if (REDIS_REPLY_INTEGER == reply->type)
		{
			retCode = true;
		}
		else
		{
			LOG_ERROR(pRC);
			retCode = false;
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
		retCode = false;
	}

	return retCode;
}

bool RedisHelp::addMemToSortedSet(redisContext* &pRC,const std::string& key,std::string& mem,const unsigned int & score)
{
	bool retCode = true;
	std::ostringstream cmdStr;
	cmdStr << "ZADD " << key << " " << score << " %s";
	redisReply *reply = (redisReply*)redisCommandMy(pRC, cmdStr.str().c_str(), mem.c_str());
	if (reply)
	{
		if (REDIS_REPLY_INTEGER == reply->type)
		{
			retCode = true;
		}
		else
		{
			retCode = false;
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
	}
	else
	{
		LOG_ERROR(pRC);
		retCode = false;
	}
	return retCode;
}

bool RedisHelp::getStrMembers(redisContext* &pRC, const std::string& key, int  start, int  stop, std::vector<std::string> &members)
{
	bool retCode = true;
	std::ostringstream cmdStr;

	cmdStr << "ZRANGE " << key << " " << start << " " << stop;
	redisReply *reply = (redisReply*)redisCommandMy(pRC, cmdStr.str().c_str());
	if (reply)
	{
		if (REDIS_REPLY_NIL == reply->type)
		{
			retCode = true;
		}
		else if (REDIS_REPLY_ARRAY == reply->type)
		{
			if (reply->elements < 1)
			{
			}
			else
			{
				for (unsigned int i = 0; i < reply->elements;)
				{
					std::string str = reply->element[i++]->str;
					members.push_back(str);
				}
			}
			retCode = true;
		}
		else
		{
			retCode = false;
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
	}
	else
	{
		retCode = false;
		LOG_ERROR(pRC);
	}
	return retCode;
}

bool RedisHelp::incrBinMemScore(redisContext* &pRC, const std::string& key, const std::string& binMem, const unsigned int& incrScore)
{

	bool retCode = true;
	std::ostringstream cmdStr;
	cmdStr << "ZINCRBY " << key << " " << incrScore << " %b";

	redisReply *reply = (redisReply*)redisCommandMy(pRC, cmdStr.str().c_str(), binMem.c_str(), binMem.size());
	if (reply)
	{
		if (REDIS_REPLY_STRING == reply->type)
		{
			retCode = true;
		}
		else
		{
			retCode = false;
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
	}
	else
	{
		retCode = false;
		LOG_ERROR(pRC);
	}

	return retCode;
}

bool RedisHelp::UnionPreDaysPlayCntSet(redisContext* &pRC, int num,const std::string& destKey)
{
	bool retCode = true;
	time_t theTime = time(NULL);
	struct tm *aTime = localtime(&theTime);
	
	std::ostringstream cmdStr;
	cmdStr << "ZUNIONSTORE " << destKey << " " << num;
	for (int i = 0; i < num; i++)
	{
		std::stringstream buffer;
		buffer << "playCnt:";
		buffer << aTime->tm_year + 1900 << "_";
		buffer << std::setw(2) << std::setfill('0') << aTime->tm_mon + 1 << "_";
		buffer << std::setw(2) << std::setfill('0') << aTime->tm_mday;

		cmdStr << " " << buffer.str();

		theTime -= 24 * 60 * 60;
		aTime = localtime(&theTime);
	}

	redisReply *reply = (redisReply*)redisCommandMy(pRC, cmdStr.str().c_str());
	if (reply)
	{
		if (REDIS_REPLY_INTEGER == reply->type)
		{
			retCode = true;
		}
		else
		{
			retCode = false;
			LOG_REPLY_ERROR(pRC, reply);
		}
		freeReplyObject(reply);
	}
	else
	{
		retCode = false;
		LOG_ERROR(pRC);
	}
	return retCode;
}

bool RedisHelp::incrMemScore(redisContext* &pRC,const std::string& key, const std::string& mem,const unsigned int& incrScore)
{
	bool retCode = true;
	std::ostringstream cmdStr;
	cmdStr << "ZINCRBY " << key << " " << incrScore << " %s";
	redisReply *reply = (redisReply*)redisCommandMy(pRC, cmdStr.str().c_str(), mem.c_str());
	if (reply)
	{
		if (REDIS_REPLY_STRING == reply->type)
		{
			retCode = true;
		}
		else
		{
			retCode = false;
			LOG_REPLY_ERROR(pRC, reply);
		}
		freeReplyObject(reply);
	}
	else
	{
		retCode = false;
		LOG_ERROR(pRC);
	}
	return retCode;
}

bool RedisHelp::getScoreByMember(redisContext* &pRC, const std::string& key, const std::string& member, unsigned int& score)
{
	std::ostringstream cmdStr;
	cmdStr << "ZSCORE " << key << " " << member;
	redisReply *reply = (redisReply*)redisCommandMy(pRC, cmdStr.str().c_str());
	if (reply)
	{
		bool retCode = true;
		if (REDIS_REPLY_NIL == reply->type)
		{
			retCode = true;
		}
		else if (REDIS_REPLY_STRING == reply->type)
		{
			score = atoi(reply->str);
			retCode = true;
		}
		else
		{
			retCode = false;
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
		return retCode;
	}
	else
	{
		return false;
		LOG_ERROR(pRC);
	}
}

bool RedisHelp::getDescRankByMember(redisContext* &pRC,const std::string& key,const std::string& member,unsigned int& rank)
{
	std::ostringstream cmdStr;
	cmdStr << "ZREVRANK " << key << " " << member;
	redisReply *reply = (redisReply*)redisCommandMy(pRC, cmdStr.str().c_str());
	if (reply)
	{
		bool retCode = true;
		if (REDIS_REPLY_NIL == reply->type)
		{
			retCode = true;
		}
		else if (REDIS_REPLY_INTEGER == reply->type)
		{
			rank = reply->integer;
			++rank;

			retCode = true;
		}
		else
		{
			retCode = false;
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
		return retCode;
	}
	else
	{
		return false;
		LOG_ERROR(pRC);
	}
}

bool RedisHelp::getRevIntMembers(redisContext* &pRC,const std::string& key,const unsigned int& start,const unsigned int& stop,std::vector<unsigned int>& members)
{
	std::ostringstream cmdStr;
	cmdStr << "ZREVRANGE " << key << " " << start << " " << stop;
	redisReply *reply = (redisReply*)redisCommandMy(pRC, cmdStr.str().c_str());
	if (reply)
	{
		bool retCode = true;
		if (REDIS_REPLY_NIL == reply->type)
		{
			retCode = true;
		}
		else if (REDIS_REPLY_ARRAY == reply->type)
		{
			if (reply->elements < 1)
			{

			}
			else
			{
				for (unsigned int i = 0; i < reply->elements; ++i)
				{
					members.push_back((unsigned int)atoi(reply->element[i]->str));
				}
			}

			retCode = true;
		}
		else
		{
			retCode = false;
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
		return retCode;
	}
	else
	{
		return false;
		LOG_ERROR(pRC);
	}
}

bool RedisHelp::getBinMemsByScore(redisContext* &pRC, const std::string& key, const unsigned int & min, const unsigned int & max, std::vector<std::string>& binMems)
{
	if (min > max)
	{
		return false;
		LOG_ERROR(pRC);
	}
	std::ostringstream cmdStr;
	cmdStr << "ZRANGEBYSCORE " << key << " " << min << " " << max;
	redisReply *reply = (redisReply*)redisCommandMy(pRC, cmdStr.str().c_str());
	if (reply)
	{
		bool retCode = true;
		if (REDIS_REPLY_NIL == reply->type)
		{
			retCode = true;
		}
		else if (REDIS_REPLY_ARRAY == reply->type)
		{
			if (reply->elements < 1)
			{
			}
			else
			{
				for (unsigned int i = 0; i < reply->elements; ++i)
				{
					std::string mem(reply->element[i]->str, reply->element[i]->len);
					binMems.push_back(mem);
				}
			}

			retCode = true;
		}
		else
		{
			retCode = false;
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
		return retCode;
	}
	else
	{
		return false;
		LOG_ERROR(pRC);
	}
}

bool RedisHelp::decrScore(redisContext* &pRC,const std::string& key,std::string& mem,const unsigned int & decrScore)
{
	bool retCode = true;
	std::stringstream cmdStr;
	cmdStr << "ZINCRBY " << key << " -" << decrScore << " %s";
	redisReply *reply = (redisReply*)redisCommandMy(pRC, cmdStr.str().c_str(), mem.c_str());
	if (reply)
	{
		if (REDIS_REPLY_STRING == reply->type)
		{
			retCode = true;
		}
		else
		{
			retCode = false; 
			LOG_ERROR(pRC);
		}
		freeReplyObject(reply);
	}
	else
	{
		retCode = false;
		LOG_ERROR(pRC);
	}

	return retCode;
}

bool RedisHelp::remRangeByScore(redisContext* &pRC, const std::string& key, const unsigned int & min, const unsigned int& max)
{
	if (min > max)
	{
		return false;
	}
	std::ostringstream cmdStr;
	cmdStr << "ZREMRANGEBYSCORE " << key << " " << min << " " << max;
	redisReply *reply = (redisReply*)redisCommandMy(pRC, cmdStr.str().c_str());
	if (reply)
	{
		bool retCode = true;
		if (REDIS_REPLY_INTEGER == reply->type)
		{
			retCode = true;
		}
		else
		{
			LOG_ERROR(pRC);
			retCode = false;
		}
		freeReplyObject(reply);
		return retCode;
	}
	else
	{
		LOG_ERROR(pRC);
		return false;
	}
}

bool RedisHelp::remMemFromSortedSet(redisContext* &pRC, const std::string& key, std::string& mem)
{
	std::ostringstream cmdStr;
	cmdStr << "ZREM " << key << " %s";
	redisReply *reply = (redisReply*)redisCommandMy(pRC, cmdStr.str().c_str(), mem.c_str());
	if (reply)
	{
		bool retCode = true;
		if (REDIS_REPLY_INTEGER == reply->type)
		{
			retCode = true;
		}
		else
		{
			LOG_ERROR(pRC);
			retCode = false;
		}
		freeReplyObject(reply);
		return retCode;
	}
	else
	{
		LOG_ERROR(pRC);
		return false;
	}
}

bool RedisHelp::getTotalCountSortedSet(redisContext* &pRC, const std::string& key, unsigned int & totalCount)
{
	std::ostringstream cmdStr;
	cmdStr << "ZCARD " << key;
	redisReply *reply = (redisReply*)redisCommandMy(pRC, cmdStr.str().c_str());
	if (reply)
	{
		bool retCode = true;

		if (REDIS_REPLY_INTEGER == reply->type)
		{
			totalCount = reply->integer;
			retCode = true;
		}
		else
		{
			LOG_ERROR(pRC);
			retCode = false;
		}
		freeReplyObject(reply);
		return retCode;
	}
	else
	{
		LOG_ERROR(pRC);
		return false;
	}
}

bool RedisHelp::PipeGetHashAllValue(redisContext* &pRC, const std::string& keyPrefix, const std::set<uint>& keys, std::map<std::string, std::map<std::string, std::string>>& valueMap)
{
	if (!pRC)
	{
		return false;
	}

	vector<string> vReplyKeys;
	for (auto it = keys.begin(); it != keys.end(); ++it)
	{
		if (REDIS_OK == redisAppendCommand(pRC, "HGETALL %s%d", keyPrefix.c_str(), *it))
		{
			vReplyKeys.push_back(to_string(*it));
		}
		else
		{
			LOG_ERROR(pRC);
		}
	}

	redisReply* reply = nullptr;
	for (auto it = vReplyKeys.begin(); it != vReplyKeys.end(); ++it)
	{
		if (REDIS_OK == redisGetReply(pRC, (void**)&reply))
		{
			if (REDIS_REPLY_ARRAY == reply->type)
			{
				for (size_t i = 1; i < reply->elements; i += 2)
				{
					valueMap[*it][reply->element[i-1]->str] = reply->element[i]->str;
				}
			}
			else
			{
				LOG_REPLY_ERROR(pRC, reply);
			}
			freeReplyObject(reply);
			reply = nullptr;
		}
		else
		{
			LOG_ERROR(pRC);
		}
	}

	return true;
}

bool RedisHelp::PipeZrevrange(redisContext* &pRC, const std::string& keyPrefix, const vector<uint>& keys, const uint start, const uint stop, map<uint, map<uint, uint>>& result)
{
	if (!pRC)
	{
		return false;
	}

	vector<uint> vReplyKeys;
	for (auto it = keys.begin(); it != keys.end(); ++it)
	{
		if (REDIS_OK == redisAppendCommand(pRC, "ZREVRANGE %s%d %d %d WITHSCORES", keyPrefix.c_str(), *it, start, stop))
		{
			vReplyKeys.push_back(*it);
		}
		else
		{
			LOG_ERROR(pRC);
		}
	}

	redisReply* reply = nullptr;
	for (auto it = vReplyKeys.begin(); it != vReplyKeys.end(); ++it)
	{
		if (REDIS_OK == redisGetReply(pRC, (void**)&reply))
		{
			if (REDIS_REPLY_ARRAY == reply->type)
			{
				for (size_t i = 1; i < reply->elements; i += 2)
				{
					result[*it][atoi(reply->element[i - 1]->str)] = atoi(reply->element[i]->str);
				}
			}
			else
			{
				LOG_REPLY_ERROR(pRC, reply);
			}
			freeReplyObject(reply);
			reply = nullptr;
		}
		else
		{
			LOG_ERROR(pRC);
		}
	}

	return true;
}

void* RedisHelp::redisCommandMy(redisContext* &c, const char *format, ...)
{
	if (!c)
	{
		LOG_ERROR_NULL_CONTEXT();
		c = RedisConnect(m_ip.c_str(), m_port);
		if (c)
		{
			Log(INFO, "Reconnct Redis Ok!");
			Auth(c, m_Auth);

			va_list va2;
			va_start(va2, format);
			redisReply* reply = (redisReply*)redisvCommand(c, format, va2);
			va_end(va2);
		}
		else
		{
			Log(ERROR, "Reconnct Redis Failed!");
			return nullptr;
		}
	}

	//get command's detail info
	va_list va0;
	va_start(va0, format);
	string strCmd = Helper::vform(format, va0);
	va_end(va0);

	stringstream ss;
	ss << "redisCommandMy " << strCmd.c_str() << " ";
	CSimpleProfiler sp(ss.str(), true, LONG_QUERY_MICROSECONDS);

	va_list va;
	va_start(va, format);
	redisReply* reply = (redisReply*)redisvCommand(c, format, va);
	va_end(va);

	if (NULL == reply)
	{
		// do reconnect
		redisFree(c);
		c = NULL;
		c = RedisConnect(m_ip.c_str(), m_port);
		if (NULL != c)
		{
			Log(INFO, "Reconnct Redis Ok!");
			Auth(c, m_Auth);

			va_list va2;
			va_start(va2, format);
			reply = (redisReply*)redisvCommand(c, format, va2);
			va_end(va2);
		}
		else
		{
			Log(ERROR, "Reconnct Redis Failed!");
		}
	}

	if (reply && REDIS_REPLY_ERROR == reply->type
		&& strcmp(reply->str, "BUSY ERRCODE[32] conflict with another client") == 0)
	{
		Log(INFO, "%s error and retry", ss.str().c_str());
		va_list vl;
		va_start(vl, format);
		reply = (redisReply*)redisvCommand(c, format, vl);
		va_end(vl);
	}

	return reply;
}
