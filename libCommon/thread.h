#pragma once

#include <pthread.h>
#include <string.h>
#include <functional>

namespace common {

	class Thread {
	public:
		Thread() {
			memset(&tid_, 0, sizeof(tid_));
		}
		bool Start(std::function<void()> thread_proc) {
			user_proc_ = thread_proc;
			int ret = pthread_create(&tid_, NULL, ProcWrapper, this);
			return (ret == 0);
		}
		typedef void* (Proc)(void*);
		bool Start(Proc proc, void* arg) {
			int ret = pthread_create(&tid_, NULL, proc, arg);
			return (ret == 0);
		}
		bool Join() {
			int ret = pthread_join(tid_, NULL);
			return (ret == 0);
		}
	private:
		Thread(const Thread&);
		void operator=(const Thread&);
		static void* ProcWrapper(void* arg) {
			reinterpret_cast<Thread*>(arg)->user_proc_();
			return NULL;
		}
	private:
		std::function<void()> user_proc_;
		pthread_t tid_;
	};

}
