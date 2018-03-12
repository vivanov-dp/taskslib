#include "TasksQueue.h"

#include "Atomic/IO/Log.h"
#include "Atomic/Core/CoreEvents.h"
#include "Atomic/Core/Defensive.h"

#include <tuple>
#include <chrono>
#include <thread>

namespace TasksLib
{
	class TaskThread : public Atomic::Thread
	{
	public:
		TaskThread(TasksQueue& queue, const uint32_t threadNum, const bool ignoreBlocking)
			: queue_(queue)
			, threadNum_(threadNum)
			, ignoreBlocking_(ignoreBlocking)
		{
		}

		virtual void ThreadFunction()
		{
			queue_.ThreadExecuteTasks(threadNum_, ignoreBlocking_);
		}

	private:
		TasksQueue& queue_;
		uint32_t threadNum_;
		bool ignoreBlocking_;
	};
	class TaskScheduleThread : public Atomic::Thread
	{
	public:
		TaskScheduleThread(TasksQueue& queue, const uint32_t threadNum)
			: queue_(queue)
			, threadNum_(threadNum)
		{
		}

		virtual void ThreadFunction()
		{
			queue_.ThreadExecuteScheduledTasks(threadNum_);
		}

	private:
		TasksQueue& queue_;
		uint32_t threadNum_;
	};

	// ===== TasksQueue::Configuration ==================================================
	TasksQueue::Configuration::Configuration()
		: Configuration(6, 2)
	{
	}
	TasksQueue::Configuration::Configuration(unsigned blocking, unsigned nonBlocking)
		: Configuration(blocking, nonBlocking, 0)
	{
	}
	TasksQueue::Configuration::Configuration(unsigned blocking, unsigned nonBlocking, unsigned scheduling)
		: blockingThreads_(blocking)
		, nonBlockingThreads_(nonBlocking)
		, schedulingThreads_(scheduling)
	{
	}

	// ===== TasksQueue =================================================================
	TasksQueue::TasksQueue()
		: isInitialized_(false)
		, shutDown_(false)
		, runningPriority_(0)
		, scheduleEarliest_(scheduleTimePoint::min())
	{
	}
	TasksQueue::~TasksQueue()
	{
		ShutDown();
	}

	void TasksQueue::Initialize(const Configuration& configuration)
	{
		std::lock_guard<std::mutex> guard(dataMutex_);

		if (isInitialized_)
		{
			ATOMIC_ASSERT(false);
			return;
		}

		CreateThreads(configuration.blockingThreads_, configuration.nonBlockingThreads_, configuration.schedulingThreads_);

		isInitialized_ = true;
		ATOMIC_LOGINFO("Initialized TasksQueue");
	}
	void TasksQueue::ShutDown()
	{
		if (!isInitialized_)
		{
			return;
		}

		shutDown_ = true;

		tasksCondition_.notify_all();
		scheduleCondition_.notify_all();
		for (auto thread : threads_)
		{
			thread->Stop();
		}
		for (auto thread : schedulingThreads_)
		{
			thread->Stop();
		}

		{
			std::lock_guard<std::mutex> guard(dataMutex_);
			threads_.Clear();
			isInitialized_ = false;
		}
	}

	bool TasksQueue::AddTask(TaskPtr task)
	{
		++stats_.added_;
		std::unique_lock<std::mutex> lock(task->GetDataMutex_());
		return AddTask(task, std::move(lock));
	};
	bool TasksQueue::AddTask(TaskPtr task, const std::unique_lock<std::mutex> lockTaskData)
	{
		if (!task)
		{
			return false;
		}

		ATOMIC_ASSERT(isInitialized_);

		if (task->suspendTime_ > std::chrono::milliseconds(0))
		{
			{
				std::unique_lock<std::mutex> lockSched(schedulerMutex_);
				scheduledTasks_.insert(schedulePair(scheduleClock::now() + task->suspendTime_, task));
				++stats_.suspended_;
				++stats_.waiting_;
			}

			scheduleEarliest_ = scheduleTimePoint::min();
			scheduleCondition_.notify_one();
		}
		else
		{
			if (!task->IsMainThread_())
			{
				{
					std::lock_guard<std::mutex> lockTask(tasksMutex_);
					tasks_.Push(task);
				}

				tasksCondition_.notify_all();
			}
			else
			{
				std::lock_guard<std::mutex> lockResponse(mtTasksMutex_);
				mtTasks_.Push(task);
			}

			task->status_ = TaskStatus::TASK_IN_QUEUE;

			if (task->GetPriority_() > runningPriority_)
			{
				runningPriority_ = task->GetPriority_();
			}
		}

		++stats_.total_;
		return true;
	}

