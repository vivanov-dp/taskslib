#include "TasksQueuesContainer.h"

namespace TasksLib {

	TasksQueuesContainer::TasksQueuesContainer() = default;
	TasksQueuesContainer::~TasksQueuesContainer() {
		for (auto& it : queueMap_) {
            it.second.Cleanup();
		}
	}

	TasksQueue* TasksQueuesContainer::GetQueue(const std::string& queueName) {
		auto queueIt = queueMap_.find(queueName);
		if (queueIt == queueMap_.end()) {
			return nullptr;
		}

		return &(queueIt->second);
	}

    [[maybe_unused]] void TasksQueuesContainer::CreateQueue(const std::string& queueName, const TasksQueue::Configuration& configuration) {
		auto queueIt = queueMap_.find(queueName);
		if (queueIt != queueMap_.end()) {
			return;
		}

		auto pair = queueMap_.emplace(std::piecewise_construct, std::forward_as_tuple(queueName), std::forward_as_tuple());
		queueIt = pair.first;
		queueIt->second.Initialize(configuration);
	}

    [[maybe_unused]] size_t TasksQueuesContainer::GetQueuesCount() const {
		return queueMap_.size();
	}

    [[maybe_unused]] void TasksQueuesContainer::Update() {
		for (auto& it : queueMap_) {
			it.second.Update();
		}
	}
}
