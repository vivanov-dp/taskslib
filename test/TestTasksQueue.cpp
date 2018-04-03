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
		TasksQueue queue;

		TasksQueueTest() 
			: queue()
		{
			InitQueue();
		}

		void InitQueue() {
			queue.Initialize({ 3,2,1 });
			ASSERT_TRUE(queue.isInitialized());
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
		TasksQueue checkQueue;

		EXPECT_FALSE(checkQueue.isInitialized());
		EXPECT_FALSE(checkQueue.isShutDown());

		EXPECT_EQ(checkQueue.numWorkerThreads(), 0);
		EXPECT_EQ(checkQueue.numBlockingThreads(), 0);
		EXPECT_EQ(checkQueue.numNonBlockingThreads(), 0);
		EXPECT_EQ(checkQueue.numSchedulingThreads(), 0);
	}
	TEST_F(TasksQueueTest, Initializes) {
		std::uniform_int_distribution<unsigned> random(1, 15);
		unsigned blocking = random(randEng);
		unsigned nonBlocking = random(randEng);
		unsigned scheduling = random(randEng);

		TasksQueue checkQueue;
		checkQueue.Initialize({ blocking, nonBlocking, scheduling });

		EXPECT_TRUE(checkQueue.isInitialized());
		EXPECT_EQ(checkQueue.numWorkerThreads(), blocking + nonBlocking);
		EXPECT_EQ(checkQueue.numBlockingThreads(), blocking);
		EXPECT_EQ(checkQueue.numNonBlockingThreads(), nonBlocking);
		EXPECT_EQ(checkQueue.numSchedulingThreads(), scheduling);

		CheckStats(0,0,0,0,0,0);
	}
	TEST_F(TasksQueueTest, ShutsDown) {
		std::uniform_int_distribution<unsigned> random(1, 15);
		unsigned blocking = random(randEng);
		unsigned nonBlocking = random(randEng);
		unsigned scheduling = random(randEng);

		TasksQueue checkQueue;
		checkQueue.Initialize({ blocking, nonBlocking, scheduling });

		ASSERT_TRUE(checkQueue.isInitialized());

		checkQueue.ShutDown();

		EXPECT_TRUE(checkQueue.isShutDown());
		EXPECT_FALSE(checkQueue.isInitialized());
		EXPECT_EQ(checkQueue.numWorkerThreads(), 0);
		EXPECT_EQ(checkQueue.numBlockingThreads(), 0);
		EXPECT_EQ(checkQueue.numNonBlockingThreads(), 0);
		EXPECT_EQ(checkQueue.numSchedulingThreads(), 0);
	}
	TEST_F(TasksQueueTest, AddsTaskWorkerThread) {
		bool threadSet = false;

		queue.AddTask(
			std::make_shared<Task>(
				[&threadSet](TasksQueue* queue, TaskPtr task) -> void {
					threadSet = true;
				}
			)
		);
		CheckStats(1, -1, -1, -1, -1, -1, "Should add the task");

		auto now = std::chrono::steady_clock::now();
		do {
			std::this_thread::yield();
		} while (!threadSet && (std::chrono::steady_clock::now() < now + std::chrono::milliseconds(50)));

		EXPECT_TRUE(threadSet);
		CheckStats(1, 1, -1, -1, -1, 0, "Should run in a worker thread");
	}
	TEST_F(TasksQueueTest, AddsTaskMainThread) {
		bool threadSet = false;

		queue.AddTask(
			std::make_shared<Task>(
				[&threadSet](TasksQueue* queue, TaskPtr task) -> void {
					threadSet = true;
				},
				TaskThreadTarget::MAIN_THREAD
			)
		);
		CheckStats(1, -1, -1, -1, -1, -1, "Should add the task");

		auto now = std::chrono::steady_clock::now();
		do {
			std::this_thread::yield();
		} while (!threadSet && (std::chrono::steady_clock::now() < (now + std::chrono::milliseconds(100))));

		EXPECT_FALSE(threadSet);
		CheckStats(1, -1, -1, -1, -1, 1, "Shouldn't run in a worker thread");

		now = std::chrono::steady_clock::now();
		do {
			queue.Update();
		} while (!threadSet && (std::chrono::steady_clock::now() < (now + std::chrono::milliseconds(50))));

		EXPECT_TRUE(threadSet);
		CheckStats(1, 1, -1, -1, -1, 0, "Should run in the main thread");
	}
	TEST_F(TasksQueueTest, IgnoresBlockingProperly) {
		bool threadSet = false;
		for (int i = 0; i < 4; i++) {
			queue.AddTask(
				std::make_shared<Task>(
					[](TasksQueue* queue, TaskPtr task) -> void {
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
					},
					TaskBlocking{ true }
				)
			);
		}
		queue.AddTask(
			std::make_shared<Task>(
				[&threadSet](TasksQueue* queue, TaskPtr task) -> void {
					threadSet = true;
				}
			)
		);
		CheckStats(5, -1, -1, -1, -1, -1, "Should add 5 tasks");

		std::this_thread::sleep_for(std::chrono::milliseconds(60));
		EXPECT_TRUE(threadSet);
		CheckStats(-1, 1, -1, -1, -1, 4, "Should have 1 task completed");
		
		std::this_thread::sleep_for(std::chrono::milliseconds(70));
		CheckStats(-1, 4, -1, -1, -1, 1, "Should have 4 tasks completed");

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		CheckStats(-1, 5, -1, -1, -1, 0, "Should have all tasks completed");
	}
	TEST_F(TasksQueueTest, ObservesPriorityInWorker) {
		bool prioritySet = false;
		bool threadSet = false;
		queue.AddTask(
			std::make_shared<Task>(
				[&prioritySet](TasksQueue* queue, TaskPtr task) -> void {
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					prioritySet = true;
				},
				TaskPriority{ 20 }
			)
		);
		queue.AddTask(
			std::make_shared<Task>(
				[&threadSet](TasksQueue* queue, TaskPtr task) -> void {
					threadSet = true;
				}
			)
		);

		CheckStats(2, -1, -1, -1, -1, 2, "Should have 2 tasks");
		std::this_thread::sleep_for(std::chrono::milliseconds(60));
		ASSERT_FALSE(prioritySet);
		EXPECT_FALSE(threadSet);
		
		std::this_thread::sleep_for(std::chrono::milliseconds(60));
		EXPECT_TRUE(threadSet);
	}
	TEST_F(TasksQueueTest, ObservesPriorityInMain) {
		bool prioritySet = false;
		bool threadSet = false;
		queue.AddTask(
			std::make_shared<Task>(
				[&prioritySet](TasksQueue* queue, TaskPtr task) -> void {
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					prioritySet = true;
				},
				TaskPriority{ 20 }
			)
		);
		queue.AddTask(
			std::make_shared<Task>(
				[&threadSet](TasksQueue* queue, TaskPtr task) -> void {
					threadSet = true;
				},
				TaskThreadTarget{ MAIN_THREAD }
			)
		);

		CheckStats(2, -1, -1, -1, -1, 2, "Should have 2 tasks");
		auto now = std::chrono::steady_clock::now();
		while (!prioritySet && (now + std::chrono::milliseconds(60) > std::chrono::steady_clock::now())) {
			queue.Update();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		ASSERT_FALSE(prioritySet);
		EXPECT_FALSE(threadSet);

		now = std::chrono::steady_clock::now();
		while (!prioritySet && (now + std::chrono::milliseconds(60) > std::chrono::steady_clock::now())) {
			queue.Update();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		ASSERT_TRUE(prioritySet);

		queue.Update();
		EXPECT_TRUE(threadSet);
	}
	TEST_F(TasksQueueTest, SetsDelay) {
		bool threadSet = false;
		queue.AddTask(
			std::make_shared<Task>(
				[&threadSet](TasksQueue* queue, TaskPtr task) -> void {
					threadSet = true;
				},
				TaskDelay{ 100 }
			)
		);

		CheckStats(1, 0, 1, 0, 1, 1, "Should have 1 task waiting");
		
		std::this_thread::sleep_for(std::chrono::milliseconds(60));
		queue.Update();
		EXPECT_FALSE(threadSet);
		CheckStats(1, 0, 1, 0, 1, 1, "Should still have 1 task waiting");

		std::this_thread::sleep_for(std::chrono::milliseconds(60));
		queue.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		EXPECT_TRUE(threadSet);
		CheckStats(1, 1, 1, 1, 0, 0, "Should have completed 1 task");
	}
	TEST_F(TasksQueueTest, ReschedulesWorkerToMain) {
		bool threadSet1 = false;
		bool threadSet2 = false;

		queue.AddTask(
			std::make_shared<Task>(
				[&threadSet1, &threadSet2](TasksQueue* queue, TaskPtr task) -> void {
					if (!threadSet1) {
						threadSet1 = true;
						task->Reschedule(TaskThreadTarget{ MAIN_THREAD });
					}
					else {
						threadSet2 = true;
					}
					
				}
			)
		);

		std::this_thread::sleep_for(std::chrono::milliseconds(30));
		ASSERT_TRUE(threadSet1);

		auto now = std::chrono::steady_clock::now();
		do {
			std::this_thread::yield();
		} while (!threadSet2 && (std::chrono::steady_clock::now() < now + std::chrono::milliseconds(50)));
		EXPECT_FALSE(threadSet2);
		CheckStats(1, 0, -1, -1, -1, 1, "Should not run in a worker thread");

		queue.Update();
		EXPECT_TRUE(threadSet2);
		CheckStats(1, 1, -1, -1, -1, 0, "Should finish the task in main thread");
	}
	TEST_F(TasksQueueTest, ReschedulesMainToWorker) {
		bool threadSet1 = false;
		bool threadSet2 = false;

		queue.AddTask(
			std::make_shared<Task>(
				[&threadSet1, &threadSet2](TasksQueue* queue, TaskPtr task) -> void {
					if (!threadSet1) {
						threadSet1 = true;
						task->Reschedule(TaskThreadTarget{ WORKER_THREAD });
					} else {
						threadSet2 = true;
					}
				},
				TaskThreadTarget{ MAIN_THREAD }
			)
		);

		queue.Update();
		ASSERT_TRUE(threadSet1);

		std::this_thread::sleep_for(std::chrono::milliseconds{ 30 });
		EXPECT_TRUE(threadSet2);
		CheckStats(1, 1, -1, -1, -1, 0, "Should finish the task in worker thread");
	}
	TEST_F(TasksQueueTest, ReschedulesToLowerPriority) {
		bool prioritySet = false;
		queue.AddTask(
			std::make_shared<Task>(
				[&prioritySet](TasksQueue* queue, TaskPtr task) -> void {
					std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
					prioritySet = true;
				},
				TaskPriority{ 25 }
			)
		);

		bool threadSet1 = false;
		bool threadSet2 = false;
		queue.AddTask(
			std::make_shared<Task>(
				[&threadSet1, &threadSet2](TasksQueue* queue, TaskPtr task) -> void {
					if (!threadSet1) {
						threadSet1 = true;
						task->Reschedule(TaskPriority{ 10 });
					} else {
						threadSet2 = true;
					}
				},
				TaskPriority{ 50 }
			)
		);

		CheckStats(2, 0, -1, -1, -1, 2, "Should have 2 tasks");
		
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		EXPECT_TRUE(threadSet1);
		EXPECT_FALSE(threadSet2);

		std::this_thread::sleep_for(std::chrono::milliseconds(60));
		CheckStats(2, 0, -1, -1, -1, 2, "Should have 2 tasks - step 2");
		EXPECT_TRUE(threadSet1);
		EXPECT_FALSE(threadSet2);

		std::this_thread::sleep_for(std::chrono::milliseconds(60));
		CheckStats(2, 2, -1, -1, -1, 0, "Should have completed 2 tasks");
		EXPECT_TRUE(threadSet1);
		EXPECT_TRUE(threadSet2);
	}
	TEST_F(TasksQueueTest, ReschedulesToHigherPriority) {
		bool prioritySet1 = false;
		bool prioritySet2 = false;
		queue.AddTask(
			std::make_shared<Task>(
				[&prioritySet1, &prioritySet2](TasksQueue* queue, TaskPtr task) -> void {
					if (!prioritySet1) {
						std::this_thread::sleep_for(std::chrono::milliseconds{ 30 });
						prioritySet1 = true;
						task->Reschedule(TaskPriority{ 50 });
					} else {
						std::this_thread::sleep_for(std::chrono::milliseconds{ 60 });
						prioritySet2 = true;
					}
				},
				TaskPriority{ 20 }
			)
		);

		// Task 1 will hold 30ms at priority 20, then 60ms at priority 50
		// Task 2 with priority 25 should complete immediately
		// After a total of 50ms Task 3 with priority 25 should remain in queue until Task 1 finishes

		bool threadSet1 = false;
		bool threadSet2 = false;
		queue.AddTask(
			std::make_shared<Task>(
				[&threadSet1](TasksQueue* queue, TaskPtr task) -> void {
					threadSet1 = true;
				},
				TaskPriority{ 25 }
			)
		);
		CheckStats(2, 0, -1, -1, -1, 2, "Should have 2 tasks");

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		CheckStats(-1, 1, -1, -1, -1, 1, "Should have completed 1 task");
		ASSERT_FALSE(prioritySet1);
		EXPECT_TRUE(threadSet1);
		EXPECT_FALSE(threadSet2);

		std::this_thread::sleep_for(std::chrono::milliseconds(40));
		queue.AddTask(
			std::make_shared<Task>(
				[&threadSet2](TasksQueue* queue, TaskPtr task) -> void {
					threadSet2 = true;
				},
				TaskPriority{ 25 }
			)
		);
		CheckStats(3, -1, -1, -1, -1, 2, "Should have 2 tasks - step 2");
		ASSERT_TRUE(prioritySet1);
		EXPECT_FALSE(threadSet2);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		EXPECT_FALSE(threadSet2);

		std::this_thread::sleep_for(std::chrono::milliseconds(60));
		CheckStats(-1, 3, -1, -1, -1, 0, "Should have completed 3 tasks");
		ASSERT_TRUE(prioritySet2);
		EXPECT_TRUE(threadSet2);
	}
}
