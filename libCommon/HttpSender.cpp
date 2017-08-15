#include "HttpSender.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>

#include "json/json.h"
#include "Helper.h"

using std::string;
using std::size_t;

size_t HttpSender::CurlWriter(void *buffer, size_t size, size_t count, void *stream) {
	string *pstream = static_cast<string *>(stream);
	(*pstream).append((char *)buffer, size * count);
	return size * count;
}

string HttpSender::ErrorResponse() {
	return "HttpSender network error";
}

CURL *HttpSender::CurlEasyHandler(const string &url, string *rsp,
	bool is_post, const HttpSenderOption& option) {
	CURL *easy_curl = curl_easy_init();

	curl_easy_setopt(easy_curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(easy_curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(easy_curl, CURLOPT_TIMEOUT_MS, option.timeout_for_entire_request_in_ms);
	curl_easy_setopt(easy_curl, CURLOPT_CONNECTTIMEOUT_MS, option.timeout_in_ms);
	curl_easy_setopt(easy_curl, CURLOPT_SSL_VERIFYHOST, 0);
	curl_easy_setopt(easy_curl, CURLOPT_SSL_VERIFYPEER, 1);

	if (is_post) {
		curl_easy_setopt(easy_curl, CURLOPT_POST, 1);
	}

	curl_easy_setopt(easy_curl, CURLOPT_WRITEFUNCTION, CurlWriter);
	curl_easy_setopt(easy_curl, CURLOPT_WRITEDATA, rsp);

	return easy_curl;
}

struct curl_slist* HttpSender::SetCurlHeaders(CURL *curl_handler, const std::map<string, string> &user_headers) {
	struct curl_slist *header_lists = NULL;

	std::map<string, string>::const_iterator it = user_headers.begin();
	string header_key, header_value, full_str;
	for (; it != user_headers.end(); ++it) {
		header_key = it->first;
		header_value = it->second;
		full_str = header_key + ": " + header_value;
		header_lists = curl_slist_append(header_lists, full_str.c_str());
	}
	curl_easy_setopt(curl_handler, CURLOPT_HTTPHEADER, header_lists);
	return header_lists;
}

string HttpSender::SendGetRequest(const std::string& url, const HttpSenderOption& option)
{
	string response = "";
	CURL* get_curl = CurlEasyHandler(url, &response, false, option);

	CURLcode ret_code = curl_easy_perform(get_curl);

	curl_easy_cleanup(get_curl);

	if (ret_code != CURLE_OK) {
		std::cerr << "SendGetRequest error! full_url:" << url << std::endl;
		return ErrorResponse();
	}
	return response;
}

string HttpSender::SendGetRequest(const string& url,
	const std::map<string, string> &user_headers,
	const std::map<string, string> &user_params,
	const HttpSenderOption& option) {
	string user_params_str = "";
	string param_key = "";
	string param_value = "";
	std::map<string, string>::const_iterator it = user_params.begin();
	for (; it != user_params.end(); ++it) {
		if (!user_params_str.empty()) {
			user_params_str += '&';
		}
		param_key = it->first;
		param_value = it->second;
		user_params_str += param_key + "=" + param_value;
	}

	string full_url(url);
	if (full_url.find('?') == string::npos) {
		full_url += "?" + user_params_str;
	}
	else {
		full_url += "&" + user_params_str;
	}

	string response = "";
	CURL* get_curl = CurlEasyHandler(full_url, &response, false, option);
	curl_slist* header_lists = SetCurlHeaders(get_curl, user_headers);

	CURLcode ret_code = curl_easy_perform(get_curl);

	curl_slist_free_all(header_lists);
	curl_easy_cleanup(get_curl);

	if (ret_code != CURLE_OK) {
		std::cerr << "SendGetRequest error! full_url:" << full_url << std::endl;
		return ErrorResponse();
	}
	return response;
}

string HttpSender::SendJsonPostRequest(const std::string& url,
									   const std::map<string, string> &user_headers,
									   const std::map<string, string> &user_params,
									   const HttpSenderOption& option) {
	string response = "";
	CURL* json_post_curl = CurlEasyHandler(url, &response, true, option);

	std::map<string, string> user_headers_cp = user_headers;
	user_headers_cp["Content-Type"] = "application/json";
	curl_slist* header_lists = SetCurlHeaders(json_post_curl, user_headers_cp);

	Json::Value param_json;
	std::map<string, string>::const_iterator it = user_params.begin();
	for (; it != user_params.end(); ++it) {
		param_json[it->first] = it->second;
	}
	Json::FastWriter json_writer;
	string param_str = json_writer.write(param_json);
	curl_easy_setopt(json_post_curl, CURLOPT_POSTFIELDS, param_str.c_str());

	CURLcode ret_code = curl_easy_perform(json_post_curl);

	curl_slist_free_all(header_lists);
	curl_easy_cleanup(json_post_curl);

	if (ret_code != CURLE_OK) {
		std::cerr << "SendJsonPostRequest error! url:" << url << std::endl;
		return ErrorResponse();
	}

	return response;
}

string HttpSender::SendJsonPostRequest(const std::string& url, 
									   const std::map<std::string, std::string>& user_headers, 
									   const Json::Value& user_params, 
									   const HttpSenderOption& option) {
	string response = "";
	CURL* json_post_curl = CurlEasyHandler(url, &response, true, option);

	std::map<string, string> user_headers_cp = user_headers;
	user_headers_cp["Content-Type"] = "application/json";
	curl_slist* header_lists = SetCurlHeaders(json_post_curl, user_headers_cp);

	Json::FastWriter json_writer;
	string param_str = json_writer.write(user_params);
	curl_easy_setopt(json_post_curl, CURLOPT_POSTFIELDS, param_str.c_str());

	CURLcode ret_code = curl_easy_perform(json_post_curl);

	curl_slist_free_all(header_lists);
	curl_easy_cleanup(json_post_curl);

	if (ret_code != CURLE_OK) {
		std::cerr << "SendJsonPostRequest error! url:" << url << std::endl;
		return ErrorResponse();
	}

	return response;
}

string HttpSender::SendUrlencodedPostRequest(const std::string& url, 
											 const std::map<std::string, std::string>& user_headers, 
											 const std::map<std::string, std::string>& user_params, 
											 const HttpSenderOption& option) {
	string response = "";
	CURL* post_curl = CurlEasyHandler(url, &response, true, option);

	std::map<string, string> user_headers_cp = user_headers;
	user_headers_cp["Content-Type"] = "application/x-www-form-urlencoded";
	curl_slist* header_lists = SetCurlHeaders(post_curl, user_headers_cp);

	string user_params_str = "";
	string param_key = "";
	string param_value = "";
	std::map<string, string>::const_iterator it = user_params.begin();
	for (; it != user_params.end(); ++it) {
		if (!user_params_str.empty()) {
			user_params_str += '&';
		}
		param_key = it->first;
		param_value = it->second;
		user_params_str += Helper::urlEncode(param_key) + "=" + Helper::urlEncode(param_value);
	}
	
	curl_easy_setopt(post_curl, CURLOPT_POSTFIELDS, user_params_str.c_str());

	CURLcode ret_code = curl_easy_perform(post_curl);

	curl_slist_free_all(header_lists);
	curl_easy_cleanup(post_curl);

	if (ret_code != CURLE_OK) {
		std::cerr << "SendJsonPostRequest error! url:" << url << std::endl;
		return ErrorResponse();
	}

	return response;
}

string HttpSender::SendXmlPostRequest(const std::string& url,
	const std::map<std::string, std::string>& user_headers,
	const std::string strXml,
	const HttpSenderOption& option) {
	string response = "";
	CURL* post_curl = CurlEasyHandler(url, &response, true, option);

	std::map<string, string> user_headers_cp = user_headers;
	user_headers_cp["Content-Type"] = "application/xml";
	curl_slist* header_lists = SetCurlHeaders(post_curl, user_headers_cp);

	curl_easy_setopt(post_curl, CURLOPT_POSTFIELDS, strXml.c_str());

	CURLcode ret_code = curl_easy_perform(post_curl);

	curl_slist_free_all(header_lists);
	curl_easy_cleanup(post_curl);

	if (ret_code != CURLE_OK) {
		std::cerr << "SendXmlPostRequest error! url:" << url << std::endl;
		return ErrorResponse();
	}

	return response;
}

string HttpSender::SendInkeNtyJsonPostRequest(const string& url,
	const std::map<string, string> &user_headers,
	const std::map<string, string> &user_params,
	const HttpSenderOption& option) {
	string response = "";
	CURL* json_post_curl = CurlEasyHandler(url, &response, true, option);

	std::map<string, string> user_headers_cp = user_headers;

	Json::Value param_json;
	std::map<string, string>::const_iterator it = user_params.begin();
	for (; it != user_params.end(); ++it) {
		param_json[it->first] = it->second;
	}
	Json::FastWriter json_writer;
	string param_str = json_writer.write(param_json);
	curl_easy_setopt(json_post_curl, CURLOPT_POSTFIELDS, param_str.c_str());

	CURLcode ret_code = curl_easy_perform(json_post_curl);

	curl_easy_cleanup(json_post_curl);

	if (ret_code != CURLE_OK) {
		std::cerr << "SendJsonPostRequest error! url:" << url << std::endl;
		return ErrorResponse();
	}

	return response;
}