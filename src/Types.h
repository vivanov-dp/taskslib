#pragma once

namespace TasksLib {

	enum TaskThreadTarget {
		MAIN_THREAD,
		WORKER_THREAD
	};
	enum TaskStatus {
		TASK_FINISHED = 0,

		TASK_INIT = 1,
		TASK_SUSPENDED,
		TASK_IN_QUEUE,
		TASK_WORKING,
	};

	class TasksQueue;
	class Task;

	using TaskPtr			= std::shared_ptr<Task>;
	using TaskUniquePtr		= std::unique_ptr<Task>;
	using TaskWeakPtr		= std::weak_ptr<Task>;
	using TaskExecutable	= std::function<void(TasksQueue* queue, TaskPtr task)>;

	using TaskBlocking = bool;
	using TaskPriority = uint32_t;

}
