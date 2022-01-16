#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <map>
#include <unordered_map>
#include <vector>
#include <cstdint>

#include "Types.h"

namespace TasksLib {

	template<typename T> struct TasksQueuePerformanceStats {
		TasksQueuePerformanceStats()
			: added(0)
			, completed(0)
			, suspended(0)
			, resumed(0)
			, waiting(0)
			, total(0)
		{}

		// accumulating between resets
		T added;			// Tasks added
		T completed;		// Tasks completed and out of queue
		T suspended;		// Tasks scheduled for delayed execution
		T resumed;			// Tasks resumed after delay
		// current (does not reset)
		T waiting;			// Tasks waiting in suspended state
		T total;			// Total tasks in the queue
	};

	class TasksQueue {
    private:
        std::atomic<bool> _isInitialized;
        std::atomic<bool> _isShuttingDown;
        std::atomic<uint32_t> _runningPriority;

        uint16_t _numNonBlockingThreads;
        TasksQueuePerformanceStats<std::atomic<std::int32_t>> _stats;

        // Mutexes lock order is - (Task->dataMutex), initMutex, schedulerMutex, tasksMutex, mtTasksMutex
        std::mutex _initMutex;				// To ensure that calling Initialize() and/or Shutdown() from many threads at the same time is going to work
        std::vector<std::shared_ptr<TasksThread>> _workerThreads;

        std::mutex _schedulerMutex;
        std::condition_variable _scheduleCondition;
        std::vector<std::shared_ptr<TasksThread>> _schedulingThreads;
        scheduleMap _scheduledTasks;

        // The earliest point in time when the scheduling thread has something to do -> the time of the first delayed task
        std::atomic<scheduleTimePoint> _scheduleEarliest;

        std::mutex _tasksMutex;
        std::condition_variable _tasksCondition;
        std::vector<TaskPtr> _tasks;

        std::mutex _mtTasksMutex;
        std::vector<TaskPtr> _mtTasks;

	public:
		struct Configuration {
			Configuration();
			Configuration(uint16_t numBlockingThreads, uint16_t numNonBlockingThreads = 0, uint16_t numSchedulingThreads = 0);

            uint16_t blockingThreads;
            uint16_t nonBlockingThreads;
            uint16_t schedulingThreads;
		};

		TasksQueue();
		TasksQueue(const Configuration& configuration);
		virtual ~TasksQueue();

		[[nodiscard]] bool isInitialized() const;
        [[maybe_unused]] [[nodiscard]] bool isShutDown() const;

        [[maybe_unused]] [[nodiscard]] uint16_t numWorkerThreads() const;
        [[maybe_unused]] [[nodiscard]] uint16_t numBlockingThreads() const;
        [[maybe_unused]] [[nodiscard]] uint16_t numNonBlockingThreads() const;
        [[maybe_unused]] [[nodiscard]] uint16_t numSchedulingThreads() const;

		TasksQueuePerformanceStats<std::uint32_t> GetPerformanceStats(bool reset = false);

		/* Initialize the threads queue with the specified number of threads 
		   @param configuration
		     
                struct Configuration {
                    Configuration();
                    Configuration(uint16_t numBlockingThreads, uint16_t numNonBlockingThreads = 0, uint16_t numSchedulingThreads = 0);

                    uint16_t blockingThreads;
                    uint16_t nonBlockingThreads;
                    uint16_t schedulingThreads;
                };
		   
		   numBlockingThreads should be at least 1.
		   numSchedulingThreads = 0 will disable the ability to put tasks on delay.
		   
		   Default constructor yields some sensible minimum thread numbers, with at least 1 in each category.
		   The TasksQueue will not initialize if the number of blocking threads requested is 0.
		*/
		void Initialize(const Configuration& configuration);
		void Cleanup();

        [[maybe_unused]] bool AddTask(const TaskPtr& task);
		/* Handle queue updates
		   You are supposed to call this periodically on your main thread. If Update() doesn't get called, tasks that are targeted on the main thread will
		   never get executed, also tasks that are suspended will never wake.
		 */
		void Update();

	private:
		void CreateThreads(const Configuration& configuration);
		bool AddTask(const TaskPtr& task, std::unique_lock<std::mutex> lockTask, bool updateTotal = true);
		
		void ThreadExecuteTasks(bool ignoreBlocking);
		void ThreadExecuteScheduledTasks();

		void RescheduleTask(const std::shared_ptr<Task>& task);
    };

}
