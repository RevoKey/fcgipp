#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "Helper.h"
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>
#include <sstream>
#include "md5.h"
#include <random>
#include <sys/wait.h>
#include "CodeConverter.h"
#include <iomanip>

using namespace std;

namespace
{
#define RAND_CHAR_NUM 62 // 26 + 26 + 10
	const char g_randChar[RAND_CHAR_NUM] = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 
		'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
		'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
	};
}

std::string Helper::urlEncode(std::string str)
{
	string new_str = "";
	unsigned char c;
	int ic;
	const char* chars = str.c_str();
	char bufHex[10] = { 0 };
	int len = strlen(chars);

	for (int i = 0; i<len; i++)
	{
		c = chars[i];
		ic = c;
		// uncomment this if you want to encode spaces with +
		/*if (c==' ') new_str += '+';
		else */if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') new_str += c;
		else
		{
			sprintf(bufHex, "%X", c);
			if (ic < 16)
				new_str += "%0";
			else
				new_str += "%";
			new_str += bufHex;
		}
	}
	return new_str;
}

std::string Helper::urlDecode(std::string str)
 {
	string ret;
	char ch;
	int i, ii, len = str.length();

	for (i = 0; i < len; i++)
	{
		if (str[i] != '%')
		{
			if (str[i] == '+')
				ret += ' ';
			else
				ret += str[i];
		}
		else
		{
			sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
			ch = static_cast<char>(ii);
			ret += ch;
			i = i + 2;
		}
	}
	return ret;
}

string Helper::int2str(long long n)
{
	stringstream ss;
	ss << n;
	return ss.str();
}

bool Helper::CopyFile(const char *src_path, const char *dst_path)
{
	auto ret = false;
	std::ifstream src(src_path, ios::binary);
	if (src.is_open())
	{
		std::ofstream dst(dst_path, ios::binary);
		if (dst.is_open())
		{
			dst << src.rdbuf();
			dst.close();
			ret = true;
		}
		src.close();
	}
	return ret;
}

bool Helper::MakeDir(const char *path, __mode_t mode)
{
	char dir[PATH_MAX] = { 0 };
	snprintf(dir, sizeof(dir), "%s", path);
	size_t len = strlen(dir);
	if ('/' != dir[len - 1])
	{
		strcat(dir, "/");
		len = strlen(dir);
	}

	for (auto p = dir + 1; *p; p++)
	{
		if ('/' == *p)
		{
			*p = 0;
			if (0 != access(dir, F_OK))
			{
				if (-1 == mkdir(dir, mode))
				{
					return false;
				}
			}
			*p = '/';
		}
	}

	return true;
}

std::string Helper::randStr(int len /* = 6 */)
{
	std::string ret = "";
	for (int i = 0; i < len; i++)
	{
		ret += g_randChar[randInt(1, RAND_CHAR_NUM) % RAND_CHAR_NUM];
	}
	return ret;
}

int Helper::randInt(const int& min, const int& max)
{
	static std::random_device rd;
	static std::mt19937 generator(rd());
	std::uniform_int_distribution<int> distribution(min, max);
	return distribution(generator);
}

std::string Helper::randIntStr(int len /* = 4 */)
{
	stringstream ss;
	for (int i = 0; i < len; i++)
	{
		ss << randInt(0,9);
	}
	return ss.str();
}

std::string Helper::Base64Encode(const std::string& plainText) 
{
	static const char b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	const std::size_t plainTextLen = plainText.size();
	string retval((((plainTextLen + 2) / 3) * 4), '=');
	std::size_t outpos = 0;
	int bits_collected = 0;
	unsigned int accumulator = 0;
	const string::const_iterator plainTextEnd = plainText.end();

	for (string::const_iterator i = plainText.begin(); i != plainTextEnd; ++i) 
	{
		accumulator = (accumulator << 8) | (*i & 0xffu);
		bits_collected += 8;
		while (bits_collected >= 6) 
		{
			bits_collected -= 6;
			retval[outpos++] = b64_table[(accumulator >> bits_collected) & 0x3fu];
		}
	}

	if (bits_collected > 0) 
	{
		accumulator <<= 6 - bits_collected;
		retval[outpos++] = b64_table[accumulator & 0x3fu];
	}
	return retval;
}

std::string Helper::SimpleXor(const std::string& src, const std::string& key)
{
	if (src.empty() || key.empty())
	{
		return "";
	}
	std::string ret;
	unsigned int i = 0;
	unsigned int j = 0;
	for (; i < src.size(); ++i)
	{
		ret.push_back(static_cast<unsigned char>(src[i] ^ key[j]));
		j = (++j) % (key.length());
	}
	return ret;
}

size_t Helper::Utf8StringLength(const std::string &src)
{
	size_t length = 0;
	for (size_t i = 0, len = 0; i < src.length();)
	{
		unsigned char byte = src[i];
		if (byte >= 0xFC)
			len = 6;
		else if (byte >= 0xF8)
			len = 5;
		else if (byte >= 0xF0)
			len = 4;
		else if (byte >= 0xE0)
			len = 3;
		else if (byte >= 0xC0)
			len = 2;
		else
			len = 1;
		length++;
		i += len;
	}
	return length;
}

