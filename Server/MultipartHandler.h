#pragma once

#include "MultipartParser.h"
#include "MultipartReader.h"
#include "fcgiapp.h"

#include <map>
#include <string.h>

class HttpEnv;
class MultipartHandler
{
public:
	struct MultipartData
	{
		int type;				// 0: text;  1: files
		std::string value;
		std::string fileName;
		std::string fileTempPath;
		std::string fileType;
		std::string fileExt;
		size_t fileSize;
		int fd;
		timeval tv;
		MultipartData(int type = 0)
		{
			this->type = type;
			fileSize = 0;
			fd = -1;
		}
	};

	MultipartHandler();
	~MultipartHandler();

	bool DoParse(HttpEnv* pEnv, FCGX_Stream *in, size_t buf_size = 1024 * 1024);

	std::multimap<std::string, MultipartData> m_mapData;
	int m_iErrCode;

protected:
	static std::map<std::string, std::string> SplitFormData(const std::string &form_data);

	static void OnPartBegin(const MultipartHeaders &headers, void *userData);
	static void OnPartData(const char *buffer, size_t size, void *userData);
	static void OnPartEnd(void *userData);
	static void OnEnd(void *userData);

private:
	MultipartReader m_reader;
	std::string m_formString;
	std::string m_currentKey;
};

