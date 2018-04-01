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
	TaskOptions::TaskOptions(const TaskOptions& other) {
		operator=(other);
	}
	TaskOptions::TaskOptions(TaskOptions&& other) {
		operator=(std::move(other));
	}

	TaskOptions& TaskOptions::operator=(const TaskOptions& other) {
		// Don't check for equality because the check would take as much time as the assignment

		priority		= other.priority;
		isBlocking		= other.isBlocking;
		isMainThread	= other.isMainThread;
		executable		= other.executable;
		suspendTime		= other.suspendTime;

		return *this;
	}
	TaskOptions& TaskOptions::operator=(TaskOptions&& other) {
		// Don't check for equality because the check would take as much time as the assignment

		priority		= other.priority;
		isBlocking		= other.isBlocking;
		isMainThread	= other.isMainThread;
		executable		= std::move(other.executable);
		suspendTime		= other.suspendTime;

		return *this;
	}

	const bool TaskOptions::operator==(const TaskOptions& other) const {
		return (
			(priority == other.priority)
			&& (isBlocking == other.isBlocking)
			&& (isMainThread == other.isMainThread)
			&& ((bool)executable == (bool)other.executable)
			&& (executable.target_type() == other.executable.target_type())
			&& (suspendTime == other.suspendTime)
			);
	}
	const bool TaskOptions::operator!=(const TaskOptions& other) const {
		return !(operator==(other));
	}

	void TaskOptions::SetOption_(const TaskOptions& other) {
		operator=(other);
	}
	void TaskOptions::SetOption_(const TaskOptions&& other) {
		operator=(std::move(other));
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
	void TaskOptions::SetOption_(TaskExecutable&& _executable) {
		executable = std::move(_executable);
	}
	void TaskOptions::SetOption_(const TaskDelay& _ms) {
		suspendTime = _ms;
	}

}
