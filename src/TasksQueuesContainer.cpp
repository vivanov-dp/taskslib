#include "TasksQueuesContainer.h"

namespace TasksLib {

	TasksQueuesContainer::TasksQueuesContainer() {};
	TasksQueuesContainer::~TasksQueuesContainer() {
		for (auto& it : queueMap_) {
			it.second.ShutDown();
		}
	}

	TasksQueue* TasksQueuesContainer::GetQueue(const std::string& queueName) {
		auto queueIt = queueMap_.find(queueName);
		if (queueIt == queueMap_.end()) {
			return nullptr;
		}

		return &(queueIt->second);
	}
	void TasksQueuesContainer::CreateQueue(const std::string& queueName, const TasksQueue::Configuration& configuration) {
		auto queueIt = queueMap_.find(queueName);
		if (queueIt != queueMap_.end()) {
			return;
		}

		auto pair = queueMap_.emplace(std::piecewise_construct, std::forward_as_tuple(queueName), std::forward_as_tuple());
		queueIt = pair.first;
		queueIt->second.Initialize(configuration);
	}

	const size_t TasksQueuesContainer::GetQueuesCount() const {
		return queueMap_.size();
	}

	void TasksQueuesContainer::Update() {
		for (auto& it : queueMap_) {
			it.second.Update();
		}
	}
}
