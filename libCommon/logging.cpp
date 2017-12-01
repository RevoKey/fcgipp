#include "logging.h"

#include <algorithm>
#include <assert.h>
#include <functional>
#include <queue>
#include <set>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#include "mutex.h"
#include "thread.h"
#include "timer.h"

namespace common {

	int g_log_level = INFO;
	int64_t g_log_size = 0;
	int32_t g_log_count = 0;
	int g_log_fd = STDOUT_FILENO;
	std::string g_log_file_name;
	int g_warning_fd = 0;
	int64_t g_total_size_limit = 0;

	std::set<std::string> g_log_set;
	int64_t current_total_size = 0;

	bool GetNewLog(bool append) {
		char buf[16];
		struct timeval tv;
		gettimeofday(&tv, NULL);
		const time_t seconds = tv.tv_sec;
		struct tm t;
		localtime_r(&seconds, &t);
		snprintf(buf, 16,
			"%04d-%02d-%02d",
			t.tm_year + 1900,
			t.tm_mon + 1,
			t.tm_mday);
		std::string full_path(g_log_file_name + ".");
		full_path.append(buf);
		size_t idx = full_path.rfind('/');
		if (idx == std::string::npos) {
			idx = 0;
		}
		else {
			idx += 1;
		}
		int flags = append ? (O_CREAT | O_APPEND | O_WRONLY) : (O_CREAT | O_WRONLY);
		int fd = ::open(full_path.c_str(), flags, 0664);
		if (fd < 0)
		{
			return false;
		}
		if (g_log_fd != STDOUT_FILENO)
		{
			::close(g_log_fd);
		}
		g_log_fd = fd;
		remove(g_log_file_name.c_str());
		symlink(full_path.substr(idx).c_str(), g_log_file_name.c_str());
		g_log_set.insert(full_path);
		while ((g_log_count && static_cast<int64_t>(g_log_set.size()) > g_log_count)
			|| (g_total_size_limit && current_total_size > g_total_size_limit)) {
			std::set<std::string>::iterator it = g_log_set.begin();
			if (it != g_log_set.end()) {
				struct stat sta;
				if (-1 == lstat(it->c_str(), &sta)) {
					return false;
				}
				remove(it->c_str());
				current_total_size -= sta.st_size;
				g_log_set.erase(it++);
			}
			else {
				break;
			}
		}
		return true;
	}

	void SetLogLevel(int level) {
		g_log_level = level;
	}

	void SetLogLevel(const std::string& level) {
		if (level == "DEBUG")
		{
			g_log_level = DEBUG;
		}
		else if (level == "INFO")
		{
			g_log_level = INFO;
		}
		else if (level == "WARN")
		{
			g_log_level = WARN;
		}
		else if (level == "ERROR")
		{
			g_log_level = ERROR;
		}
		else if (level == "FATAL")
		{
			g_log_level = FATAL;
		}
	}

