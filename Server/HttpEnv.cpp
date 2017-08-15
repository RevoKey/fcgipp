#include "HttpEnv.h"
#include "logging.h"

#include <algorithm>
#include <cstring>
#include <stdio.h>

HttpEnv::HttpEnv()
{
}

HttpEnv::~HttpEnv()
{
	for (auto& file : files)
	{
		remove(file.second.path.c_str());
	}
}

char* HttpEnv::percentEscapedToRealBytes(
	const char* start,
	const char* end,
	char* destination)
{
	enum State
	{
		NORMAL,
		DECODINGFIRST,
		DECODINGSECOND,
	} state = NORMAL;

	while (start != end)
	{
		if (state == NORMAL)
		{
			if (*start == '%')
			{
				*destination = 0;
				state = DECODINGFIRST;
			}
			else if (*start == '+')
				*destination++ = ' ';
			else
				*destination++ = *start;
		}
		else if (state == DECODINGFIRST)
		{
			if ((*start | 0x20) >= 'a' && (*start | 0x20) <= 'f')
				*destination = ((*start | 0x20) - 0x57) << 4;
			else if (*start >= '0' && *start <= '9')
				*destination = (*start & 0x0f) << 4;

			state = DECODINGSECOND;
		}
		else if (state == DECODINGSECOND)
		{
			if ((*start | 0x20) >= 'a' && (*start | 0x20) <= 'f')
				*destination |= (*start | 0x20) - 0x57;
			else if (*start >= '0' && *start <= '9')
				*destination |= *start & 0x0f;

			++destination;
			state = NORMAL;
		}
		++start;
	}
	return destination;
}

void HttpEnv::decodeUrlEncoded(
	const char* data,
	const char* const dataEnd,
	std::multimap<std::string, std::string> &output,
	const char* const fieldSeparator)
{
	std::unique_ptr<char[]> buffer(new char[dataEnd - data]);
	std::string name;
	std::string value;

	const size_t fieldSeparatorSize = std::strlen(fieldSeparator);
	const char* const fieldSeparatorEnd = fieldSeparator + fieldSeparatorSize;

	enum EndState
	{
		NONE,
		END,
		SEPARATOR
	};
	EndState endState;

	auto nameStart(data);
	auto nameEnd(dataEnd);
	auto valueStart(dataEnd);
	auto valueEnd(dataEnd);

	while (data < dataEnd)
	{
		if (nameEnd != dataEnd)
		{
			if (data + 1 == dataEnd)
			{
				endState = END;
				valueEnd = data + 1;
			}
			else if (data + fieldSeparatorSize <= dataEnd
				&& std::equal(fieldSeparator, fieldSeparatorEnd, data))
			{
				endState = SEPARATOR;
				valueEnd = data;
			}
			else
				endState = NONE;

			if (endState != NONE)
			{
				valueEnd = percentEscapedToRealBytes(
					valueStart,
					valueEnd,
					buffer.get());
				vecToString(buffer.get(), valueEnd, value);

				output.insert(std::make_pair(
					std::move(name),
					std::move(value)));

				nameStart = data + fieldSeparatorSize;
				data += fieldSeparatorSize - 1;
				nameEnd = dataEnd;
				valueStart = dataEnd;
				valueEnd = dataEnd;
			}
		}
		else
		{
			if (*data == '=')
			{
				nameEnd = percentEscapedToRealBytes(
					nameStart,
					data,
					buffer.get());
				vecToString(buffer.get(), nameEnd, name);
				valueStart = data + 1;
			}
		}
		++data;
	}
}

