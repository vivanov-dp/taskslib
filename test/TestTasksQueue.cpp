#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "Types.h"
#include "TestTools.h"
#include "TasksQueue.h"

namespace TasksLib {

	using namespace ::testing;

	class TasksQueueTest : public TestWithRandom {
	public:
		TasksQueue queue{};

		void InitQueue() {
			queue.Initialize({ 3,2,1 });
		}
	};
	TEST_F(TasksQueueTest, Creates) {
		EXPECT_FALSE(queue.isInitialized());
		EXPECT_FALSE(queue.isShutDown());

		EXPECT_EQ(queue.numWorkerThreads(), 0);
		EXPECT_EQ(queue.numBlockingThreads(), 0);
		EXPECT_EQ(queue.numNonBlockingThreads(), 0);
		EXPECT_EQ(queue.numSchedulingThreads(), 0);
	}
	TEST_F(TasksQueueTest, Initializes) {
		std::uniform_int_distribution<unsigned> random(1, 15);
		unsigned blocking = random(randEng);
		unsigned nonBlocking = random(randEng);
		unsigned scheduling = random(randEng);

		queue.Initialize({ blocking, nonBlocking, scheduling });

		EXPECT_TRUE(queue.isInitialized());
		EXPECT_EQ(queue.numWorkerThreads(), blocking + nonBlocking);
		EXPECT_EQ(queue.numBlockingThreads(), blocking);
		EXPECT_EQ(queue.numNonBlockingThreads(), nonBlocking);
		EXPECT_EQ(queue.numSchedulingThreads(), scheduling);
	}
	TEST_F(TasksQueueTest, ShutsDown) {
		std::uniform_int_distribution<unsigned> random(1, 15);
		unsigned blocking = random(randEng);
		unsigned nonBlocking = random(randEng);
		unsigned scheduling = random(randEng);

		queue.Initialize({ blocking, nonBlocking, scheduling });

		ASSERT_TRUE(queue.isInitialized());

		queue.ShutDown();

		EXPECT_TRUE(queue.isShutDown());
		EXPECT_FALSE(queue.isInitialized());
		EXPECT_EQ(queue.numWorkerThreads(), 0);
		EXPECT_EQ(queue.numBlockingThreads(), 0);
		EXPECT_EQ(queue.numNonBlockingThreads(), 0);
		EXPECT_EQ(queue.numSchedulingThreads(), 0);
	}

}
