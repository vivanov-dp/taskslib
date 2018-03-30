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

	void TasksQueue::Initialize(const Configuration& configuration) {
		std::lock_guard<std::mutex> guard(initMutex_);

		if (isInitialized_ || isShutDown_) {
			return;
		}
		if (configuration.blockingThreads_ < 1) {		// It's pointless without any normal worker threads
			return;
		}

		CreateThreads(configuration.blockingThreads_, configuration.nonBlockingThreads_, configuration.schedulingThreads_);
		numNonBlockingThreads_ = configuration.nonBlockingThreads_;
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




	bool TasksQueue::AddTask(TaskPtr task)
	{
		++stats_.added_;
		std::unique_lock<std::mutex> lock(task->GetTaskMutex_());
		return AddTask(task, std::move(lock));
	};
	bool TasksQueue::AddTask(TaskPtr task, const std::unique_lock<std::mutex> lockTaskData)
	{
		if (!task)
		{
			return false;
		}

		if (task->options_.suspendTime > std::chrono::milliseconds(0))
		{
			{
				std::unique_lock<std::mutex> lockSched(schedulerMutex_);
				scheduledTasks_.insert(schedulePair(scheduleClock::now() + task->options_.suspendTime, task));
				++stats_.suspended_;
				++stats_.waiting_;
			}

			scheduleEarliest_ = scheduleTimePoint::min();
			scheduleCondition_.notify_one();
		}
		else
		{
			if (!task->GetOptions().isMainThread)
			{
				{
					std::lock_guard<std::mutex> lockTask(tasksMutex_);
					tasks_.push_back(task);
				}

				tasksCondition_.notify_all();
			}
			else
			{
				std::lock_guard<std::mutex> lockResponse(mtTasksMutex_);
				mtTasks_.push_back(task);
			}

			task->status_ = TaskStatus::TASK_IN_QUEUE;

			if (task->GetOptions().priority > runningPriority_)
			{
				runningPriority_ = task->GetOptions().priority;
			}
		}

		++stats_.total_;
		return true;
	}

	void TasksQueue::ThreadExecuteTasks(const bool ignoreBlocking)
	{
		for (;;)
		{
			TaskPtr task = nullptr;
			{
				std::unique_lock<std::mutex> lockTasks(tasksMutex_);

				while (!isShutDown_ && tasks_.empty())
				{
					tasksCondition_.wait(lockTasks);
				}

				if (isShutDown_)
				{
					break;
				}

				task = tasks_.front();
				if (task)
				{
					std::lock_guard<std::mutex> lockTaskData(task->GetTaskMutex_());
					if ((task->GetOptions().isBlocking && ignoreBlocking)
						|| (task->GetOptions().priority < runningPriority_)
					   )
					{
						task = nullptr;
					}
					else
					{
						tasks_.erase(tasks_.begin());
					}
				}
			}

			if (task)
			{
				task->Execute(this, task);
				RescheduleTask(task);
			}
		}
	}
	void TasksQueue::ThreadExecuteScheduledTasks()
	{
		for (;;)
		{
			std::vector<TaskPtr> runTasks;

			{
				std::unique_lock<std::mutex> lockSched(schedulerMutex_);

				while (!isShutDown_ && scheduleEarliest_.load() > scheduleClock::now())
				{
					scheduleCondition_.wait(lockSched);
				}

				if (isShutDown_)
				{
					break;
				}

				auto now = scheduleClock::now();
				auto it = scheduledTasks_.begin();
				while ((it != scheduledTasks_.end()) && (it->first < now))
				{
					auto task = it->second;
					std::unique_lock<std::mutex> lockTask(task->GetTaskMutex_());
					task->options_.suspendTime = std::chrono::milliseconds(0);
					runTasks.push_back(task);
					it = scheduledTasks_.erase(it);
				}

				scheduleEarliest_ = (it != scheduledTasks_.end()) ? it->first : scheduleTimePoint::max();
			}

			while (!runTasks.empty())
			{
				auto task = runTasks.back();
				++stats_.resumed_;
				--stats_.waiting_;
				--stats_.total_;		// AddTask will increase this back
				AddTask(task);
				runTasks.pop_back();
			}
		}
	}
	void TasksQueue::Update()
	{
		if (isShutDown_)
		{
			return;
		}

		if (scheduleEarliest_.load() <= scheduleClock::now())
		{
			scheduleCondition_.notify_one();
		}

		TaskPtr task = nullptr;
		{
			std::vector<TaskPtr> runTasks;

			{
				std::lock_guard<std::mutex> lockTasks(mtTasksMutex_);
				if (mtTasks_.empty())
				{
					return;
				}

				while (!mtTasks_.empty())
				{
					task = mtTasks_.back();
					mtTasks_.pop_back();

					if (task)
					{
						std::lock_guard<std::mutex> lockTaskData(task->GetTaskMutex_());

						if (task->GetOptions().priority >= runningPriority_)
						{
							runTasks.push_back(task);
						}
						else
						{
							--stats_.total_;
						}
					}
				}
			}

			while (!runTasks.empty())
			{
				task = runTasks.back();
				task->Execute(this, task);
				RescheduleTask(task);
				runTasks.pop_back();
			}
		}
	}

	
	
	
	
	
	void TasksQueue::CreateThreads(const unsigned numBlockingThreads, const unsigned numNonBlockingThreads, const unsigned numSchedulingThreads) {
		uint32_t i;
		for (i = 0; i < numBlockingThreads; ++i) {
			auto thread = std::make_shared<TasksThread>(true, &TasksQueue::ThreadExecuteTasks, this, true);
			workerThreads_.push_back(thread);
		}
		for (i = 0; i < numNonBlockingThreads; ++i) {
			auto thread = std::make_shared<TasksThread>(false, &TasksQueue::ThreadExecuteTasks, this, false);
			workerThreads_.push_back(thread);
		}
		for (i = 0; i < numSchedulingThreads; ++i) {
			auto thread = std::make_shared<TasksThread>(false, &TasksQueue::ThreadExecuteScheduledTasks, this);
			schedulingThreads_.push_back(thread);
		}
	}





	void TasksQueue::RescheduleTask(std::shared_ptr<Task> task)
	{
		--stats_.total_;

		std::unique_lock<std::mutex> lockTaskData(task->GetTaskMutex_());
		if (task->doReschedule_)
		{
			task->ApplyReschedule_();
			AddTask(task, std::move(lockTaskData));
		}
		else
		{
			if (task->options_.priority > 0)
			{
				runningPriority_ = 0;
			}
			++stats_.completed_;
		}
	}

	TasksQueuePerformanceStats<std::int32_t> TasksQueue::GetPerformanceStats(bool reset)
	{
		TasksQueuePerformanceStats<std::int32_t> stats;

		stats.added_ = reset ? stats_.added_.exchange(0) : stats_.added_.load();
		stats.completed_ = reset ? stats_.completed_.exchange(0) : stats_.completed_.load();
		stats.suspended_ = reset ? stats_.suspended_.exchange(0) : stats_.suspended_.load();
		stats.resumed_ = reset ? stats_.resumed_.exchange(0) : stats_.resumed_.load();

		return stats;
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