void HttpEnv::ParseHeader(char** envp)
{
	const char* name;
	const char* nameEnd;
	const char* value;
	const char* end;
	int valueLen;
	for (auto env = envp; *env; ++env)
	{
		name = *env;
		nameEnd = strstr(name, "=");
		value = nameEnd + 1;
		if (!value)
		{
			continue;
		}
		end = value + std::strlen(value);
		if (end - value < 1)
		{
			continue;
		}
		switch (nameEnd - name)
		{
		case 9:
			if (std::equal(name, nameEnd, "HTTP_HOST"))
			{
				host.assign(value);
			}
			else if (std::equal(name, nameEnd, "PATH_INFO"))
			{
				const size_t bufferSize = end - value;
				std::unique_ptr<char[]> buffer(new char[bufferSize]);
				int size = -1;
				for (auto source = value; source <= end; ++source, ++size)
				{
					if (*source == '/' || source == end)
					{
						if (size > 0)
						{
							const auto bufferEnd = percentEscapedToRealBytes(
								source - size,
								source,
								buffer.get());
							pathInfo.push_back(std::string());
							vecToString(
								buffer.get(),
								bufferEnd,
								pathInfo.back());
						}
						size = -1;
					}
				}
			}
			break;
		case 11:
			if (std::equal(name, nameEnd, "HTTP_ACCEPT"))
				vecToString(value, end, acceptContentTypes);
			else if (std::equal(name, nameEnd, "HTTP_COOKIE"))
				decodeUrlEncoded(value, end, cookies, "; ");
			else if (std::equal(name, nameEnd, "SERVER_ADDR"))
				serverAddress.assign(value, end);
			else if (std::equal(name, nameEnd, "REMOTE_ADDR"))
				remoteAddress.assign(value, end);
			else if (std::equal(name, nameEnd, "SERVER_PORT"))
				serverPort = atoi(value);
			else if (std::equal(name, nameEnd, "REMOTE_PORT"))
				remotePort = atoi(value);
			else if (std::equal(name, nameEnd, "SCRIPT_NAME"))
				vecToString(value, end, scriptName);
			else if (std::equal(name, nameEnd, "REQUEST_URI"))
				vecToString(value, end, requestUri);
			break;
		case 12:
			if (std::equal(name, nameEnd, "HTTP_REFERER"))
				vecToString(value, end, referer);
			else if (std::equal(name, nameEnd, "CONTENT_TYPE"))
			{
				const auto semicolon = std::find(value, end, ';');
				vecToString(
					value,
					semicolon,
					contentType);
				if (semicolon != end)
				{
					const auto equals = std::find(semicolon, end, '=');
					if (equals != end)
						boundary.assign(
						equals + 1,
						end);
				}
			}
			else if (std::equal(name, nameEnd, "QUERY_STRING"))
				decodeUrlEncoded(value, end, gets);
			break;
		case 13:
			if (std::equal(name, nameEnd, "DOCUMENT_ROOT"))
				vecToString(value, end, root);
			break;
		case 14:
			if (std::equal(name, nameEnd, "REQUEST_METHOD"))
			{
				requestMethod = RequestMethod::ERROR;
				switch (end - value)
				{
				case 3:
					if (std::equal(value, end, "GET"))
						requestMethod = RequestMethod::GET;
					else if (std::equal(value, end, "PUT"))
						requestMethod = RequestMethod::PUT;
					break;
				case 4:
					if (std::equal(value, end, "HEAD"))
						requestMethod = RequestMethod::HEAD;
					else if (std::equal(value, end, "POST"))
						requestMethod = RequestMethod::POST;
					break;
				case 5:
					if (std::equal(value, end, "TRACE"))
						requestMethod = RequestMethod::TRACE;
					break;
				case 6:
					if (std::equal(value, end, "DELETE"))
						requestMethod = RequestMethod::DELETE;
					break;
				case 7:
					if (std::equal(value, end, "OPTIONS"))
						requestMethod = RequestMethod::OPTIONS;
					else if (std::equal(value, end, "CONNECT"))
						requestMethod = RequestMethod::CONNECT;
					break;
				}
			}
			else if (std::equal(name, nameEnd, "CONTENT_LENGTH"))
				contentLength = atoi(value);
			break;
		case 15:
			if (std::equal(name, nameEnd, "HTTP_USER_AGENT"))
				vecToString(value, end, userAgent);
			else if (std::equal(name, nameEnd, "HTTP_KEEP_ALIVE"))
				keepAlive = atoi(value);
			break;
		case 18:
			if (std::equal(name, nameEnd, "HTTP_IF_NONE_MATCH"))
				etag = atoi(value);
			else if (std::equal(name, nameEnd, "HTTP_AUTHORIZATION"))
				vecToString(value, end, authorization);
			break;
		case 19:
			if (std::equal(name, nameEnd, "HTTP_ACCEPT_CHARSET"))
				vecToString(value, end, acceptCharsets);
			break;
		case 20:
			if (std::equal(name, nameEnd, "HTTP_ACCEPT_LANGUAGE"))
			{
				const char* groupStart = value;
				const char* groupEnd;
				const char* subStart;
				const char* subEnd;
				size_t dash;
				while (groupStart < end)
				{
					acceptLanguages.push_back(std::string());
					std::string& language = acceptLanguages.back();

					groupEnd = std::find(groupStart, end, ',');

					// Setup the locality
					subEnd = std::find(groupStart, groupEnd, ';');
					subStart = groupStart;
					while (subStart != subEnd && *subStart == ' ')
						++subStart;
					while (subEnd != subStart && *(subEnd - 1) == ' ')
						--subEnd;
					vecToString(subStart, subEnd, language);

					dash = language.find('-');
					if (dash != std::string::npos)
						language[dash] = '_';

					groupStart = groupEnd + 1;
				}
			}
			break;
		case 22:
			if (std::equal(name, nameEnd, "HTTP_IF_MODIFIED_SINCE"))
			{
				std::tm time;
				std::fill(
					reinterpret_cast<char*>(&time),
					reinterpret_cast<char*>(&time) + sizeof(time),
					0);
				std::stringstream dateStream;
				dateStream.write(&*value, end - value);
				ifModifiedSince = std::mktime(&time) - timezone;
			}
			break;
		}
	}
}

void HttpEnv::ParsePostBuffer(FCGX_Stream *in)
{
	static const std::string multipartStr("multipart/form-data");
	static const std::string urlEncodedStr("application/x-www-form-urlencoded");
	static const std::string jsonStr("application/json");

	if (!in)
	{
		return;
	}

	if (jsonStr == contentType)
	{
		parseJson(in);
	}
	else if (multipartStr == contentType)
	{
		parseMultipart(in);
	}
	else if (urlEncodedStr == contentType)
	{
		parseUrlEncoded(in);
	}
}

void HttpEnv::parseJson(FCGX_Stream *in)
{
	std::unique_ptr<char> buf(new char[contentLength]);
	auto nGet = FCGX_GetStr(buf.get(), contentLength, in);
	Json::Reader reader;
	reader.parse(buf.get(), jsons);
}

void HttpEnv::parseMultipart(FCGX_Stream *in)
{
	MultipartHandler parser;
	if (parser.DoParse(this, in))
	{
		for (auto& data : parser.m_mapData)
		{
			switch (data.second.type)
			{
			case 0:
				posts.insert(std::make_pair(
					std::move(data.first),
					std::move(data.second.value)));
				break;
			case 1:
				files.insert(std::make_pair(
					std::move(data.first),
					std::move(data.second)));
				break;
			default:
				break;
			}
		}
	}
}

void HttpEnv::parseUrlEncoded(FCGX_Stream *in)
{
	std::unique_ptr<char> buf(new char[contentLength]);
	auto nGet = FCGX_GetStr(buf.get(), contentLength, in);
	decodeUrlEncoded(buf.get(), buf.get() + nGet, posts);
}