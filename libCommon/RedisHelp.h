#pragma once

#include "hiredis.h"

#include <string>
#include <iostream>
#include <sstream>
#include <set>
#include <map>
#include <vector>
#include <string.h>
#include <stdlib.h>

#define REDIS_KEY_EXIST							1
#define REDIS_KEY_NOT_EXIST						0

static const unsigned int  REDIS_MEM_IN_KEY = 1;
static const unsigned int  REDIS_MEM_NOT_IN_KEY = 0;

class RedisHelp
{
public:
	RedisHelp();
	~RedisHelp();
	static redisContext* RedisConnectWithTimeout(const char *ip, int port, const struct timeval tv);
	static redisContext* RedisConnect(const char *ip, int port);
	static void RedisFree(redisContext* pRC);

	//Connection
	bool Auth(redisContext* &pRC, const std::string& password);
	bool SelectDB(redisContext* &pRC, unsigned int dbType);

	//key
	bool DelKey(redisContext* &pRC, const std::string& key);
	bool SetKeyTTL(redisContext* &pRC, const std::string& key, unsigned int time);
	bool GetKeyTTL(redisContext* &pRC, const std::string& key, bool& bKeyExist, int& time);
	bool GetAllKeys(redisContext* &pRC, const std::string& keyList, std::set<std::string>& value, bool& keyExist);
	bool SetKey(redisContext* &pRC, const std::string& key);
	bool IsKeyExist(redisContext* &pRC, const std::string& key, bool& keyExist);

	//string
	bool GetStrValue(redisContext* &pRC, const std::string& key, std::string& value, bool& keyExist);
	bool GetBatchStrValue(redisContext* &pRC, const std::set<std::string>& setKeys, std::map<std::string, std::string>& outMapInfo);
	bool SetStrValue(redisContext* &pRC, const std::string& key, const std::string& value);

	//set
	bool IsSetMember(redisContext* &pRC, const std::string& key, const std::string& value, bool& isMem);
	bool GetSetValue(redisContext* &pRC, const std::string& key, std::set<std::string>& value, bool& keyExist);
	bool AddSetValue(redisContext* &pRC, const std::string& key, const std::string& value);
	bool SetSetValue(redisContext* &pRC, const std::string& key, std::set<std::string>& value);
	bool RemSetValue(redisContext* &pRC, const std::string& key, std::string& value);
	bool batchAddSetValue(redisContext* &pRC, const std::string &key, const std::set<std::string> &value, unsigned int & addCount);
	bool batchRemSetValue(redisContext* &pRC, const std::string &key, const std::set<std::string> &value, unsigned int & rmCount);
	bool mGetSetInterValue(redisContext*&pRC, const std::string &set1key, std::string &set2key ,std::vector<std::string>&value, bool& keyExist);

	//hash
	bool AddHashValue(redisContext* &pRC, const std::string& key, std::string field, std::string strvalue);
	bool delHashValue(redisContext* &pRC, const std::string& key, std::string field);
	bool incrHashValue(redisContext* &pRC, const std::string& key, std::string field, std::string value);
	bool isMemInHash(redisContext* &pRC, const std::string& key, std::string field, bool& isMem);
	bool getHashAllValue(redisContext* &pRC, const std::string& key, std::map<std::string, std::string>& valueMap, bool& keyExist);
	bool GetHashFieldValue(redisContext* &pRC, const std::string& key, std::string field, std::string &strValue, bool& fieldExist);
	bool mSetHashValue(redisContext* &pRC, const std::string& key, std::map<std::string, std::string>& fieldValue);
	bool mGetHashValue(redisContext* &pRC, const std::string& key, std::vector<std::string>& fields, std::map<std::string, std::string>& values, std::vector<std::string>& nonExistingFields);
	
	//List
	bool LPUSHList(redisContext* &pRC, const std::string& key, const std::string& value);
	bool RPUSHList(redisContext* &pRC, const std::string& key, std::string& value);
	bool getListLen(redisContext* &pRC, const std::string& key, unsigned int & len, bool& keyExist);
	bool RPUSHListAndGetLen(redisContext* &pRC, const std::string& key, std::string& value, unsigned int & len);
	bool RPUSHListBinMem(redisContext* &pRC, const std::string& key, std::string& binMem);
	bool LPUSHXList(redisContext* &pRC, const std::string& key, std::string& value);
	bool RPUSHXList(redisContext* &pRC, const std::string& key, std::string& value);
	bool batchRPUSHList(redisContext* &pRC, const std::string& key, std::vector<std::string>& values);
	bool batchLPUSHList(redisContext* &pRC, const std::string& key, std::vector<std::string>& values);
	bool RPOPList(redisContext* &pRC, const std::string& key, std::string& value, bool& keyExist);
	bool LPOPList(redisContext* &pRC, const std::string& key, std::string& value, bool& keyExist);
	bool LREMList(redisContext* &pRC, const std::string& key, std::string& value, std::string count);
	bool LPOPListBinMem(redisContext* &pRC, const std::string& key, std::string& binMem, bool& keyExist);
	bool LRANGEList(redisContext* &pRC, const std::string& key, std::string  startPos, std::string endPos, std::vector<std::string>& result, bool& keyExist);

	//score_set
	bool addBinMemToSortedSet(redisContext* &pRC, const std::string& key, const std::string& binMem, const unsigned int& score);
	bool addMemToSortedSet(redisContext* &pRC, const std::string& key, std::string& mem, const unsigned int & score);
	bool getStrMembers(redisContext* &pRC, const std::string& key, int  start, int  stop, std::vector<std::string> &members);
	bool incrBinMemScore(redisContext* &pRC, const std::string& key, const std::string& binMem, const unsigned int& incrScore);
	bool incrMemScore(redisContext* &pRC, const std::string& key, const std::string& mem, const unsigned int& incrScore);
	bool getScoreByMember(redisContext * &pRC, const std::string& key, const std::string& member, unsigned int& score);
	bool getDescRankByMember(redisContext* &pRC, const std::string& key, const std::string& member, unsigned int& rank);
	bool getRevIntMembers(redisContext* &pRC, const std::string& key, const unsigned int& start, const unsigned int& stop, std::vector<unsigned int>& members);
	bool getBinMemsByScore(redisContext* &pRC, const std::string& key, const unsigned int& min, const unsigned int& max, std::vector<std::string>& binMems);
	bool decrScore(redisContext* &pRC, const std::string& key, std::string& mem, const unsigned int& decrScore);
	bool remRangeByScore(redisContext* &pRC, const std::string& key, const unsigned int& min, const unsigned int& max);
	bool remMemFromSortedSet(redisContext* &pRC, const std::string& key, std::string& mem);
	bool getTotalCountSortedSet(redisContext* &pRC, const std::string& key, unsigned int& totalCount);
	bool UnionPreDaysPlayCntSet(redisContext* &pRC, int num, const std::string& destKey);

	// pipeline
	
	// hash
	bool PipeGetHashAllValue(redisContext* &pRC, const std::string& keyPrefix, const std::set<uint>& keys, std::map<std::string, std::map<std::string, std::string>>& valueMap);
	
	// sorted set
	bool PipeZrevrange(redisContext* &pRC, const std::string& keyPrefix, const std::vector<uint>& keys, const uint start, const uint stop, std::map<uint, std::map<uint, uint>>& result);

private:
	void *redisCommandMy(redisContext* &c, const char *format, ...);

	static std::string m_ip;
	static int m_port;
	static std::string m_Auth;
};