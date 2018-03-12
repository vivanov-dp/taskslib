#include "Task.h"

#include <mutex>
#include <memory>
#include <string>

namespace TasksLib
{
	using BlockingType = bool;

	Task::Task(const bool isBlocking, const TaskPriority priority, const bool isMainThread)
		: isBlocking_(isBlocking)
		, priority_(priority)
		, isMainThread_(isMainThread)
		, status_(TASK_INIT)
		, suspendTime_(std::chrono::milliseconds(0))
	{
	}
	Task::Task(const TaskExecutable executable, const bool isBlocking, const TaskPriority priority, const bool isMainThread)
		: Task(isBlocking, priority, isMainThread)
	{
		executable_ = executable;
	}
	Task::~Task()
	{
	}

	const uint32_t Task::GetPriority()
	{
		std::lock_guard<std::mutex> lock(dataMutex_);
		return GetPriority_();
	}
	const bool Task::IsBlocking()
	{
		std::lock_guard<std::mutex> lock(dataMutex_);
		return IsBlocking_();
	}
	const bool Task::IsMainThread()
	{
		std::lock_guard<std::mutex> lock(dataMutex_);
		return IsMainThread_();
	}

	const TaskStatus Task::GetStatus()
	{
		std::lock_guard<std::mutex> lock(dataMutex_);
		return status_;
	}

	void Task::Execute(TasksQueue* queue, std::shared_ptr<Task> task)
	{
		{
			std::unique_lock<std::mutex> lock(dataMutex_);
			if (executable_)
			{
				status_ = TASK_WORKING;
				ResetReschedule_(std::move(lock));			// This releases the lock
				executable_(queue, task);
			}
		}

		{
			std::lock_guard<std::mutex> lock2(dataMutex_);
			if (!doReschedule_)
			{
				status_ = TASK_FINISHED;
			}
		}
	}

	std::mutex& Task::GetDataMutex_()
	{
		return dataMutex_;
	}
	const uint32_t Task::GetPriority_()
	{
		return priority_;
	}
	const bool Task::IsBlocking_()
	{
		return isBlocking_;
	}
	const bool Task::IsMainThread_()
	{
		return isMainThread_;
	}

	void Task::ResetReschedule_(std::unique_lock<std::mutex> lock)
	{
		doReschedule_ = false;
		rescheduleBlocking_ = isBlocking_;
		rescheduleMainThread_ = false;
		reschedulePriority_ = priority_;
		rescheduleExecutable_ = executable_;
		rescheduleSuspendTime_ = std::chrono::milliseconds(0);
	}
	// This is called by the queue to copy the reschedule options to the task, locking handled from outside
	void Task::ApplyReschedule_()
	{
		isBlocking_ = rescheduleBlocking_;
		isMainThread_ = rescheduleMainThread_;
		priority_ = reschedulePriority_;
		executable_ = rescheduleExecutable_;
		suspendTime_ = rescheduleSuspendTime_;
	}

	void Task::RescheduleOption_(const BlockingType& isBlocking)
	{
		rescheduleBlocking_ = isBlocking;
	}
	void Task::RescheduleOption_(const BlockingType&& isBlocking)
	{
		rescheduleBlocking_ = isBlocking;
	}
	void Task::RescheduleOption_(const ThreadTarget&& thread)
	{
		rescheduleMainThread_ = (thread == MAIN_THREAD);
	}
	void Task::RescheduleOption_(const TaskPriority&& priority)
	{
		reschedulePriority_ = priority;
	}
	void Task::RescheduleOption_(const TaskExecutable executable)
	{
		rescheduleExecutable_ = executable;
	}
	void Task::RescheduleOption_(const std::chrono::milliseconds& ms)
	{
		rescheduleSuspendTime_ = ms;
	}
	void Task::RescheduleOption_(const std::chrono::milliseconds&& ms)
	{
		rescheduleSuspendTime_ = ms;
	}

}
