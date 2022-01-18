#pragma once

#include <chrono>

#include "Types.h"

namespace TasksLib {

	class TaskOptions {
	public:
        TaskPriority	priority;
        bool			isBlocking;
        bool			isMainThread;
        TaskExecutable	executable;
        TaskDelay		suspendTime;

    public:
		/* Creates TaskOptions with the default set of values */
		TaskOptions() noexcept;
		TaskOptions(const TaskOptions& other) noexcept;
		TaskOptions(TaskOptions&& other) noexcept;
		/*
		   Creates TaskOptions with the specified set of values
		   Usage: TaskOptions( TaskPriority{10}, [&](TasksQueue* queue, TaskPtr task)->void { }, ... );
		*/
		template <typename... Ts> TaskOptions(Ts&& ...opts);

		template <typename T> void SetOptions(T&& opt);
		template <typename T, typename... Ts> [[maybe_unused]] void SetOptions(T&& opt, Ts&& ... opts);

		TaskOptions& operator=(const TaskOptions& other);
		TaskOptions& operator=(TaskOptions&& other) noexcept ;

		/*
		   Equality operator. 
		   Note that std::function is uncomparable in C++ so executable is only half-matched (it will return false if
		   one of the executables is nullptr or if their underlying types differ, but no more checks are performed)
		*/
		bool operator==(const TaskOptions& other) const;
		/*
		   Inequality operator.
		   Note that std::function is uncomparable in C++ so executable is only half-matched (it will return true if
		   one of the executables is nullptr or if their underlying types differ, but no more checks are performed)
		*/
		bool operator!=(const TaskOptions& other) const;

	private:
		void SetOption_(const TaskOptions& other);					// Copy and Move constructing sometimes matches the templated constructor and boils down to here
        [[maybe_unused]] void SetOption_(TaskOptions&& other);		// Sometimes it doesn't and it is unclear when and why, so both implementations are provided
        [[maybe_unused]] void SetOption_(const TaskPriority& priority);
        [[maybe_unused]] void SetOption_(const TaskBlocking& isBlocking);
        [[maybe_unused]] void SetOption_(const TaskThreadTarget& threadTarget);
        [[maybe_unused]] void SetOption_(const TaskExecutable& executable);
        [[maybe_unused]] void SetOption_(TaskExecutable&& executable);
        [[maybe_unused]] void SetOption_(const TaskDelay& ms);
	};



    // These have to be defined in the .h
    template <typename... Ts> TaskOptions::TaskOptions(Ts&& ...opts)
            : TaskOptions()
    {
        SetOptions(std::forward<Ts>(opts)...);
    }
    template <typename T> void TaskOptions::SetOptions(T&& opt)
    {
        SetOption_(std::forward<T>(opt));
    }
    template <typename T, typename... Ts> [[maybe_unused]] void TaskOptions::SetOptions(T&& opt, Ts&& ... opts)
    {
        SetOptions(std::forward<T>(opt));
        SetOptions(std::forward<Ts>(opts)...);
    }

}
