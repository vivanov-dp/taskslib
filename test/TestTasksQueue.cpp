#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <chrono>

#include "Types.h"
#include "TestTools.h"
#include "TasksQueue.h"
#include "Task.h"

namespace TasksLib {

	using namespace ::testing;

	class TasksQueueTest : public TestWithRandom {
	public:
		TasksQueue queue{};

		void InitQueue() {
			queue.Initialize({ 3,2,1 });
		}
		// Check queue.stats: -1 is skip, >=0 is EXPECT_EQ()
		void CheckStats(int added = -1, int completed = -1, int suspended = -1, int resumed = -1, int waiting = -1, int total = -1, std::string helper = "") {
			TasksQueuePerformanceStats<std::int32_t> stats = queue.GetPerformanceStats();
			if (added >= 0) {
				EXPECT_EQ(stats.added, added) << helper;
			}
			if (completed >= 0) {
				EXPECT_EQ(stats.completed, completed) << helper;
			}
			if (suspended >= 0) {
				EXPECT_EQ(stats.suspended, suspended) << helper;
			}
			if (resumed >= 0) {
				EXPECT_EQ(stats.resumed, resumed) << helper;
			}
			if (waiting >= 0) {
				EXPECT_EQ(stats.waiting, waiting) << helper;
			}
			if (total >= 0) {
				EXPECT_EQ(stats.total, total) << helper;
			}
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

		CheckStats(0,0,0,0,0,0);
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
	TEST_F(TasksQueueTest, AddsTaskWorkerThread) {
		InitQueue();
		ASSERT_TRUE(queue.isInitialized());

		bool threadSet = false;

		queue.AddTask(
			std::make_shared<Task>(
				[&threadSet](TasksQueue* queue, TaskPtr task) -> void {
					threadSet = true;
				}
			)
		);
		CheckStats(1, -1, -1, -1, -1, 1, "Should add the task");

		auto now = std::chrono::steady_clock::now();
		do {
			std::this_thread::yield();
		} while (!threadSet && (std::chrono::steady_clock::now() < now + std::chrono::milliseconds(100)));

		EXPECT_TRUE(threadSet);
		CheckStats(1, 1, 0, 0, 0, 0, "Should run in a worker thread");
	}
	TEST_F(TasksQueueTest, AddsTaskMainThread) {
		InitQueue();
		ASSERT_TRUE(queue.isInitialized());

		bool threadSet = false;

		queue.AddTask(
			std::make_shared<Task>(
				[&threadSet](TasksQueue* queue, TaskPtr task) -> void {
					threadSet = true;
				},
				TaskThreadTarget::MAIN_THREAD
			)
		);
		CheckStats(1, -1, -1, -1, -1, 1, "Should add the task");

		auto now = std::chrono::steady_clock::now();
		do {
			std::this_thread::yield();
		} while (!threadSet && (std::chrono::steady_clock::now() < (now + std::chrono::milliseconds(200))));

		EXPECT_FALSE(threadSet);
		CheckStats(1, -1, -1, -1, -1, 1, "Shouldn't run in a worker thread");

		now = std::chrono::steady_clock::now();
		do {
			queue.Update();
		} while (!threadSet && (std::chrono::steady_clock::now() < (now + std::chrono::milliseconds(100))));

		EXPECT_TRUE(threadSet);
		CheckStats(1, 1, 0, 0, 0, 0, "Should run in the main thread");
	}

}
