#include "CommonDef.h"
#include "MultipartHandler.h"
#include <errno.h>
#include "sys/time.h"
#include <vector>
#include <string>
#include <sstream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "HttpEnv.h"

#define TEMP_FILE_PATH_TEMPLATE "/tmp/MH_XXXXXX"

namespace
{
	static std::string TrimString(const std::string& str, const char c = ' ')
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

	static std::vector<std::string> SplitString(const std::string& src, const std::string& pattern, bool bTrim = false)
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
					result.push_back(bTrim ? TrimString(s) : s);
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

	static std::string GetFileExt(const std::string &file_name)
	{
		auto pos = file_name.rfind(".");
		if (std::string::npos != pos)
		{
			return file_name.substr(pos + 1, file_name.size());
		}
		return std::string();
	}
}

MultipartHandler::MultipartHandler() : m_iErrCode(ERR_OK)
{
}

MultipartHandler::~MultipartHandler()
{
}

bool MultipartHandler::DoParse(HttpEnv* pEnv, FCGX_Stream *in, size_t buf_size /* = 1024 * 1024 */)
{
	if (!pEnv || !in)
	{
		return false;
	}

	unsigned int len = pEnv->contentLength;
	if (!len)
	{
		return false;
	}

	m_reader.onPartBegin = OnPartBegin;
	m_reader.onPartData = OnPartData;
	m_reader.onPartEnd = OnPartEnd;
	m_reader.onEnd = OnEnd;
	m_reader.setBoundary(pEnv->boundary);
	m_reader.userData = this;

	if (len > buf_size)
	{
		auto buf = new char[buf_size];
		int nGet = 0;
		int nCount = 0;
		do 
		{
			memset(buf, 0x00, buf_size);
			nGet = FCGX_GetStr(buf, buf_size, in);
			size_t fed = 0;
			do
			{
				size_t ret = m_reader.feed(buf + fed, nGet - fed);
				fed += ret;
			} while (fed < nGet && !m_reader.stopped());
		} while (nGet > 0);
		delete[]buf;
	}
	else
	{
		auto buf = new char[len];
		memset(buf, 0x00, len);
		FCGX_GetStr(buf, len, in);
		size_t fed = 0;
		do 
		{
			size_t ret = m_reader.feed(buf + fed, len - fed);
			fed += ret;
		} while (fed < len && !m_reader.stopped());
		delete[]buf;
	}

	return ERR_OK == m_iErrCode;
}

std::map<std::string, std::string> MultipartHandler::SplitFormData(const std::string &form_data)
{
	std::map<std::string, std::string> ret;
	auto datas = SplitString(form_data, "; ");
	if (*datas.begin() == "form-data")
	{
		datas.erase(datas.begin());
	}
	std::string key = "";
	std::string value = "";
	for (auto it = datas.begin(); it != datas.end(); ++it)
	{
		auto kv = SplitString(*it, "=");
		if (kv.size() == 2)
		{
			ret[kv[0]] = TrimString(kv[1], '"');
		}
	}
	return ret;
}

void MultipartHandler::OnPartBegin(const MultipartHeaders &headers, void *userData) 
{
	MultipartHandler* pHandler = (MultipartHandler*)userData;
	if (pHandler)
	{
		std::string key = "";
		MultipartData data;
		for (auto it = headers.begin(); it != headers.end(); it++)
		{
			if (it->first == "Content-Disposition")
			{
				auto mapDatas = SplitFormData(it->second);
				if (mapDatas.find("name") != mapDatas.end())
				{
					key = mapDatas["name"];
				}
				if (mapDatas.find("filename") != mapDatas.end())
				{
					data.type = 1;
					data.fileName = mapDatas["filename"];
					data.fileExt = GetFileExt(data.fileName);
					char pTempPath[] = TEMP_FILE_PATH_TEMPLATE;
					data.fd = mkstemp(pTempPath);
					if (-1 != data.fd)
					{
						data.fileTempPath = pTempPath;
					}
					else
					{
						pHandler->m_iErrCode = ERR_WRITE_FILE;
					}
				}
			}
			else if (it->first == "Content-Type")
			{
				data.fileType = it->second;
			}
		}
		pHandler->m_currentKey = key;
		pHandler->m_mapData.insert(std::make_pair(key, data));
	}
}

void MultipartHandler::OnPartData(const char *buffer, size_t size, void *userData)
{
	MultipartHandler* pHandler = (MultipartHandler*)userData;
	if (pHandler && !pHandler->m_currentKey.empty())
	{
		auto it = pHandler->m_mapData.find(pHandler->m_currentKey);
		if (it != pHandler->m_mapData.end())
		{
			if (0 == it->second.type)
			{
				if (!it->second.value.empty())
				{
					it->second.value += std::string(buffer, size);
				}
				else
				{
					it->second.value = std::string(buffer, size);
				}
			}
			else if (1 == it->second.type && -1 != it->second.fd)
			{
				if (size != write(it->second.fd, buffer, size))
				{
					close(it->second.fd);
					remove(it->second.fileTempPath.c_str());
					pHandler->m_mapData.erase(it);
					pHandler->m_currentKey.clear();
					pHandler->m_iErrCode = ERR_WRITE_FILE;
				}
				else
				{
					it->second.fileSize += size;
				}
			}
		}
	}
}

void MultipartHandler::OnPartEnd(void *userData)
{
	MultipartHandler* pHandler = (MultipartHandler*)userData;
	if (pHandler)
	{
		auto it = pHandler->m_mapData.find(pHandler->m_currentKey);
		if (it != pHandler->m_mapData.end())
		{
			if (1 == it->second.type && -1 != it->second.fd)
			{
				close(it->second.fd);
				it->second.fd = -1;
				gettimeofday(&(it->second.tv), NULL);
			}
		}
		pHandler->m_currentKey.clear();
	}
}

void MultipartHandler::OnEnd(void *userData)
{
}