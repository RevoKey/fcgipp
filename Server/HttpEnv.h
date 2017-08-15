#pragma once

#include "fcgiapp.h"
#include "jsoncpp/json/json.h"
#include "MultipartHandler.h"

#include <array>
#include <ctime>
#include <map>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

class HttpEnv
{
public:
	HttpEnv();
	~HttpEnv();

	void ParseHeader(char** envp);
	void ParsePostBuffer(FCGX_Stream *in);

	friend class MultipartHandler;

	enum class RequestMethod
	{
		ERROR = 0,
		HEAD = 1,
		GET = 2,
		POST = 3,
		PUT = 4,
		DELETE = 5,
		TRACE = 6,
		OPTIONS = 7,
		CONNECT = 8
	};

	struct File
	{
		//! Filename
		std::string name;

		//! Content Type
		std::string contentType;

		//! Filename ext
		std::string ext;

		//! File path
		std::string path;

		//! Size of file
		size_t size;

		//! Move constructor
		File(MultipartHandler::MultipartData&& x) :
			name(std::move(x.fileName)),
			contentType(std::move(x.fileType)),
			ext(std::move(x.fileExt)),
			path(std::move(x.fileTempPath)),
			size(x.fileSize)
		{}

		File() {}
	};

	//! Hostname of the server
	std::string host;

	//! User agent string
	std::string userAgent;

	//! Content types the client accepts
	std::string acceptContentTypes;

	//! Languages the client accepts
	std::vector<std::string> acceptLanguages;

	//! Character sets the clients accepts
	std::string acceptCharsets;

	//! Http authorization string
	std::string authorization;

	//! Referral URL
	std::string referer;

	//! Content type of data sent from client
	std::string contentType;

	//! HTTP root directory
	std::string root;

	//! Filename of script relative to the HTTP root directory
	std::string scriptName;

	//! REQUEST_METHOD
	RequestMethod requestMethod;

	//! REQUEST_URI
	std::string requestUri;

	//! Path information
	std::vector<std::string> pathInfo;

	//! The etag the client assumes this document should have
	unsigned int etag;

	//! How many seconds the connection should be kept alive
	unsigned int keepAlive;

	//! Length of content to be received from the client (post data)
	unsigned int contentLength;

	//! IP address of the server
	std::string serverAddress;

	//! IP address of the client
	std::string remoteAddress;

	//! TCP port used by the server
	uint16_t serverPort;

	//! TCP port used by the client
	uint16_t remotePort;

	//! Timestamp the client has for this document
	std::time_t ifModifiedSince;

	//! Container of Json POST data
	Json::Value jsons;

	//! Container with all url-encoded cookie data
	std::multimap<
		std::string,
		std::string> cookies;

	//! Container with all url-encoded GET data
	std::multimap<
		std::string,
		std::string> gets;

	//! Container of none-file POST data
	std::multimap<
		std::string,
		std::string> posts;

	//! Container of file POST data
	std::multimap<
		std::string,
		File> files;
private:
	void parseMultipart(FCGX_Stream *in);
	void parseUrlEncoded(FCGX_Stream *in);
	void parseJson(FCGX_Stream *in);
	char* percentEscapedToRealBytes(
		const char* start,
		const char* end,
		char* destination);
	void decodeUrlEncoded(
		const char* data,
		const char* dataEnd,
		std::multimap<std::string, std::string> &output,
		const char* const fieldSeparator = "&");
	inline void vecToString(
		const char* start,
		const char* end,
		std::string& string)
	{
		string.assign(start, end);
	}

	//! Raw string of characters representing the post boundary
	std::string boundary;
};