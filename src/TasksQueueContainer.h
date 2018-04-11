#pragma once

#include <unordered_map>

#include "Types.h"
#include "TasksQueue.h"

namespace TasksLib {

	class TasksQueuesContainer {
	public:
		TasksQueuesContainer();
		virtual ~TasksQueuesContainer();

		void Initialize();
		void ShutDown();

		TasksQueue* GetQueue(const std::string& queueName);
		void CreateQueue(const std::string& queueName, const TasksQueue::Configuration& configuration);

		void Update();

	private:
		bool isInitialized_;

		std::unordered_map<std::string, TasksQueue> queueMap_;
	};
}
