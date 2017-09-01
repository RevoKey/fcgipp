#pragma once

#include "CommonDef.h"
#include <string>
#include <vector>
#include <map>

namespace Helper
{
	std::string urlEncode(std::string str);
	std::string urlDecode(std::string str);

	std::string int2str(long long n);
	bool CopyFile(const char *src_path, const char *dst_path);
	bool MakeDir(const char *path, __mode_t mode);
	std::string randStr(int len = 6);
	int randInt(const int& min, const int& max);
	std::string randIntStr(int len = 4);
	std::string Base64Encode(const std::string& plainText);
	std::string SimpleXor(const std::string& src, const std::string& key);
	size_t Utf8StringLength(const std::string &src);
	std::string GetCurrentDir();

	/**
		Returns a string contructed from the a format specifier
		and a va_list of arguments, analogously to vprintf(3).
		@param format the format specifier.
		@param args the va_list of arguments.
	**/
	std::string vform(const char* format, va_list args);
	void StringFormat(std::string& str, const char* format, ...);

	std::string TrimString(const std::string& str, const char c = ' ');
	std::vector<std::string> SplitString(const std::string& src, const std::string& pattern, bool bTrim = false);

	bool BeforeHour(int hour);

	int GetDayInt();

	void ReplaceStringInPlace(std::string& subject, const std::string& search, const std::string& replace);

	// only tm_sec,tm_min,tm_hour,tm_mday,tm_mon,tm_year are set
	void FastUnixtimeToTm(const time_t& unix_timestamp, struct tm* tm, int time_zone = 8);
	
	time_t FastTmToUnixtime(const tm* tp);

	time_t GetDateUnixTime(const int day_offset = 0);
};



