#pragma once

#include <memory>
#include <functional>
#include <chrono>
#include <map>

namespace TasksLib {

	// === Classes =====
	class TaskOptions;
	class Task;
	class TasksThread;
	class TasksQueue;
	class TasksQueuesContainer;

	// === Task =====
	enum TaskStatus {
		TASK_FINISHED = 0,

		TASK_INIT = 1,			// Freshly created, not added to queue
		TASK_SUSPENDED,			// Put on hold, will reschedule after a specified delay
		TASK_IN_QUEUE,			// Waiting in a queue
		TASK_WORKING,			// Executing
	};

	using TaskPtr		= std::shared_ptr<Task>;
	using TaskUniquePtr	= std::unique_ptr<Task>;
	using TaskWeakPtr	= std::weak_ptr<Task>;

	template <class T> class ResourcePool;

	// Types accepted as options for tasks (in Task::Task() and Task::SetOptions()) :
	enum TaskThreadTarget {
		MAIN_THREAD,
		WORKER_THREAD
	};
	using TaskBlocking		= bool;
	using TaskPriority		= uint32_t;
	using TaskExecutable	= std::function<void(TasksQueue* queue, TaskPtr task)>;
	using TaskDelay			= std::chrono::milliseconds;
	// </Types as options>

	// === TasksQueue =====
	using scheduleClock		= std::chrono::steady_clock;
	using scheduleTimePoint	= std::chrono::time_point<scheduleClock>;
	using scheduleDuration	= scheduleClock::duration;
	using scheduleMap		= std::multimap<scheduleTimePoint, TaskPtr>;
	using schedulePair		= std::pair<scheduleTimePoint, TaskPtr>;

}
