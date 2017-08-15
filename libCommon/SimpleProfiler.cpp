#include "SimpleProfiler.h"
#include <sstream>
#include <sys/syscall.h>
#include <unistd.h>
#include "logging.h"

using namespace std;

CSimpleProfiler::CSimpleProfiler(string sPrefix, bool bDebugLog /* = true */, unsigned int iWarnLogTime /* = 0 */)
: m_sTip(sPrefix)
, m_bDebugLog(bDebugLog)
, m_bWarnLog(iWarnLogTime > 0)
, m_iWarnLogTime(iWarnLogTime)
{
	Start();
}

CSimpleProfiler::~CSimpleProfiler()
{
	auto elapsed = End();
	stringstream ss;
	ss << "sp_elapsed_time " << elapsed << " " << m_sTip;

	if (m_bDebugLog)
	{
		Log(DEBUG, "%s", ss.str().c_str());
	}
	else
	{
		printf("%s\n", ss.str().c_str());
	}

	if (m_iWarnLogTime && elapsed > m_iWarnLogTime)
	{
		Log(WARN, "%s", ss.str().c_str());
	}
}

void CSimpleProfiler::Start()
{
	gettimeofday(&StartTime, NULL);
}

unsigned int CSimpleProfiler::End()
{
	gettimeofday(&EndTime, NULL);
	return 1000000 * (EndTime.tv_sec - StartTime.tv_sec) + EndTime.tv_usec - StartTime.tv_usec;
}