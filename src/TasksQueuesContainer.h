#pragma once

#include <unordered_map>

#include "Types.h"
#include "TasksQueue.h"

namespace TasksLib {

	class TasksQueuesContainer {
	public:
		TasksQueuesContainer();
		virtual ~TasksQueuesContainer();

		TasksQueue* GetQueue(const std::string& queueName);
        [[maybe_unused]] void CreateQueue(const std::string& queueName, const TasksQueue::Configuration& configuration);
        [[maybe_unused]] size_t GetQueuesCount() const;
        [[maybe_unused]] void Update();

	private:
		std::unordered_map<std::string, TasksQueue> queueMap_;
	};
}
