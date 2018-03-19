#pragma once

#include <memory>
#include <functional>
#include <chrono>

namespace TasksLib {

	enum TaskStatus {
		TASK_FINISHED = 0,

		TASK_INIT = 1,			// Freshly created, not added to queue
		TASK_SUSPENDED,			// Put on hold, will reschedule after a specified delay
		TASK_IN_QUEUE,			// Waiting in a queue
		TASK_WORKING,			// Executing
	};

	class TasksQueue;
	class Task;

	using TaskPtr		= std::shared_ptr<Task>;
	using TaskUniquePtr	= std::unique_ptr<Task>;
	using TaskWeakPtr	= std::weak_ptr<Task>;

	// Types accepted as options for tasks (in Task::Task() and Task::SetOptions()) :
	enum TaskThreadTarget {
		MAIN_THREAD,
		WORKER_THREAD
	};
	using TaskBlocking		= bool;
	using TaskPriority		= uint32_t;
	using TaskExecutable	= std::function<void(TasksQueue* queue, TaskPtr task)>;
	using TaskMilliseconds	= std::chrono::milliseconds;
	// </Types as options>

}
