#pragma once

#include <memory>
#include <functional>

namespace TasksLib {

	enum TaskThreadTarget {
		MAIN_THREAD,
		WORKER_THREAD
	};
	enum TaskStatus {
		TASK_FINISHED = 0,

		TASK_INIT = 1,			// Freshly created, not added to queue
		TASK_SUSPENDED,			// Put on hold, will reschedule after a specified delay
		TASK_IN_QUEUE,			// Waiting in a queue
		TASK_WORKING,			// Executing
	};

	using TaskBlocking = bool;
	using TaskPriority = uint32_t;

	class TasksQueue;
	class Task;

	using TaskPtr			= std::shared_ptr<Task>;
	using TaskUniquePtr		= std::unique_ptr<Task>;
	using TaskWeakPtr		= std::weak_ptr<Task>;
	using TaskExecutable	= std::function<void(TasksQueue* queue, TaskPtr task)>;

}
