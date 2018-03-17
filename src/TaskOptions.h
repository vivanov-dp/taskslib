#pragma once

#include <chrono>

#include "Types.h"

namespace TasksLib {

	struct TaskOptions {
	public:
		TaskOptions();
		template <typename... Ts>
		explicit TaskOptions(Ts&& ...opts);

		TaskPriority				priority;
		bool						isBlocking;
		bool						isMainThread;
		TaskExecutable				executable;
		std::chrono::milliseconds	suspendTime;

	public:
		template <typename T>
		void SetOptions(T&& opt);
		template <typename T, typename... Ts>
		void SetOptions(T&& opt, Ts&& ... opts);

	private:
		void SetOption_(const TaskPriority& priority);
		void SetOption_(const TaskBlocking& isBlocking);
		void SetOption_(const TaskThreadTarget& threadTarget);
		void SetOption_(const TaskExecutable& executable);
		void SetOption_(const TaskExecutable&& executable);
		void SetOption_(const std::chrono::milliseconds& ms);
	};

	template <typename... Ts>
	TaskOptions::TaskOptions(Ts&& ...opts)
		: TaskOptions()
	{
		SetOptions(std::forward<Ts>(opts)...);
	}
	template <typename T>
	void TaskOptions::SetOptions(T&& opt)
	{
		SetOption_(std::forward<T>(opt));
	}
	template <typename T, typename... Ts>
	void TaskOptions::SetOptions(T&& opt, Ts&& ... opts)
	{
		SetOptions(std::forward<T>(opt));
		SetOptions(std::forward<Ts>(opts)...);
	}


}
