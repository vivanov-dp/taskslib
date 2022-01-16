#include "TaskOptions.h"

namespace TasksLib {

	TaskOptions::TaskOptions() noexcept
		: priority(0)
		, isBlocking(false)
		, isMainThread(false)
		, executable(nullptr)
		, suspendTime(0)
	{
	}
	TaskOptions::TaskOptions(const TaskOptions& other) noexcept = default;
	TaskOptions::TaskOptions(TaskOptions&& other) noexcept
        : TaskOptions()
    {
		operator=(std::move(other));
	}
    template <typename... Ts> TaskOptions::TaskOptions(Ts&& ...opts)
            : TaskOptions()
    {
        SetOptions(std::forward<Ts>(opts)...);
    }

	TaskOptions& TaskOptions::operator=(const TaskOptions& other) = default;
	TaskOptions& TaskOptions::operator=(TaskOptions&& other) noexcept {
		priority		= other.priority;
		isBlocking		= other.isBlocking;
		isMainThread	= other.isMainThread;
		executable		= std::move(other.executable);
		suspendTime		= other.suspendTime;

		return *this;
	}

	bool TaskOptions::operator==(const TaskOptions& other) const {
		return (
			(priority == other.priority)
			&& (isBlocking == other.isBlocking)
			&& (isMainThread == other.isMainThread)
			&& ((bool)executable == (bool)other.executable)
			&& (executable.target_type() == other.executable.target_type())
			&& (suspendTime == other.suspendTime)
        );
	}
	bool TaskOptions::operator!=(const TaskOptions& other) const {
		return !(operator==(other));
	}

	void TaskOptions::SetOption_(const TaskOptions& other) {
		operator=(other);
	}

    [[maybe_unused]] void TaskOptions::SetOption_(TaskOptions&& other) {
		operator=(std::move(other));
	}
    [[maybe_unused]] void TaskOptions::SetOption_(const TaskPriority& _priority) {
		priority = _priority;
	}
    [[maybe_unused]] void TaskOptions::SetOption_(const TaskBlocking& _isBlocking) {
		isBlocking = _isBlocking;
	}
    [[maybe_unused]] void TaskOptions::SetOption_(const TaskThreadTarget& _threadTarget) {
		isMainThread = (_threadTarget == MAIN_THREAD);
	}
    [[maybe_unused]] void TaskOptions::SetOption_(const TaskExecutable& _executable) {
		executable = _executable;
	}
    [[maybe_unused]] void TaskOptions::SetOption_(TaskExecutable&& _executable) {
		executable = std::move(_executable);
	}
    [[maybe_unused]] void TaskOptions::SetOption_(const TaskDelay& _ms) {
		suspendTime = _ms;
	}

}
