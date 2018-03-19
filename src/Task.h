#pragma once

#include <mutex>
#include <string>
#include <chrono>

#include "Types.h"
#include "TaskOptions.h"

namespace TasksLib {

	class Task {
	public:
		Task();
		/*
		   Creates Task with the specified set of options
		   Usage: Task( TaskPriority{10}, [&](TasksQueue* queue, TaskPtr task)->void { }, ... );
		*/
		template <typename... Ts>
		explicit Task(Ts&& ...Ts);
		virtual ~Task();

		const TaskStatus GetStatus() const;
		TaskOptions const& GetOptions() const;
		TaskOptions const& GetRescheduleOptions() const;
		const bool WillReschedule() const;

		template <typename... Ts>
		void Reschedule(Ts&& ...ts);

	protected:
		std::mutex	dataMutex_;

	protected:
		std::mutex& GetDataMutex_();
		void ApplyReschedule_(std::unique_lock<std::mutex> lock);
		void ResetReschedule_(std::unique_lock<std::mutex> lock);
		void Execute(TasksQueue* queue, TaskPtr task);

	private:
		TaskStatus	status_;
		TaskOptions	options_;
		TaskOptions	rescheduleOptions_;
		bool		doReschedule_;

	private:
		void ApplyReschedule_();
		void ResetReschedule_();

		friend class TasksQueue;
		friend class TaskTest;
	};

	template <typename... Ts>
	Task::Task(Ts&& ...ts) 
		: Task()
	{
		options_.SetOptions(std::forward<Ts>(ts)...);
	}
	template <typename... Ts>
	void Task::Reschedule(Ts&& ... ts) {
		std::lock_guard<std::mutex> lock(dataMutex_);

		rescheduleOptions_.SetOptions(std::forward<Ts>(ts)...);
		doReschedule_ = true;
	}



	// ====== TaskWithData ==============================================================

	template <class T>
	class TaskWithData : public Task {
	public:
		virtual ~TaskWithData();

		std::shared_ptr<T> GetData();
		void SetData(std::shared_ptr<T> data);

	private:
		std::shared_ptr<T> data_;
	};

	template <class T>
	TaskWithData<T>::~TaskWithData() {}

	template <class T>
	std::shared_ptr<T> TaskWithData<T>::GetData() {
		std::lock_guard<std::mutex> lock(dataMutex_);

		return data_;
	}
	template <class T>
	void TaskWithData<T>::SetData(std::shared_ptr<T> data) {
		std::lock_guard<std::mutex> lock(dataMutex_);

		data_ = data;
	}
}
