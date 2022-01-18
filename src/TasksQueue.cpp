#include <tuple>
#include <chrono>
#include <thread>

#include "TasksThread.h"
#include "Task.h"
#include "TasksQueue.h"

#define DEFAULT_TQUEUE_BLOCKING		6
#define DEFAULT_TQUEUE_NONBLOCKING	2
#define DEFAULT_TQUEUE_SCHEDULING	1

namespace TasksLib {

	// ===== TasksQueue::Configuration ==================================================
	TasksQueue::Configuration::Configuration()
		: Configuration(DEFAULT_TQUEUE_BLOCKING, DEFAULT_TQUEUE_NONBLOCKING, DEFAULT_TQUEUE_SCHEDULING) {}
	TasksQueue::Configuration::Configuration(uint16_t numBlockingThreads, uint16_t numNonBlockingThreads, uint16_t numSchedulingThreads)
		: blockingThreads(numBlockingThreads)
		, nonBlockingThreads(numNonBlockingThreads)
		, schedulingThreads(numSchedulingThreads) {}

	// ===== TasksQueue =================================================================
	TasksQueue::TasksQueue()
		: _isInitialized(false)
		, _isShuttingDown(false)
		, _runningPriority(0)
		, _numNonBlockingThreads(0)
		, _scheduleEarliest(scheduleTimePoint::min())
	{}
	TasksQueue::TasksQueue(const Configuration& configuration)
		: TasksQueue()
	{
		Initialize(configuration);
	}
	TasksQueue::~TasksQueue() {
        Cleanup();
	}

	bool TasksQueue::isInitialized() const {
		return _isInitialized;
	}
    [[maybe_unused]] bool TasksQueue::isShuttingDown() const {
		return _isShuttingDown;
	}

    [[maybe_unused]] uint16_t TasksQueue::numWorkerThreads() const {
		return static_cast<uint16_t>(_workerThreads.size());
	}
    [[maybe_unused]] uint16_t TasksQueue::numBlockingThreads() const {
		return static_cast<uint16_t>(_workerThreads.size()) - _numNonBlockingThreads;
	}
    [[maybe_unused]] uint16_t TasksQueue::numNonBlockingThreads() const {
		return _numNonBlockingThreads;
	}
    [[maybe_unused]] uint16_t TasksQueue::numSchedulingThreads() const {
		return static_cast<uint16_t>(_schedulingThreads.size());
	}

	TasksQueuePerformanceStats<std::uint32_t> TasksQueue::GetPerformanceStats(const bool reset) {
		TasksQueuePerformanceStats<std::uint32_t> stats;

        if (reset) {
            stats.added = _stats.added.exchange(0);
            stats.completed = _stats.completed.exchange(0);
            stats.suspended = _stats.suspended.exchange(0);
            stats.resumed = _stats.resumed.exchange(0);
        } else {
            stats.added = _stats.added.load();
            stats.completed = _stats.completed.load();
            stats.suspended = _stats.suspended.load();
            stats.resumed = _stats.resumed.load();
        }

		stats.waiting = _stats.waiting.load();
		stats.total = _stats.total.load();

		return stats;
	}

	void TasksQueue::Initialize(const Configuration& configuration) {
		std::lock_guard<std::mutex> guard(_initMutex);

		if (_isInitialized || _isShuttingDown) {
			return;
		}
		if (configuration.blockingThreads < 1) {		// It's pointless without any normal worker threads
			return;
		}

		CreateThreads(configuration);
        _isInitialized = true;
	}
	void TasksQueue::Cleanup() {
		if (!_isInitialized || _isShuttingDown) {
			return;
		}

        _isShuttingDown = true;

		_tasksCondition.notify_all();
		_scheduleCondition.notify_all();
		for (const std::shared_ptr<TasksThread>& thread : _workerThreads) {
			thread->join();
		}
		for (const std::shared_ptr<TasksThread>& thread : _schedulingThreads) {
			thread->join();
		}

		{
			std::lock_guard<std::mutex> guard(_initMutex);
			_workerThreads.clear();
			_schedulingThreads.clear();
            _numNonBlockingThreads = 0;
            _isInitialized = false;
            _isShuttingDown = false;
		}
	}

    [[maybe_unused]] bool TasksQueue::AddTask(const TaskPtr& task) {
		if (!_isInitialized || _isShuttingDown) {
			return false;
		}

		++_stats.added;
		std::unique_lock<std::mutex> lock(task->GetTaskMutex_());
		return AddTask(task, std::move(lock));
	};
	void TasksQueue::Update() {
		if (!_isInitialized || _isShuttingDown) {
			return;
		}

		if (_scheduleEarliest.load() <= scheduleClock::now()) {
			_scheduleCondition.notify_one();
		}

		std::vector<TaskPtr> runTasks;
		std::vector<TaskPtr> ignoreTasks;

		{
			std::lock_guard<std::mutex> lockTasks(_mtTasksMutex);

			for (const auto& task : _mtTasks) {
				if (task) {
					std::lock_guard<std::mutex> lockTask(task->GetTaskMutex_());

					if (task->GetOptions().priority >= _runningPriority) {
						runTasks.push_back(task);
					}
					else {
						ignoreTasks.push_back(task);
					}
				}
			}

            _mtTasks = std::move(ignoreTasks);
		}

		// TODO: Don't execute all tasks at once, instead examine stats and come up with a smaller number, but staying ahead of newly added ones.
		//		 Make that an option switch:
		//			1. One per Update() (risks falling behind)
		//			2. All per Update() (risks delaying the main thread)
		//			3. Auto (passive/eager) (less predictable)
		while (!runTasks.empty()) {
			TaskPtr task = runTasks.back();
			task->Execute(this, task);
			RescheduleTask(task);
			runTasks.pop_back();
		}
	}

