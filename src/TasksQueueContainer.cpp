#include "TasksQueueContainer.h"

namespace TasksLib {

	TasksQueuesContainer::TasksQueuesContainer()
		: isInitialized_(false) {};
	TasksQueuesContainer::~TasksQueuesContainer() {
		ShutDown();
	}

	void TasksQueuesContainer::Initialize() {
		isInitialized_ = true;
	}

	void TasksQueuesContainer::ShutDown() {
		isInitialized_ = false;

		for (auto& it : queueMap_) {
			it.second.ShutDown();
		}
	}

	TasksQueue* TasksQueuesContainer::GetQueue(const std::string& queueName) {
		if (!isInitialized_) {
			return nullptr;
		}

		auto queueIt = queueMap_.find(queueName);
		if (queueIt == queueMap_.end()) {
			return nullptr;
		}

		return &(queueIt->second);
	}

	void TasksQueuesContainer::CreateQueue(const std::string& queueName, const TasksQueue::Configuration& configuration)
	{
		auto queueIt = queueMap_.find(queueName);
		if (queueIt != queueMap_.end()) {
			return;
		}

		auto pair = queueMap_.emplace(std::piecewise_construct, std::forward_as_tuple(queueName), std::forward_as_tuple());
		queueIt = pair.first;
		queueIt->second.Initialize(configuration);
	}

	void TasksQueuesContainer::Update()
	{
		for (auto it = queueMap_.begin(); it != queueMap_.end(); ++it)
		{
			it->second.Update();
		}
	}
}
