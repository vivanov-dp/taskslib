#include "TaskOptions.h"

namespace TasksLib {

	TaskOptions::TaskOptions() 
		: priority(0)
		, isBlocking(false)
		, isMainThread(false)
		, executable(nullptr)
		, suspendTime(0)
	{
	}

	void TaskOptions::SetOption_(const TaskPriority& _priority) {
		priority = _priority;
	}
	void TaskOptions::SetOption_(const TaskBlocking& _isBlocking) {
		isBlocking = _isBlocking;
	}
	void TaskOptions::SetOption_(const TaskThreadTarget& _threadTarget) {
		isMainThread = (_threadTarget == MAIN_THREAD);
	}
	void TaskOptions::SetOption_(const TaskExecutable& _executable) {
		executable = _executable;
	}
	void TaskOptions::SetOption_(const TaskExecutable&& _executable) {
		executable = std::move(_executable);
	}
	void TaskOptions::SetOption_(const std::chrono::milliseconds& _ms) {
		suspendTime = _ms;
	}

}
