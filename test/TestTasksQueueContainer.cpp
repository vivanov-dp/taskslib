#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <chrono>
#include <thread>

#include "Types.h"
#include "TestTools.h"
#include "TasksQueueContainer.h"
#include "Task.h"
#include "TasksQueue.h"

namespace TasksLib {

	using namespace ::testing;

	class TasksQueuesContainerTest : public TestWithRandom {
	public:
		TasksQueuesContainer queuesContainer;
	};

	TEST_F(TasksQueuesContainerTest, CreatesQueues) {
		std::string queueName = GenerateRandomString(5, 8, randEng);

		EXPECT_EQ(queuesContainer.GetQueue(queueName), nullptr);
		queuesContainer.CreateQueue(queueName, { 3,2,1 });
		
		TasksQueue* queue = queuesContainer.GetQueue(queueName);
		ASSERT_NE(queue, nullptr);

		EXPECT_EQ(queue->numBlockingThreads(), 3);
		EXPECT_EQ(queue->numNonBlockingThreads(), 2);
		EXPECT_EQ(queue->numSchedulingThreads(), 1);
	}
	TEST_F(TasksQueuesContainerTest, UpdatesQueues) {
		std::uniform_int_distribution<unsigned> dist(5, 10);
		unsigned num = dist(randEng);
		std::vector<std::string> queueNames;

		for (unsigned i = 0; i < num; i++) {
			std::string name = GenerateRandomString(10, 15, randEng);
			queueNames.push_back(name);
			queuesContainer.CreateQueue(name, { 1,0,0 });
		}

		ASSERT_EQ(queuesContainer.GetQueuesCount(), num);

		unsigned count = 0;
		auto task = std::make_shared<Task>(
			[&count](TasksQueue* queue, TaskPtr task) -> void {
				count++;
			},
			TaskThreadTarget{ MAIN_THREAD }
		);

		// Adding the same task to multiple queues is NOT OK in real situations - task status is going to be inconsistent and rescheduling would 
		// result in unpredictable behavior, but it should work here
		for (auto& name : queueNames) {
			TasksQueue* queue = queuesContainer.GetQueue(name);
			queue->AddTask(task);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(30));
		EXPECT_EQ(count, 0);
		queuesContainer.Update();
		EXPECT_EQ(count, num);
	}
}
