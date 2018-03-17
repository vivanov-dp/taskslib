#pragma once

#include <chrono>

#include "Types.h"

namespace TasksLib {

	struct TaskOptions {
	public:
		TaskOptions();

		TaskPriority				priority_;
		bool						isBlocking_;
		bool						isMainThread_;
		TaskExecutable				executable_;
		std::chrono::milliseconds	suspendTime_;
	};

}