	class AsyncLogger {
	public:
		AsyncLogger()
			: jobs_(&mu_), done_(&mu_), stopped_(true), size_(0) {
			buffer_queue_ = new std::queue<std::pair<int, std::string*> >;
			bg_queue_ = new std::queue<std::pair<int, std::string*> >;
		}
		~AsyncLogger() {
			if (!stopped_)
			{
				stopped_ = true;
				{
					MutexLock lock(&mu_);
					jobs_.Signal();
				}
				thread_.Join();
				delete buffer_queue_;
				delete bg_queue_;
				// close fd
				if (g_warning_fd)
				{
					::close(g_warning_fd);
				}
				if (g_log_fd)
				{
					::close(g_log_fd);
				}
			}
		}
		void Start() {
			stopped_ = false;
			thread_.Start(std::bind(&AsyncLogger::AsyncWriter, this));
		}
		void WriteLog(int log_level, const char* buffer, int32_t len) {
			std::string* log_str = new std::string(buffer, len);
			MutexLock lock(&mu_);
			buffer_queue_->push(make_pair(log_level, log_str));
			jobs_.Signal();
		}
		void AsyncWriter() {
			int64_t loglen = 0;
			int64_t wflen = 0;
			while (1) {
				while (!bg_queue_->empty()) {
					int log_level = bg_queue_->front().first;
					std::string* str = bg_queue_->front().second;
					bg_queue_->pop();
					if (g_log_fd != STDOUT_FILENO && g_log_size && str &&
						static_cast<int64_t>(size_ + str->length()) > g_log_size) {
						current_total_size += static_cast<int64_t>(size_ + str->length());
						GetNewLog(false);
						size_ = 0;
					}
					if (str && !str->empty()) {
						size_t lret = ::write(g_log_fd, str->data(), str->size());
						loglen += lret;
						if (g_warning_fd && log_level >= LogLevel::WARN) {
							size_t wret = ::write(g_log_fd, str->data(), str->size());
							wflen += wret;
						}
						if (g_log_size) size_ += lret;
					}
					delete str;
				}
				MutexLock lock(&mu_);
				if (!buffer_queue_->empty()) {
					std::swap(buffer_queue_, bg_queue_);
					continue;
				}
				done_.Broadcast();
				if (stopped_) {
					break;
				}
				jobs_.Wait();
				loglen = 0;
				wflen = 0;
			}
		}
		void Flush() {
			MutexLock lock(&mu_);
			buffer_queue_->push(std::make_pair(0, reinterpret_cast<std::string*>(NULL)));
			jobs_.Signal();
			done_.Wait();
		}
	private:
		Mutex mu_;
		CondVar jobs_;
		CondVar done_;
		bool stopped_;
		int64_t size_;
		Thread thread_;
		std::queue<std::pair<int, std::string*>> *buffer_queue_;
		std::queue<std::pair<int, std::string*>> *bg_queue_;
	};

	AsyncLogger g_logger;

	void StartLog(int level /* = INFO */) {
		g_logger.Start();
	}

	bool SetWarningFile(const char* path, bool append)
	{
		int flags = append ? (O_CREAT | O_APPEND | O_WRONLY) : (O_CREAT | O_WRONLY);
		int fd = ::open(path, flags, 0664);
		if (fd < 0)
		{
			return false;
		}
		if (g_warning_fd != STDOUT_FILENO)
		{
			::close(g_warning_fd);
		}
		g_warning_fd = fd;
		return true;
	}

	bool RecoverHistory(const char* path) {
		std::string log_path(path);
		size_t idx = log_path.rfind('/');
		std::string dir = "./";
		std::string log(path);
		if (idx != std::string::npos) {
			dir = log_path.substr(0, idx + 1);
			log = log_path.substr(idx + 1);
		}
		struct dirent *entry = NULL;
		DIR *dir_ptr = opendir(dir.c_str());
		if (dir_ptr == NULL) {
			return false;
		}
		std::vector<std::string> loglist;
		while ((entry = readdir(dir_ptr)) != NULL) {
			if (std::string(entry->d_name).find(log) != std::string::npos) {
				std::string file_name = dir + std::string(entry->d_name);
				struct stat sta;
				if (-1 == lstat(file_name.c_str(), &sta)) {
					return false;
				}
				if (S_ISREG(sta.st_mode)) {
					loglist.push_back(dir + std::string(entry->d_name));
					current_total_size += sta.st_size;
				}
			}
		}
		closedir(dir_ptr);
		std::sort(loglist.begin(), loglist.end());
		for (std::vector<std::string>::iterator it = loglist.begin(); it != loglist.end();
			++it) {
			g_log_set.insert(*it);
		}
		while ((g_log_count && static_cast<int64_t>(g_log_set.size()) > g_log_count)
			|| (g_total_size_limit && current_total_size > g_total_size_limit)) {
			std::set<std::string>::iterator it = g_log_set.begin();
			if (it != g_log_set.end()) {
				struct stat sta;
				if (-1 == lstat(it->c_str(), &sta)) {
					return false;
				}
				remove(it->c_str());
				current_total_size -= sta.st_size;
				g_log_set.erase(it++);
			}
		}
		return true;
	}

	bool SetLogFile(const char* path, bool append) {
		g_log_file_name.assign(path);
		return GetNewLog(append);
	}

	bool SetLogSize(int size) {
		if (size < 0) {
			return false;
		}
		g_log_size = static_cast<int64_t>(size) << 20;
		return true;
	}

