#pragma once

#include <mutex>
#include <memory>
#include <string>
#include <chrono>

#define TASK_FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace TasksLib
{
	using BlockingType = bool;
	enum ThreadTarget
	{
		MAIN_THREAD,
		WORKER_THREAD
	};
	enum TaskStatus
	{
		TASK_INIT,
		TASK_SUSPENDED,
		TASK_IN_QUEUE,
		TASK_WORKING,

		TASK_FINISHED = -100
	};

	class TasksQueue;
	class Task;

	using TaskPtr = std::shared_ptr<Task>;
	using TaskWeakPtr = std::weak_ptr<Task>;
	using TaskExecutable = std::function<void(TasksQueue* queue, TaskPtr task)>;

	using TaskPriority = uint32_t;

	class Task
	{
	public:
		Task(const bool isBlocking = false, const TaskPriority priority = 0, const bool isMainThread = false);
		Task(const TaskExecutable executable, const bool isBlocking = false, const TaskPriority priority = 0, const bool isMainThread = false);
		virtual ~Task();

		const uint32_t GetPriority();
		const bool IsBlocking();
		const bool IsMainThread();

		const TaskStatus GetStatus();

		template <typename... Ts>
		void Reschedule(Ts&& ... ts);

	protected:
		std::mutex& GetDataMutex_();
		const uint32_t GetPriority_();
		const bool IsBlocking_();
		const bool IsMainThread_();

		void Execute(TasksQueue* queue, std::shared_ptr<Task> task);
		void ResetReschedule_(std::unique_lock<std::mutex> lock);
		void ApplyReschedule_();

		template <typename T>
		void SetRescheduleOption_(T&& t);
		template <typename T, typename... Ts>
		void SetRescheduleOption_(T&& t, Ts&& ... ts);
		void RescheduleOption_(const BlockingType& isBlocking);
		void RescheduleOption_(const BlockingType&& isBlocking);
		void RescheduleOption_(const ThreadTarget&& thread);
		void RescheduleOption_(const TaskPriority&& priority);
		void RescheduleOption_(const TaskExecutable executable);
		void RescheduleOption_(const std::chrono::milliseconds& ms);
		void RescheduleOption_(const std::chrono::milliseconds&& ms);

	protected:
		std::mutex dataMutex_;
		TaskPriority priority_;
		bool isBlocking_;
		bool isMainThread_;
		TaskStatus status_;
		TaskExecutable executable_;
		std::chrono::milliseconds suspendTime_;

		bool doReschedule_;
		bool rescheduleBlocking_;
		bool rescheduleMainThread_;
		TaskPriority reschedulePriority_;
		TaskExecutable rescheduleExecutable_;
		std::chrono::milliseconds rescheduleSuspendTime_;

		friend class TasksQueue;
	};

	template <typename... Ts>
	void Task::Reschedule(Ts&& ... ts)
	{
		std::lock_guard<std::mutex> lock(dataMutex_);
		SetRescheduleOption_(TASK_FWD(ts)...);
		doReschedule_ = true;
	}
	template <typename T>
	void Task::SetRescheduleOption_(T&& t)
	{
		RescheduleOption_(TASK_FWD(t));
	}
	template <typename T, typename... Ts>
	void Task::SetRescheduleOption_(T&& t, Ts&& ... ts)
	{
		SetRescheduleOption_(TASK_FWD(t));
		SetRescheduleOption_(TASK_FWD(ts)...);
	}


	// ====== TaskWithData ==============================================================
	template<class T>
	std::shared_ptr<T> reinterpret_task_cast(TaskPtr task)
	{
		return std::shared_ptr<T>(task, reinterpret_cast<T*>(task.get()));
	}

	template <class T>
	class TaskWithData : public Task
	{
	public:
		TaskWithData(const bool isBlocking = false, const TaskPriority priority = 0, const bool isMainThread = false);
		TaskWithData(const TaskExecutable executable, const bool isBlocking = false, const TaskPriority priority = 0, const bool isMainThread = false);
		virtual ~TaskWithData();

		std::shared_ptr<T> GetData();
		void SetData(std::shared_ptr<T> data);

	private:
		std::shared_ptr<T> data_;
	};

	template <class T>
	TaskWithData<T>::TaskWithData(const bool isBlocking, const TaskPriority priority, const bool isMainThread)
		: Task(isBlocking, priority, isMainThread)
	{
	}
	template <class T>
	TaskWithData<T>::TaskWithData(const TaskExecutable executable, const bool isBlocking, const TaskPriority priority, const bool isMainThread)
		: Task(executable, isBlocking, priority, isMainThread)
	{
	}
	template <class T>
	TaskWithData<T>::~TaskWithData()
	{
	}

	template <class T>
	std::shared_ptr<T> TaskWithData<T>::GetData()
	{
		std::lock_guard<std::mutex> lock(dataMutex_);

		return data_;
	}
	template <class T>
	void TaskWithData<T>::SetData(std::shared_ptr<T> data)
	{
		std::lock_guard<std::mutex> lock(dataMutex_);

		data_ = data;
	}
}
