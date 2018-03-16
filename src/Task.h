#pragma once

#include <mutex>
#include <memory>
#include <string>
#include <chrono>

//#define TASK_FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

#include "Types.h"

namespace TasksLib {

	class Task {
	public:
		Task();
		template <typename... Ts>
		explicit Task(Ts&& ...Ts);
		virtual ~Task();

		const uint32_t GetPriority() const;
		const bool IsBlocking() const;
		const bool IsMainThread() const;
		const TaskStatus GetStatus() const;
		TaskExecutable const& GetExecutable() const;
		const std::chrono::milliseconds GetSuspendTime() const;




		template <typename... Ts>
		void Reschedule(Ts&& ... ts);

	protected:
		std::mutex& GetDataMutex_();

		void Execute(TasksQueue* queue, std::shared_ptr<Task> task);
		void ResetReschedule_(std::unique_lock<std::mutex> lock);

		template <typename T>
		void SetRescheduleOption_(T&& t);
		template <typename T, typename... Ts>
		void SetRescheduleOption_(T&& t, Ts&& ... ts);
		void RescheduleOption_(const TaskBlocking& isBlocking);
		void RescheduleOption_(const TaskBlocking&& isBlocking);
		void RescheduleOption_(const TaskThreadTarget&& thread);
		void RescheduleOption_(const TaskPriority&& priority);
		void RescheduleOption_(const TaskExecutable executable);
		void RescheduleOption_(const std::chrono::milliseconds& ms);
		void RescheduleOption_(const std::chrono::milliseconds&& ms);

	protected:
		std::mutex					dataMutex_;
		TaskPriority				priority_;
		bool						isBlocking_;
		bool						isMainThread_;
		TaskStatus					status_;
		TaskExecutable				executable_;
		std::chrono::milliseconds	suspendTime_;

		bool						doReschedule_;
		bool						rescheduleBlocking_;
		bool						rescheduleMainThread_;
		TaskPriority				reschedulePriority_;
		TaskExecutable				rescheduleExecutable_;
		std::chrono::milliseconds	rescheduleSuspendTime_;

	private:
		void ApplyReschedule_();
		void ResetReschedule_();

		friend class TasksQueue;
	};




	template <typename... Ts>
	Task::Task(Ts&& ...ts) 
		: Task()
	{
		SetRescheduleOption_(std::forward<Ts>(ts)...);
		ApplyReschedule_();
		ResetReschedule_();
	}






	template <typename... Ts>
	void Task::Reschedule(Ts&& ... ts)
	{
		std::lock_guard<std::mutex> lock(dataMutex_);
		SetRescheduleOption_(std::forward<Ts>(ts)...);
		doReschedule_ = true;
	}
	template <typename T>
	void Task::SetRescheduleOption_(T&& t)
	{
		RescheduleOption_(std::forward<T>(t));
	}
	template <typename T, typename... Ts>
	void Task::SetRescheduleOption_(T&& t, Ts&& ... ts)
	{
		SetRescheduleOption_(std::forward<T>(t));
		SetRescheduleOption_(std::forward<Ts>(ts)...);
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