	bool SetLogCount(int count) {
		if (count < 0 || g_total_size_limit != 0) {
			return false;
		}
		g_log_count = count;
		if (!RecoverHistory(g_log_file_name.c_str())) {
			return false;
		}
		return true;
	}

	bool SetLogSizeLimit(int size) {
		if (size < 0 || g_log_count != 0) {
			return false;
		}
		g_total_size_limit = static_cast<int64_t>(size) << 20;
		if (!RecoverHistory(g_log_file_name.c_str())) {
			return false;
		}
		return true;
	}

	void Logv(int log_level, const char* format, va_list ap) {
		static __thread uint64_t process_id = 0;
		static __thread char pid_str[8];
		static __thread int pid_str_len = 0;
		static __thread uint64_t thread_id = 0;
		static __thread char tid_str[8];
		static __thread int tid_str_len = 0;
		if (process_id == 0) {
			process_id = syscall(__NR_getpid);
			pid_str_len = snprintf(pid_str, sizeof(pid_str), "|%d", static_cast<int32_t>(process_id));
		}
		if (thread_id == 0) {
			thread_id = syscall(__NR_gettid);
			tid_str_len = snprintf(tid_str, sizeof(tid_str), "|%d|", static_cast<int32_t>(thread_id));
		}

		int cur_level_len = 0;
		char cur_level[8] = "";
		if (log_level < DEBUG) {
			cur_level_len = snprintf(cur_level, sizeof(cur_level), "VERB");
		}
		else if (log_level < INFO) {
			cur_level_len = snprintf(cur_level, sizeof(cur_level), "DEBUG");
		}
		else if (log_level < WARN) {
			cur_level_len = snprintf(cur_level, sizeof(cur_level), "INFO");
		}
		else if (log_level < ERROR) {
			cur_level_len = snprintf(cur_level, sizeof(cur_level), "WARN");
		}
		else if (log_level < FATAL) {
			cur_level_len = snprintf(cur_level, sizeof(cur_level), "ERROR");
		}
		else {
			cur_level_len = snprintf(cur_level, sizeof(cur_level), "FATAL");
		}

		// We try twice: the first time with a fixed-size stack allocated buffer,
		// and the second time with a much larger dynamically allocated buffer.
		char buffer[500];
		for (int iter = 0; iter < 2; iter++) {
			char* base;
			int bufsize;
			if (iter == 0) {
				bufsize = sizeof(buffer);
				base = buffer;
			}
			else {
				bufsize = 30000;
				base = new char[bufsize];
			}
			char* p = base;
			char* limit = base + bufsize;

			int32_t rlen = timer::now_time_str(p, limit - p);
			p += rlen;
			*p++ = '|';
			memcpy(p, cur_level, cur_level_len);
			p += cur_level_len;
			memcpy(p, pid_str, pid_str_len);
			p += pid_str_len;
			memcpy(p, tid_str, tid_str_len);
			p += tid_str_len;

			// Print the message
			if (p < limit) {
				va_list backup_ap;
				va_copy(backup_ap, ap);
				p += vsnprintf(p, limit - p, format, backup_ap);
				va_end(backup_ap);
			}

			// Truncate to available space if necessary
			if (p >= limit) {
				if (iter == 0) {
					continue;       // Try again with larger buffer
				}
				else {
					p = limit - 1;
				}
			}

			// Add newline if necessary
			if (p == base || p[-1] != '\n') {
				*p++ = '\n';
			}

			assert(p <= limit);
			g_logger.WriteLog(log_level, base, p - base);
			if (log_level >= ERROR) {
				g_logger.Flush();
			}
			if (base != buffer) {
				delete[] base;
			}
			break;
		}
	}

	void Log(int level, const char* fmt, ...) {
		va_list ap;
		va_start(ap, fmt);
		if (level >= g_log_level) {
			Logv(level, fmt, ap);
		}
		va_end(ap);
		if (level == FATAL) {
			abort();
		}
	}

	LogStream::LogStream(int level) : level_(level) {}

	LogStream::~LogStream() {
		Log(level_, "%s", oss_.str().c_str());
	}

} // namespace common