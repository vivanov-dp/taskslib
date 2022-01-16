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
		/* Creates Task with the specified set of options
		   Usage: Task( TaskPriority{10}, [&](TasksQueue* queue, TaskPtr task)->void { }, ... );
		*/
		template <typename... Ts> Task(Ts&& ...opts);
		virtual ~Task();

        [[maybe_unused]] [[nodiscard]] TaskStatus GetStatus() const;
        [[maybe_unused]] [[nodiscard]] TaskOptions const& GetOptions() const;
        [[maybe_unused]] [[nodiscard]] TaskOptions const& GetRescheduleOptions() const;
        [[maybe_unused]] [[nodiscard]] bool WillReschedule() const;

		/* Sets the task up for another run through the task queue with a new set of options.
		   This method can be used by the task callback to keep the task going.
		   If Reschedule( ... ) is not called, the task is complete and is removed from the queue.
		*/
		template <typename... Ts> void Reschedule(Ts&& ...opts);

	protected:
		std::mutex& GetTaskMutex_();

		void Execute(TasksQueue* queue, TaskPtr task);

	private:
		TaskStatus	_status;
		TaskOptions	_options;
		TaskOptions	_rescheduleOptions;
		bool		_doReschedule;

	private:
		std::mutex	_taskMutex;
		void ApplyReschedule_();
		void ResetReschedule_();

		friend class TasksQueue;
        friend class TaskTest;			// To enable tests to call ApplyReschedule_() & ResetReschedule_(), which are called by TasksQueue
	};

    template <> void Task::Reschedule();
    // This has to be defined in the .h
    template <typename... Ts> Task::Task(Ts&& ...ts)
            : Task()
    {
        _options.SetOptions(std::forward<Ts>(ts)...);
    }



	// ====== TaskWithData ==============================================================

	template <class T> class TaskWithData : public Task {
	public:
		~TaskWithData() override;

        [[maybe_unused]] std::shared_ptr<T> GetData();
        [[maybe_unused]] void SetData(std::shared_ptr<T> data);

	private:
		std::mutex			dataMutex_;
		std::shared_ptr<T>	data_;
	};

	template <class T> TaskWithData<T>::~TaskWithData() = default;

	template <class T> [[maybe_unused]] std::shared_ptr<T> TaskWithData<T>::GetData() {
		std::lock_guard<std::mutex> lock(dataMutex_);

		return data_;
	}
	template <class T> [[maybe_unused]] void TaskWithData<T>::SetData(std::shared_ptr<T> data) {
		std::lock_guard<std::mutex> lock(dataMutex_);

		data_ = data;
	}
}
