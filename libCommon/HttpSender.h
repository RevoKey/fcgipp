#pragma once

#include <string>
#include <map>
#include <stdint.h>
#include <curl/curl.h>
#include <curl/multi.h>
#include "jsoncpp/json/json.h"

#define MAX_WAIT_MSECS 60       /* Max wait timeout, 60 seconds */

struct HttpSenderOption {
	uint64_t timeout_in_ms;
	uint64_t timeout_for_entire_request_in_ms;

	HttpSenderOption()
		: timeout_in_ms(30000),
		timeout_for_entire_request_in_ms(300 * 1000){}  // default timeout 30s

	HttpSenderOption(uint64_t t_timeout_in_s,
		             uint64_t t_timeout_for_entire_request_in_ms)
		: timeout_in_ms(t_timeout_in_s),
		timeout_for_entire_request_in_ms(t_timeout_for_entire_request_in_ms){}

	HttpSenderOption(const HttpSenderOption& option) {
		timeout_in_ms = option.timeout_in_ms;
		timeout_for_entire_request_in_ms = option.timeout_for_entire_request_in_ms;
	}
};

class HttpSender {
private:
    static size_t CurlWriter(void* buffer, size_t size, size_t count, void* stream);

    static CURL *CurlEasyHandler(const std::string& url, std::string* rsp,
                                 bool is_post, const HttpSenderOption& option);

    static struct curl_slist* SetCurlHeaders(CURL* curl_handler, const std::map<std::string, std::string>& user_headers);
public:
	static std::string SendGetRequest(const std::string& url, const HttpSenderOption& option);

    static std::string SendGetRequest(const std::string& url,
                                      const std::map<std::string, std::string>& user_headers,
                                      const std::map<std::string, std::string>& user_params,
									  const HttpSenderOption& option);

	static std::string SendJsonPostRequest(const std::string& url,
										   const std::map<std::string, std::string>& user_headers,
										   const std::map<std::string, std::string>& user_params,
										   const HttpSenderOption& option);
	static std::string SendJsonPostRequest(const std::string& url,
										   const std::map<std::string, std::string>& user_headers,
										   const Json::Value& user_params,
										   const HttpSenderOption& option);
	static std::string SendInkeNtyJsonPostRequest(const std::string& url,
										   const std::map<std::string, std::string>& user_headers,
										   const std::map<std::string, std::string>& user_params,
										   const HttpSenderOption& option);

	static std::string SendUrlencodedPostRequest(const std::string& url,
												 const std::map<std::string, std::string>& user_headers,
												 const std::map<std::string, std::string>& user_params,
												 const HttpSenderOption& option);

	static std::string SendXmlPostRequest(const std::string& url,
		const std::map<std::string, std::string>& user_headers,
		const std::string strXml,
		const HttpSenderOption& option);

    static std::string ErrorResponse();
};
