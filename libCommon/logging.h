#pragma once

#include <sstream>
#include <string>

namespace common {

	enum LogLevel {
		DEBUG = 2,
		INFO = 4,
		WARN = 8,
		ERROR = 16,
		FATAL = 32,
	};

	void StartLog(int level = INFO);
	void SetLogLevel(int level);
	void SetLogLevel(const std::string& level);
	bool SetLogFile(const char* path, bool append = true);
	bool SetWarningFile(const char* path, bool append = true);
	bool SetLogSize(int size); // in MB
	bool SetLogCount(int count);
	bool SetLogSizeLimit(int size); // in MB

	void Log(int level, const char* fmt, ...);

	class LogStream {
	public:
		LogStream(int level);
		template<class T>
		LogStream& operator<<(const T& t) {
			oss_ << t;
			return *this;
		}
		~LogStream();
	private:
		int level_;
		std::ostringstream oss_;
	};

} // namespace common

using common::DEBUG;
using common::INFO;
using common::WARN;
using common::ERROR;
using common::FATAL;

#define LOG(level, fmt, args...) ::common::Log(level, "[%s:%d] " fmt, __FILE__, __LINE__, ##args)
#define LOGS(level) ::common::LogStream(level)