	void TasksQueue::ThreadExecuteTasks(const uint32_t threadNum, const bool ignoreBlocking)
	{
		ATOMIC_LOGDEBUGF("Thread %d starting: %s", threadNum, (ignoreBlocking ? "non-blocking" : "blocking"));
		for (;;)
		{
			std::shared_ptr<Task> task = nullptr;
			{
				std::unique_lock<std::mutex> lockTasks(tasksMutex_);

				while (!shutDown_ && tasks_.Empty())
				{
					tasksCondition_.wait(lockTasks);
				}

				if (shutDown_)
				{
					ATOMIC_LOGINFOF("Thread %d: shut down", threadNum);
					break;
				}

				task = tasks_.Front();
				if (task)
				{
					std::lock_guard<std::mutex> lockTaskData(task->GetDataMutex_());
					if ((task->IsBlocking_() && ignoreBlocking)
						|| (task->GetPriority_() < runningPriority_)
					   )
					{
						task = nullptr;
					}
					else
					{
						tasks_.PopFront();
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
	void TasksQueue::ThreadExecuteScheduledTasks(const uint32_t threadNum)
	{
		ATOMIC_LOGDEBUGF("Thread %d (scheduling) starting", threadNum);
		for (;;)
		{
			Atomic::List<std::shared_ptr<Task>> runTasks;

			{
				std::unique_lock<std::mutex> lockSched(schedulerMutex_);

				while (!shutDown_ && scheduleEarliest_.load() > scheduleClock::now())
				{
					scheduleCondition_.wait(lockSched);
				}

				if (shutDown_)
				{
					ATOMIC_LOGINFOF("Thread %d (scheduling): shut down", threadNum);
					break;
				}

				auto now = scheduleClock::now();
				auto it = scheduledTasks_.begin();
				while ((it != scheduledTasks_.end()) && (it->first < now))
				{
					auto task = it->second;
					std::unique_lock<std::mutex> lockTask(task->GetDataMutex_());
					task->suspendTime_ = std::chrono::milliseconds(0);
					runTasks.Push(task);
					ATOMIC_LOGDEBUGF("Thread %d (scheduling): resume task", threadNum);
					it = scheduledTasks_.erase(it);
				}

				scheduleEarliest_ = (it != scheduledTasks_.end()) ? it->first : scheduleTimePoint::max();
			}

			while (!runTasks.Empty())
			{
				auto task = runTasks.Front();
				++stats_.resumed_;
				--stats_.waiting_;
				--stats_.total_;		// AddTask will increase this back
				AddTask(task);
				runTasks.PopFront();
			}
		}
	}
	void TasksQueue::Update()
	{
		if (shutDown_)
		{
			return;
		}

		if (scheduleEarliest_.load() <= scheduleClock::now())
		{
			scheduleCondition_.notify_one();
		}

		std::shared_ptr<Task> task = nullptr;
		{
			Atomic::List<std::shared_ptr<Task>> runTasks;

			{
				std::lock_guard<std::mutex> lockTasks(mtTasksMutex_);
				if (mtTasks_.Empty())
				{
					return;
				}

				while (!mtTasks_.Empty())
				{
					task = mtTasks_.Front();
					mtTasks_.PopFront();

					if (task)
					{
						std::lock_guard<std::mutex> lockTaskData(task->GetDataMutex_());

						if (task->GetPriority_() >= runningPriority_)
						{
							runTasks.Push(task);
						}
						else
						{
							--stats_.total_;
						}
					}
				}
			}

			while (!runTasks.Empty())
			{
				task = runTasks.Front();
				task->Execute(this, task);
				RescheduleTask(task);
				runTasks.PopFront();
			}
		}
	}

	void TasksQueue::CreateThreads(const unsigned count, const unsigned countNonBlocking, const unsigned countScheduling)
	{
#ifdef ATOMIC_THREADING
		uint32_t i;
		for (i = 0; i < countNonBlocking; ++i)
		{
			auto thread = std::make_shared<TaskThread>(*this, i, true);
			threads_.Push(thread);
			thread->Run();
		}
		for (; i < count + countNonBlocking; ++i)
		{
			auto thread = std::make_shared<TaskThread>(*this, i, false);
			threads_.Push(thread);
			thread->Run();
		}
		for (; i < count + countNonBlocking + countScheduling; ++i)
		{
			auto thread = std::make_shared<TaskScheduleThread>(*this, i);
			schedulingThreads_.Push(thread);
			thread->Run();
		}
#else
		ATOMIC_LOGERROR("Can not create request threads as threading is disabled");
#endif
	}
	void TasksQueue::RescheduleTask(std::shared_ptr<Task> task)
	{
		--stats_.total_;

		std::unique_lock<std::mutex> lockTaskData(task->GetDataMutex_());
		if (task->doReschedule_)
		{
			task->ApplyReschedule_();
			AddTask(task, std::move(lockTaskData));
		}
		else
		{
			if (task->priority_ > 0)
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
		// stats.waiting_ = reset ? stats_.waiting_.exchange(0) : stats_.waiting_.load();
		// stats.total_ = reset ? stats_.total_.exchange(0) : stats_.total_.load();

		return stats;
	}


	// ===== TasksQueueAtomic ===========================================================
	TasksQueueAtomic::TasksQueueAtomic(Atomic::Context* context)
		: Object(context)
		, TasksQueue()
	{
		SubscribeToEvent(Atomic::E_UPDATE, ATOMIC_HANDLER(TasksQueueAtomic, HandleUpdate));
	}
	TasksQueueAtomic::~TasksQueueAtomic()
	{
	}

	void TasksQueueAtomic::HandleUpdate(Atomic::StringHash eventType, Atomic::VariantMap& eventData)
	{
		Update();
	}


	// ===== TasksQueueContainer ========================================================
	TasksQueuesContainer::TasksQueuesContainer(Atomic::Context* context)
		: Object(context)
		, isInitialized_(false)
	{
		SubscribeToEvent(Atomic::E_UPDATE, ATOMIC_HANDLER(TasksQueuesContainer, HandleUpdate));
	}
	TasksQueuesContainer::~TasksQueuesContainer()
	{
		ShutDown();
	}

	void TasksQueuesContainer::Initialize()
	{
		ATOMIC_ASSERT(!isInitialized_);

		isInitialized_ = true;
		ATOMIC_LOGINFO("Initialized TasksQueueContainer");
	}

	void TasksQueuesContainer::ShutDown()
	{
		isInitialized_ = false;

		for (auto& it : queueMap_)
		{
			it.second.ShutDown();
		}
	}

	TasksQueue& TasksQueuesContainer::GetQueue(const std::string& queueName)
	{
		ATOMIC_ASSERT(isInitialized_);

		auto queueIt = queueMap_.find(queueName);
		ATOMIC_ASSERT(queueIt != queueMap_.end());

		return queueIt->second;
	}

	void TasksQueuesContainer::CreateQueue(const std::string& queueName, const TasksQueue::Configuration& configuration)
	{
		auto queueIt = queueMap_.find(queueName);
		AssertReturnUnless(queueIt == queueMap_.end());

		auto pair = queueMap_.emplace(std::piecewise_construct, std::forward_as_tuple(queueName), std::forward_as_tuple());
		queueIt = pair.first;
		queueIt->second.Initialize(configuration);
	}

	void TasksQueuesContainer::HandleUpdate(Atomic::StringHash eventType, Atomic::VariantMap& eventData)
	{
		for (auto it = queueMap_.begin(); it != queueMap_.end(); ++it)
		{
			it->second.Update();
		}
	}

}
