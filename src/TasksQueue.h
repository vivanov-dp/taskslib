#pragma once

#include "Atomic/Core/Assert.h"
#include "Atomic/Core/Object.h"
#include "Atomic/Container/List.h"

#include "Task.h"

#include <mutex>
#include <condition_variable>
#include <map>
#include <unordered_map>

namespace TasksLib
{
	class TaskThread;
	class TaskScheduleThread;
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

	private:
		void CreateThreads(const unsigned count, const unsigned countNonBlocking, const unsigned scheduling);
		void RescheduleTask(std::shared_ptr<Task> task);

	private:

		std::atomic<bool> shutDown_;
		std::atomic<bool> isInitialized_;
		std::atomic<unsigned int> runningPriority_;

		// Mutexes lock order is - (Task->dataMutex), dataMutex, schedulerMutex, tasksMutex, mtTasksMutex

		std::mutex dataMutex_;
		Atomic::Vector<std::shared_ptr<TaskThread>> threads_;

		std::mutex schedulerMutex_;
		std::condition_variable scheduleCondition_;
		Atomic::Vector<std::shared_ptr<TaskScheduleThread>> schedulingThreads_;
		scheduleMap scheduledTasks_;

		std::atomic<scheduleTimePoint> scheduleEarliest_;

		std::mutex tasksMutex_;
		std::condition_variable tasksCondition_;
		Atomic::List<TaskPtr> tasks_;

		std::mutex mtTasksMutex_;
		Atomic::List<TaskPtr> mtTasks_;

		TasksQueuePerformanceStats<std::atomic<std::int32_t>> stats_;

	protected:
		void Update();

		friend class TasksQueuesContainer;
	};

	// ===== TasksQueueAtomic ========================================================
	class ATOMIC_API TasksQueueAtomic : public Atomic::Object, public TasksQueue
	{
		ATOMIC_OBJECT(TasksQueueAtomic, Object);

	public:
		TasksQueueAtomic(Atomic::Context* context);
		virtual ~TasksQueueAtomic();

		void HandleUpdate(Atomic::StringHash eventType, Atomic::VariantMap& eventData);
	};

	// ===== TasksQueueContainer ========================================================
	class ATOMIC_API TasksQueuesContainer : public Atomic::Object
	{
		ATOMIC_OBJECT(TasksQueuesContainer, Object);

	public:
		TasksQueuesContainer(Atomic::Context* context);
		virtual ~TasksQueuesContainer();

		void Initialize();
		void ShutDown();

		TasksQueue& GetQueue(const std::string& queueName);
		void CreateQueue(const std::string& queueName, const TasksQueue::Configuration& configuration);

		void HandleUpdate(Atomic::StringHash eventType, Atomic::VariantMap& eventData);

	private:
		bool isInitialized_;

		std::unordered_map<std::string, TasksQueue> queueMap_;
	};
}
