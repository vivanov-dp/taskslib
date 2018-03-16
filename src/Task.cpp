#include "Task.h"

#include <mutex>
#include <memory>
#include <string>
#include <chrono>

namespace TasksLib {

	Task::Task() 
		: priority_(0)
		, isBlocking_(false)
		, isMainThread_(false)
		, status_(TASK_INIT)
		, executable_(nullptr)
		, suspendTime_(0)
	{
		ResetReschedule_();
	}
	Task::~Task() {}

	const uint32_t Task::GetPriority() const {
		return priority_;
	}
	const bool Task::IsBlocking() const {
		return isBlocking_;
	}
	const bool Task::IsMainThread() const {
		return isMainThread_;
	}
	const TaskStatus Task::GetStatus() const {
		return status_;
	}
	TaskExecutable const& Task::GetExecutable() const {
		return executable_;
	}
	const std::chrono::milliseconds Task::GetSuspendTime() const {
		return suspendTime_;
	}

	void Task::Execute(TasksQueue* queue, std::shared_ptr<Task> task) {
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

	void Task::ResetReschedule_() {
		doReschedule_ = false;
		reschedulePriority_ = priority_;
		rescheduleBlocking_ = isBlocking_;
		rescheduleMainThread_ = false;
		rescheduleExecutable_ = executable_;
		rescheduleSuspendTime_ = std::chrono::milliseconds(0);
	}
	void Task::ResetReschedule_(std::unique_lock<std::mutex> lock) {
		ResetReschedule_();
	}
	// This is called by the queue to copy the reschedule options to the task, locking handled from outside
	void Task::ApplyReschedule_()
	{
		priority_ = reschedulePriority_;
		isBlocking_ = rescheduleBlocking_;
		isMainThread_ = rescheduleMainThread_;
		executable_ = rescheduleExecutable_;
		suspendTime_ = rescheduleSuspendTime_;
	}

	void Task::RescheduleOption_(const TaskBlocking& isBlocking)
	{
		rescheduleBlocking_ = isBlocking;
	}
	void Task::RescheduleOption_(const TaskBlocking&& isBlocking)
	{
		rescheduleBlocking_ = isBlocking;
	}
	void Task::RescheduleOption_(const TaskThreadTarget&& thread)
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
