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
		void CreateQueue(const std::string& queueName, const TasksQueue::Configuration& configuration);

		const size_t GetQueuesCount() const;

		void Update();

	private:
		std::unordered_map<std::string, TasksQueue> queueMap_;
	};
}
