#include <mutex>
#include <memory>
#include <string>
#include <chrono>
#include <utility>

#include "Task.h"

namespace TasksLib {

	Task::Task() 
		: _status(TASK_INIT)
		, _doReschedule(false)
	{}
	Task::~Task() = default;

    [[maybe_unused]] TaskStatus Task::GetStatus() const {
		return _status;
	}
	TaskOptions const& Task::GetOptions() const {
		return _options;
	}
	TaskOptions const& Task::GetRescheduleOptions() const {
		return _rescheduleOptions;
	}
    [[maybe_unused]] bool Task::WillReschedule() const {
		return _doReschedule;
	}

    template <> void Task::Reschedule() {
        _doReschedule = true;
    }

	std::mutex& Task::GetTaskMutex_() {
		return _taskMutex;
	}
	void Task::Execute(TasksQueue* queue, TaskPtr task) {
		// Here *task == *this, but it is a shared_ptr supplied by the queue and holds a stake 
		// at the point of creation of the task, which ensures that even if everything gets released,
		// the task will still exist until it finishes

		{
			std::unique_lock<std::mutex> lock(_taskMutex);
			if (_options.executable) {
                _status = TASK_WORKING;
				ResetReschedule_();
                // Unlock the task while executing
                lock.release();
				_options.executable(queue, std::move(task));
			}
		}

		{
			std::lock_guard<std::mutex> lock(_taskMutex);
			if (!_doReschedule) {
                _status = TASK_FINISHED;
			}
		}
	}
	
	void Task::ResetReschedule_() {
        _doReschedule = false;
        _rescheduleOptions = _options;
	}
	/* This is called by the queue to copy the reschedule options to the task, locking handled from outside */
	void Task::ApplyReschedule_() {
        _options = _rescheduleOptions;
	}
}
