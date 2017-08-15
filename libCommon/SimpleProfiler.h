#pragma once

#include <sys/time.h>
#include <stdio.h>
#include <string>

class CSimpleProfiler
{
public:
	CSimpleProfiler(std::string sPrefix, bool bDebugLog = true, unsigned int iWarnLogTime = 0);
	~CSimpleProfiler();
	void Start();
	unsigned int End();
private:
	struct timeval StartTime;
	struct timeval EndTime;
	std::string m_sTip;
	bool m_bDebugLog;
	bool m_bWarnLog;
	unsigned int m_iWarnLogTime;
};

