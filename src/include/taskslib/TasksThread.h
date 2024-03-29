#pragma once

#include <thread>

namespace TasksLib {

	class TasksThread : public std::thread {
	public:
		template <class F, class... Args> TasksThread(const bool ignoreBlocking, F&& f, Args&&... args)
			: std::thread(std::forward<F>(f), std::forward<Args>(args)...)
			, _ignoreBlocking(ignoreBlocking)
		{}

	private:
        bool _ignoreBlocking;
	};

}
