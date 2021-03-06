#pragma once

#include <chrono>

#include "Types.h"

namespace TasksLib {

	class TaskOptions {
	public:
		/* Creates TaskOptions with the default set of values */
		TaskOptions();
		TaskOptions(const TaskOptions& other);
		TaskOptions(TaskOptions&& other);
		/*
		   Creates TaskOptions with the specified set of values
		   Usage: TaskOptions( TaskPriority{10}, [&](TasksQueue* queue, TaskPtr task)->void { }, ... );
		*/
		template <typename... Ts>
		explicit TaskOptions(Ts&& ...opts);

		TaskPriority	priority;
		bool			isBlocking;
		bool			isMainThread;
		TaskExecutable	executable;
		TaskDelay		suspendTime;

	public:
		template <typename T>
		void SetOptions(T&& opt);
		template <typename T, typename... Ts>
		void SetOptions(T&& opt, Ts&& ... opts);

		TaskOptions& operator=(const TaskOptions& other);
		TaskOptions& operator=(TaskOptions&& other);

		/*
		   Equality operator. 
		   Note that std::function is uncomparable in C++ so executable is only half-matched (it will return false if 
		   one of the executables is nullptr or if their underlying types differ, but no more checks are performed)
		*/
		const bool operator==(const TaskOptions& other) const;
		/*
		   Inequality operator.
		   Note that std::function is uncomparable in C++ so executable is only half-matched (it will return true if
		   one of the executables is nullptr or if their underlying types differ, but no more checks are performed)
		*/
		const bool operator!=(const TaskOptions& other) const;

	private:
		void SetOption_(const TaskOptions& other);					// Copy and Move constructing sometimes matches the templated constructor and boils down to here
		void SetOption_(const TaskOptions&& other);					// Sometimes it doesn't and it is unclear when and why, so both implementations are provided
		void SetOption_(const TaskPriority& priority);
		void SetOption_(const TaskBlocking& isBlocking);
		void SetOption_(const TaskThreadTarget& threadTarget);
		void SetOption_(const TaskExecutable& executable);
		void SetOption_(TaskExecutable&& executable);
		void SetOption_(const TaskDelay& ms);
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
