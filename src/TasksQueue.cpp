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
	TasksQueue::Configuration::Configuration(unsigned numBlockingThreads, unsigned numNonBlockingThreads, unsigned numSchedulingThreads)
		: blockingThreads_(numBlockingThreads)
		, nonBlockingThreads_(numNonBlockingThreads)
		, schedulingThreads_(numSchedulingThreads) {}

	// ===== TasksQueue =================================================================
	TasksQueue::TasksQueue()
		: isInitialized_(false)
		, isShutDown_(false)
		, runningPriority_(0)
		, numNonBlockingThreads_(0)
		, scheduleEarliest_(scheduleTimePoint::min())
	{}
	TasksQueue::~TasksQueue() {
		ShutDown();
	}

	const bool TasksQueue::isInitialized() const {
		return isInitialized_;
	}
	const bool TasksQueue::isShutDown() const {
		return isShutDown_;
	}

	const unsigned TasksQueue::numWorkerThreads() const {
		return static_cast<unsigned>(workerThreads_.size());
	}
	const unsigned TasksQueue::numBlockingThreads() const {
		return static_cast<unsigned>(workerThreads_.size()) - numNonBlockingThreads_;
	}
	const unsigned TasksQueue::numNonBlockingThreads() const {
		return numNonBlockingThreads_;
	}
	const unsigned TasksQueue::numSchedulingThreads() const {
		return static_cast<unsigned>(schedulingThreads_.size());
	}

	TasksQueuePerformanceStats<std::int32_t> TasksQueue::GetPerformanceStats(const bool reset) {
		TasksQueuePerformanceStats<std::int32_t> stats;

		stats.added = reset ? stats_.added.exchange(0) : stats_.added.load();
		stats.completed = reset ? stats_.completed.exchange(0) : stats_.completed.load();
		stats.suspended = reset ? stats_.suspended.exchange(0) : stats_.suspended.load();
		stats.resumed = reset ? stats_.resumed.exchange(0) : stats_.resumed.load();
		stats.waiting = stats_.waiting.load();
		stats.total = stats_.total.load();

		return std::move(stats);
	}

	void TasksQueue::Initialize(const Configuration& configuration) {
		std::lock_guard<std::mutex> guard(initMutex_);

		if (isInitialized_ || isShutDown_) {
			return;
		}
		if (configuration.blockingThreads_ < 1) {		// It's pointless without any normal worker threads
			return;
		}

		CreateThreads(configuration.blockingThreads_, configuration.nonBlockingThreads_, configuration.schedulingThreads_);
		isInitialized_ = true;
	}
	void TasksQueue::ShutDown() {
		if (!isInitialized_ || isShutDown_) {
			return;
		}

		isShutDown_ = true;

		tasksCondition_.notify_all();
		scheduleCondition_.notify_all();
		for (std::shared_ptr<TasksThread> thread : workerThreads_) {
			thread->join();
		}
		for (std::shared_ptr<TasksThread> thread : schedulingThreads_) {
			thread->join();
		}

		{
			std::lock_guard<std::mutex> guard(initMutex_);
			workerThreads_.clear();
			schedulingThreads_.clear();
			numNonBlockingThreads_ = 0;
			isInitialized_ = false;
		}
	}

	bool TasksQueue::AddTask(TaskPtr task) {
		if (!isInitialized_) {
			return false;
		}

		++stats_.added;
		std::unique_lock<std::mutex> lock(task->GetTaskMutex_());
		return AddTask(task, std::move(lock));
	};

	void TasksQueue::Update() {
		if (!isInitialized_ || isShutDown_) {
			return;
		}

		if (scheduleEarliest_.load() <= scheduleClock::now()) {
			scheduleCondition_.notify_one();
		}

		std::vector<TaskPtr> runTasks;
		std::vector<TaskPtr> ignoreTasks;

		{
			std::lock_guard<std::mutex> lockTasks(mtTasksMutex_);

			for (auto task : mtTasks_) {
				if (task) {
					std::lock_guard<std::mutex> lockTask(task->GetTaskMutex_());

					if (task->GetOptions().priority >= runningPriority_) {
						runTasks.push_back(task);
					}
					else {
						ignoreTasks.push_back(task);
					}
				}
			}

			mtTasks_ = std::move(ignoreTasks);
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

	void TasksQueue::CreateThreads(const unsigned numBlockingThreads, const unsigned numNonBlockingThreads, const unsigned numSchedulingThreads) {
		for (unsigned i = 0; i < numBlockingThreads; ++i) {
			auto thread = std::make_shared<TasksThread>(false, &TasksQueue::ThreadExecuteTasks, this, false);
			workerThreads_.push_back(thread);
		}
		for (unsigned i = 0; i < numNonBlockingThreads; ++i) {
			auto thread = std::make_shared<TasksThread>(true, &TasksQueue::ThreadExecuteTasks, this, true);
			workerThreads_.push_back(thread);
		}
		numNonBlockingThreads_ = numNonBlockingThreads;
		for (unsigned i = 0; i < numSchedulingThreads; ++i) {
			auto thread = std::make_shared<TasksThread>(false, &TasksQueue::ThreadExecuteScheduledTasks, this);
			schedulingThreads_.push_back(thread);
		}
	}
	bool TasksQueue::AddTask(TaskPtr task, const std::unique_lock<std::mutex> lockTask, const bool updateTotal) {
		if (!task || isShutDown_) {
			return false;
		}

		if (task->options_.suspendTime > std::chrono::milliseconds(0)) {
			{
				std::lock_guard<std::mutex> lockSched(schedulerMutex_);
				scheduledTasks_.insert(schedulePair(scheduleClock::now() + task->options_.suspendTime, task));
				++stats_.suspended;
				++stats_.waiting;
				task->status_ = TaskStatus::TASK_SUSPENDED;
			}

			scheduleEarliest_ = scheduleTimePoint::min();
			scheduleCondition_.notify_one();
		} else {
			if (!task->GetOptions().isMainThread) {
				{
					std::lock_guard<std::mutex> lock(tasksMutex_);
					tasks_.push_back(task);
					task->status_ = TaskStatus::TASK_IN_QUEUE;
				}

				tasksCondition_.notify_all();
			} else {
				std::lock_guard<std::mutex> lock(mtTasksMutex_);
				mtTasks_.push_back(task);
				task->status_ = TaskStatus::TASK_IN_QUEUE_MAIN_THREAD;
			}

			if (task->GetOptions().priority > runningPriority_) {
				runningPriority_ = task->GetOptions().priority;
			}
		}

		if (updateTotal) {
			++stats_.total;
		}
		return true;
	}

	void TasksQueue::ThreadExecuteTasks(const bool ignoreBlocking) {
		for (;;) {
			TaskPtr task = nullptr;
			{
				std::unique_lock<std::mutex> lockTasks(tasksMutex_);

				while (!isShutDown_ && tasks_.empty()) {
					tasksCondition_.wait(lockTasks);
				}
				if (isShutDown_) {
					break;
				}

				for (auto it = tasks_.begin(); it < tasks_.end(); ++it) {
					task = *it;
					if (task) {
						std::lock_guard<std::mutex> lockTask(task->GetTaskMutex_());
						if ((task->GetOptions().isBlocking && ignoreBlocking)
							|| (task->GetOptions().priority < runningPriority_)
							)
						{
							task = nullptr;
						}
						else {
							tasks_.erase(it);
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
				std::unique_lock<std::mutex> lockSched(schedulerMutex_);

				while (!isShutDown_ && scheduleEarliest_.load() > scheduleClock::now()) {
					scheduleCondition_.wait(lockSched);
				}
				if (isShutDown_) {
					break;
				}

				auto now = scheduleClock::now();
				auto it = scheduledTasks_.begin();
				while ((it != scheduledTasks_.end()) && (it->first < now)) {
					auto task = it->second;
					std::unique_lock<std::mutex> lockTask(task->GetTaskMutex_());
					task->options_.suspendTime = TaskDelay{ 0 };
					runTasks.push_back(task);
					it = scheduledTasks_.erase(it);
				}

				scheduleEarliest_ = (it != scheduledTasks_.end()) ? it->first : scheduleTimePoint::max();
			}

			while (!runTasks.empty()) {
				auto task = runTasks.back();
				++stats_.resumed;
				--stats_.waiting;
				
				std::unique_lock<std::mutex> lock(task->GetTaskMutex_());
				AddTask(task, std::move(lock), false);

				runTasks.pop_back();
			}
		}
	}
	
	void TasksQueue::RescheduleTask(std::shared_ptr<Task> task) {
		std::unique_lock<std::mutex> lockTaskData(task->GetTaskMutex_());
		if (task->doReschedule_) {
			task->ApplyReschedule_();
			AddTask(task, std::move(lockTaskData), false);
		} else {
			if (task->options_.priority > 0) {
				runningPriority_ = 0;
			}
			--stats_.total;
			++stats_.completed;
		}
	}



	// ===== TasksQueueContainer ========================================================
	TasksQueuesContainer::TasksQueuesContainer()
		: isInitialized_(false) {};
	TasksQueuesContainer::~TasksQueuesContainer() {
		ShutDown();
	}

	void TasksQueuesContainer::Initialize() {
		isInitialized_ = true;
	}

	void TasksQueuesContainer::ShutDown() {
		isInitialized_ = false;

		for (auto& it : queueMap_) {
			it.second.ShutDown();
		}
	}

	TasksQueue* TasksQueuesContainer::GetQueue(const std::string& queueName) {
		if (!isInitialized_) {
			return nullptr;
		}

		auto queueIt = queueMap_.find(queueName);
		if (queueIt == queueMap_.end()) {
			return nullptr;
		}

		return &(queueIt->second);
	}

	void TasksQueuesContainer::CreateQueue(const std::string& queueName, const TasksQueue::Configuration& configuration)
	{
		auto queueIt = queueMap_.find(queueName);
		if (queueIt != queueMap_.end()) {
			return;
		}

		auto pair = queueMap_.emplace(std::piecewise_construct, std::forward_as_tuple(queueName), std::forward_as_tuple());
		queueIt = pair.first;
		queueIt->second.Initialize(configuration);
	}

	void TasksQueuesContainer::Update()
	{
		for (auto it = queueMap_.begin(); it != queueMap_.end(); ++it)
		{
			it->second.Update();
		}
	}
}
