#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <map>
#include <unordered_map>
#include <vector>

#include "Types.h"

namespace TasksLib {

	template<typename T>
	struct TasksQueuePerformanceStats {
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

	class TasksQueue {
	public:
		struct Configuration {
			Configuration();
			Configuration(unsigned numBlockingThreads, unsigned numNonBlockingThreads = 0, unsigned numSchedulingThreads = 0);

			unsigned blockingThreads_;
			unsigned nonBlockingThreads_;
			unsigned schedulingThreads_;
		};

		TasksQueue();
		virtual ~TasksQueue();

		const bool isInitialized() const;
		const bool isShutDown() const;

		const unsigned numWorkerThreads() const;
		const unsigned numBlockingThreads() const;
		const unsigned numNonBlockingThreads() const;
		const unsigned numSchedulingThreads() const;

		/* Initialize the threads queue with the specified number of threads 
		   @param configuration
		     
			 struct Configuration {
		       Configuration();
		       Configuration(unsigned numBlockingThreads, unsigned numNonBlockingThreads = 0, unsigned numSchedulingThreads = 0);

		       unsigned blockingThreads_;
		       unsigned nonBlockingThreads_;
		       unsigned schedulingThreads_;
		     };
		   
		   numBlockingThreads should be at least 1.
		   numSchedulingThreads = 0 will disable the ability to put tasks on delay.
		   
		   Default constructor yields some sensible minimum thread numbers, with at least 1 in each category.
		   The TasksQueue will not initialize if the number of blocking threads requested is 0.
		*/
		void Initialize(const Configuration& configuration);
		void ShutDown();





		bool AddTask(TaskPtr task);
		bool AddTask(TaskPtr task, const std::unique_lock<std::mutex> lockTaskData);
		void ThreadExecuteTasks(const bool ignoreBlocking);
		void ThreadExecuteScheduledTasks();

		TasksQueuePerformanceStats<std::int32_t> GetPerformanceStats(bool reset);

		void Update();

	private:
		void CreateThreads(const unsigned numBlockingThreads, const unsigned numNonBlockingThreads, const unsigned numSchedulingThreads);
		void RescheduleTask(std::shared_ptr<Task> task);

	private:
		std::atomic<bool> isInitialized_;
		std::atomic<bool> isShutDown_;
		std::atomic<unsigned int> runningPriority_;
		
		unsigned numNonBlockingThreads_;

		// Mutexes lock order is - (Task->dataMutex), initMutex, schedulerMutex, tasksMutex, mtTasksMutex

		std::mutex initMutex_;				// To ensure that calling Initialize() and/or Shutdown() from many threads at the same time is going to work
		std::vector<std::shared_ptr<TasksThread>> workerThreads_;

		std::mutex schedulerMutex_;
		std::condition_variable scheduleCondition_;
		std::vector<std::shared_ptr<TasksThread>> schedulingThreads_;
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
	class TasksQueuesContainer {
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
