#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <map>
#include <unordered_map>
#include <vector>

#include "Task.h"



namespace TasksLib
{
	class TaskThread;
	class TasksQueuesContainer;

	using scheduleClock = std::chrono::steady_clock;
	using scheduleTimePoint = std::chrono::time_point<scheduleClock>;
	using scheduleDuration = scheduleClock::duration;
	using scheduleMap = std::multimap<scheduleTimePoint, TaskPtr>;
	using schedulePair = std::pair<scheduleTimePoint, TaskPtr>;

	template<typename T>
	struct TasksQueuePerformanceStats
	{
		TasksQueuePerformanceStats()
			: added_(0)
			, completed_(0)
			, suspended_(0)
			, resumed_(0)
			, waiting_(0)
			, total_(0)
		{}

		T added_;
		T completed_;
		T suspended_;
		T resumed_;
		T waiting_;
		T total_;
	};

	class TasksQueue
	{
	public:
		struct Configuration
		{
			Configuration();
			Configuration(unsigned blocking, unsigned nonBlocking);
			Configuration(unsigned blocking, unsigned nonBlocking, unsigned scheduling);

			unsigned blockingThreads_;
			unsigned nonBlockingThreads_;
			unsigned schedulingThreads_;
		};

		TasksQueue();
		virtual ~TasksQueue();

		void Initialize(const Configuration& configuration);
		void ShutDown();

		bool AddTask(TaskPtr task);
		bool AddTask(TaskPtr task, const std::unique_lock<std::mutex> lockTaskData);
		void ThreadExecuteTasks(const uint32_t threadNum, const bool ignoreBlocking);
		void ThreadExecuteScheduledTasks(const uint32_t threadNum);

		TasksQueuePerformanceStats<std::int32_t> GetPerformanceStats(bool reset);

		void Update();

	private:
		void CreateThreads(const unsigned count, const unsigned countNonBlocking, const unsigned scheduling);
		void RescheduleTask(std::shared_ptr<Task> task);

	private:
		std::atomic<bool> shutDown_;
		std::atomic<bool> isInitialized_;
		std::atomic<unsigned int> runningPriority_;

		// Mutexes lock order is - (Task->dataMutex), dataMutex, schedulerMutex, tasksMutex, mtTasksMutex

		std::mutex dataMutex_;
		std::vector<std::shared_ptr<TaskThread>> threads_;

		std::mutex schedulerMutex_;
		std::condition_variable scheduleCondition_;
		std::vector<std::shared_ptr<TaskThread>> schedulingThreads_;
		scheduleMap scheduledTasks_;

		std::atomic<scheduleTimePoint> scheduleEarliest_;

		std::mutex tasksMutex_;
		std::condition_variable tasksCondition_;
		std::vector<TaskPtr> tasks_;

		std::mutex mtTasksMutex_;
		std::vector<TaskPtr> mtTasks_;

		TasksQueuePerformanceStats<std::atomic<std::int32_t>> stats_;
	};

	// ===== TasksQueueContainer ========================================================
	class TasksQueuesContainer
	{
	public:
		TasksQueuesContainer();
		virtual ~TasksQueuesContainer();

		void Initialize();
		void ShutDown();

		TasksQueue* GetQueue(const std::string& queueName);
		void CreateQueue(const std::string& queueName, const TasksQueue::Configuration& configuration);

		void Update();

	private:
		bool isInitialized_;

		std::unordered_map<std::string, TasksQueue> queueMap_;
	};
}