	void TasksQueue::CreateThreads(const Configuration& i_config) {
		for (int i = 0; i < i_config.blockingThreads; ++i) {
			auto thread = std::make_shared<TasksThread>(false, &TasksQueue::ThreadExecuteTasks, this, false);
			_workerThreads.push_back(thread);
		}
		for (int i = 0; i < i_config.nonBlockingThreads; ++i) {
			auto thread = std::make_shared<TasksThread>(true, &TasksQueue::ThreadExecuteTasks, this, true);
			_workerThreads.push_back(thread);
		}
        _numNonBlockingThreads = i_config.nonBlockingThreads;
		for (int i = 0; i < i_config.schedulingThreads; ++i) {
			auto thread = std::make_shared<TasksThread>(false, &TasksQueue::ThreadExecuteScheduledTasks, this);
			_schedulingThreads.push_back(thread);
		}
	}
	bool TasksQueue::AddTask(const TaskPtr& task, [[maybe_unused]] const std::unique_lock<std::mutex> lockTask, const bool updateTotal) {
		if (!task || _isShuttingDown) {
			return false;
		}

		if (task->_options.suspendTime > std::chrono::milliseconds(0)) {
			{
				std::lock_guard<std::mutex> lockSched(_schedulerMutex);
				_scheduledTasks.insert(schedulePair(scheduleClock::now() + task->_options.suspendTime, task));
				++_stats.suspended;
				++_stats.waiting;
				task->_status = TaskStatus::TASK_SUSPENDED;
			}

            _scheduleEarliest = scheduleTimePoint::min();
			_scheduleCondition.notify_one();
		} else {
			if (!task->GetOptions().isMainThread) {
				{
					std::lock_guard<std::mutex> lock(_tasksMutex);
					_tasks.push_back(task);
					task->_status = TaskStatus::TASK_IN_QUEUE;
				}

				_tasksCondition.notify_all();
			} else {
				std::lock_guard<std::mutex> lock(_mtTasksMutex);
				_mtTasks.push_back(task);
				task->_status = TaskStatus::TASK_IN_QUEUE_MAIN_THREAD;
			}

			if (task->GetOptions().priority > _runningPriority) {
                _runningPriority = task->GetOptions().priority;
			}
		}

		if (updateTotal) {
			++_stats.total;
		}
		return true;
	}

	void TasksQueue::ThreadExecuteTasks(const bool ignoreBlocking) {
		for (;;) {
			TaskPtr task = nullptr;
			{
				std::unique_lock<std::mutex> lockTasks(_tasksMutex);

				while (!_isShuttingDown && _tasks.empty()) {
					_tasksCondition.wait(lockTasks);
				}
				if (_isShuttingDown) {
					break;
				}

				for (auto it = _tasks.begin(); it < _tasks.end(); ++it) {
					task = *it;
					if (task) {
						std::lock_guard<std::mutex> lockTask(task->GetTaskMutex_());
						if ((task->GetOptions().isBlocking && ignoreBlocking)
							|| (task->GetOptions().priority < _runningPriority)
							)
						{
							task = nullptr;
						}
						else {
							_tasks.erase(it);
							break;
						}
					}
				}
			}

			if (task) {
				task->Execute(this, task);
				RescheduleTask(task);
			}
		}
	}
	void TasksQueue::ThreadExecuteScheduledTasks() {
		for (;;) {
			std::vector<TaskPtr> runTasks;

			{
				std::unique_lock<std::mutex> lockSched(_schedulerMutex);

				while (!_isShuttingDown && _scheduleEarliest.load() > scheduleClock::now()) {
					_scheduleCondition.wait(lockSched);
				}
				if (_isShuttingDown) {
					break;
				}

				auto now = scheduleClock::now();
				auto it = _scheduledTasks.begin();
				while ((it != _scheduledTasks.end()) && (it->first < now)) {
					auto task = it->second;
					std::unique_lock<std::mutex> lockTask(task->GetTaskMutex_());
					task->_options.suspendTime = TaskDelay{0 };
					runTasks.push_back(task);
					it = _scheduledTasks.erase(it);
				}

                _scheduleEarliest = (it != _scheduledTasks.end()) ? it->first : scheduleTimePoint::max();
			}

			while (!runTasks.empty()) {
				auto task = runTasks.back();
				++_stats.resumed;
				--_stats.waiting;
				
				std::unique_lock<std::mutex> lock(task->GetTaskMutex_());
				AddTask(task, std::move(lock), false);

				runTasks.pop_back();
			}
		}
	}
	
	void TasksQueue::RescheduleTask(const std::shared_ptr<Task>& task) {
		std::unique_lock<std::mutex> lockTaskData(task->GetTaskMutex_());
		if (task->_doReschedule) {
			task->ApplyReschedule_();
			AddTask(task, std::move(lockTaskData), false);
		} else {
			if (task->_options.priority > 0) {
                _runningPriority = 0;
			}
			--_stats.total;
			++_stats.completed;
		}
	}

}
