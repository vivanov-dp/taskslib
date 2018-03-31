#include <mutex>
#include <memory>
#include <string>
#include <chrono>

#include "Task.h"

namespace TasksLib {

	Task::Task() 
		: status_(TASK_INIT)
		, doReschedule_(false)
	{}
	Task::~Task() {}

	const TaskStatus Task::GetStatus() const {
		return status_;
	}
	TaskOptions const& Task::GetOptions() const {
		return options_;
	}
	TaskOptions const& Task::GetRescheduleOptions() const {
		return rescheduleOptions_;
	}
	const bool Task::WillReschedule() const {
		return doReschedule_;
	}

	std::mutex& Task::GetTaskMutex_() {
		return taskMutex_;
	}

	/* This is called by the queue to reset the reschedule options before running the task */
	void Task::ResetReschedule_(std::unique_lock<std::mutex> lock) {
		ResetReschedule_();
	}
	void Task::ApplyReschedule_(std::unique_lock<std::mutex> lock) {
		ApplyReschedule_();
	}
	void Task::Execute(TasksQueue* queue, TaskPtr task) {
		// Here *task == *this, but it is a shared_ptr supplied by the queue and holds a stake 
		// at the point of creation of the task, which ensures that even if everything gets released,
		// the task will still exists until it finishes

		{
			std::unique_lock<std::mutex> lock(taskMutex_);
			if (options_.executable) {
				status_ = TASK_WORKING;
				ResetReschedule_(std::move(lock));			// This releases the lock
				options_.executable(queue, task);
			}
		}

		{
			std::lock_guard<std::mutex> lock(taskMutex_);
			if (!doReschedule_) {
				status_ = TASK_FINISHED;
			}
		}
	}
	
	void Task::ResetReschedule_() {
		doReschedule_ = false;
		rescheduleOptions_ = options_;
	}
	/* This is called by the queue to copy the reschedule options to the task, locking handled from outside */
	void Task::ApplyReschedule_() {
		options_ = rescheduleOptions_;
	}
}