std::string Helper::GetCurrentDir()
{
	char buf[1024] = { 0 };
	int len = readlink("/proc/self/exe", buf, 1024);
	std::string path(buf, len);
	return path.substr(0, path.rfind("/"));
}

std::string Helper::vform(const char* format, va_list args) 
{
	size_t size = 1024;
	char* buffer = new char[size];

	while (1) 
	{
		va_list args_copy;
		va_copy(args_copy, args);
		int n = vsnprintf(buffer, size, format, args_copy);
		va_end(args_copy);

		// If that worked, return a string.
		if ((n > -1) && (static_cast<size_t>(n) < size)) 
		{
			std::string s(buffer);
			delete[] buffer;
			return s;
		}

		// Else try again with more space.
		size = (n > -1) ?
			n + 1 :   // ISO/IEC 9899:1999
			size * 2; // twice the old size

		delete[] buffer;
		buffer = new char[size];
	}
}

void Helper::StringFormat(std::string& str, const char* format, ...)
{
	va_list va;
	va_start(va, format);
	str = vform(format, va);
	va_end(va);
}

std::string Helper::TrimString(const std::string& str, const char c /* = ' ' */)
{
	std::string::size_type pos = str.find_first_not_of(c);
	if (pos == std::string::npos)
	{
		return str;
	}
	std::string::size_type pos2 = str.find_last_not_of(c);
	if (pos2 != std::string::npos)
	{
		return str.substr(pos, pos2 - pos + 1);
	}
	return str.substr(pos);
}

std::vector<std::string> Helper::SplitString(const std::string& src, const std::string& pattern, bool bTrim /* = false */)
{
	std::string str = src;
	std::string::size_type pos;
	std::vector<std::string> result;
	str += pattern;
	int size = str.size();
	for (int i = 0; i < size;)
	{
		pos = str.find(pattern, i);
		if (pos != std::string::npos)
		{
			std::string s = str.substr(i, pos - i);
			if (!s.empty())
			{
				result.push_back(bTrim ? Helper::TrimString(s) : s);
			}
			i = pos + pattern.size();
		}
		else
		{
			break;
		}
	}
	return result;
}

bool Helper::BeforeHour(int hour)
{
	time_t theTime = time(NULL);
	struct tm *aTime = localtime(&theTime);
	return aTime->tm_hour < hour;
}

int Helper::GetDayInt()
{
	time_t theTime = time(NULL);
	struct tm *aTime = localtime(&theTime);
	auto ret = (1900 + aTime->tm_year) * 10000 + (1 + aTime->tm_mon) * 100 + aTime->tm_mday;

	return ret;
}

void Helper::ReplaceStringInPlace(std::string& subject, const std::string& search, const std::string& replace)
{
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos)
	{
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
}

void Helper::FastUnixtimeToTm(const time_t& unix_timestamp, struct tm* tm, int time_zone /* = 8 */)
{
	static const int kHoursInDay = 24;
	static const int kMinutesInHour = 60;
	static const int kDaysFromUnixTime = 2472632;
	static const int kDaysFromYear = 153;
	static const int kMagicUnkonwnFirst = 146097;
	static const int kMagicUnkonwnSec = 1461;
	tm->tm_sec = unix_timestamp % kMinutesInHour;
	int i = (unix_timestamp / kMinutesInHour);
	tm->tm_min = i % kMinutesInHour;
	i /= kMinutesInHour;
	tm->tm_hour = (i + time_zone) % kHoursInDay;
	tm->tm_mday = (i + time_zone) / kHoursInDay;
	int a = tm->tm_mday + kDaysFromUnixTime;
	int b = (a * 4 + 3) / kMagicUnkonwnFirst;
	int c = (-b*kMagicUnkonwnFirst) / 4 + a;
	int d = ((c * 4 + 3) / kMagicUnkonwnSec);
	int e = -d * kMagicUnkonwnSec;
	e = e / 4 + c;
	int m = (5 * e + 2) / kDaysFromYear;
	tm->tm_mday = -(kDaysFromYear * m + 2) / 5 + e + 1;
	tm->tm_mon = (-m / 10) * 12 + m + 2;
	tm->tm_year = b * 100 + d - 6700 + (m / 10);
}

time_t Helper::FastTmToUnixtime(const tm* tp)
{
	//http://blog.csdn.net/ok2222991/article/details/21019977

	if (!tp)
	{
		return 0;
	}
	auto year = tp->tm_year + 1900;
	auto mon = tp->tm_mon + 1;
	if (0 >= (int)(mon -= 2))
	{
		mon += 12;
		year -= 1;
	}

	return ((((year - 1) * 365 + year / 4 - year / 100 + year / 400 + 367 * mon / 12 - 30 + 59 + tp->tm_mday - 1 - 719162) * 24 + tp->tm_hour) * 60 + tp->tm_min) * 60 + tp->tm_sec - tp->tm_gmtoff;
}

time_t Helper::GetDateUnixTime(const int day_offset /* = 0 */)
{
	tm tm;
	time_t ts = time(NULL);
	FastUnixtimeToTm(ts, &tm);
	tm.tm_hour = 0;
	tm.tm_min = 0;
	tm.tm_sec = 0;
	tm.tm_gmtoff = 28800;
	return FastTmToUnixtime(&tm) + 86400 * day_offset;
}