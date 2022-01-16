#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <chrono>
#include <thread>

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
			TasksQueuePerformanceStats<std::uint32_t> stats = queue.GetPerformanceStats();
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
	TEST_F(TasksQueueTest, CreatesWithConfig) {
		std::uniform_int_distribution<unsigned> random(1, 15);
		uint16_t blocking = random(randEng);
		uint16_t nonBlocking = random(randEng);
		uint16_t scheduling = random(randEng);

		TasksQueue checkQueue({ blocking, nonBlocking, scheduling });
		
		EXPECT_TRUE(checkQueue.isInitialized());
		EXPECT_EQ(checkQueue.numWorkerThreads(), blocking + nonBlocking);
		EXPECT_EQ(checkQueue.numBlockingThreads(), blocking);
		EXPECT_EQ(checkQueue.numNonBlockingThreads(), nonBlocking);
		EXPECT_EQ(checkQueue.numSchedulingThreads(), scheduling);

		CheckStats(0, 0, 0, 0, 0, 0);
	}
	TEST_F(TasksQueueTest, Initializes) {
		std::uniform_int_distribution<unsigned> random(1, 15);
		uint16_t blocking = random(randEng);
		uint16_t nonBlocking = random(randEng);
		uint16_t scheduling = random(randEng);

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
		uint16_t blocking = random(randEng);
		uint16_t nonBlocking = random(randEng);
		uint16_t scheduling = random(randEng);

		TasksQueue checkQueue;
		checkQueue.Initialize({ blocking, nonBlocking, scheduling });

		ASSERT_TRUE(checkQueue.isInitialized());

		checkQueue.Cleanup();

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
				[&threadSet](TasksQueue* queue, const TaskPtr& task) -> void {
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
		bool ready = false;
		bool threadSet1 = false;
		bool threadSet2 = false;

		auto task = std::make_shared<Task>(
			[&ready, &threadSet1, &threadSet2](TasksQueue* queue, TaskPtr task) -> void {
				if (!ready) return;			// Try to minimize time spent in thread if the test is not ready
				if (!threadSet1) {
					threadSet1 = true;
					task->Reschedule(TaskThreadTarget{ MAIN_THREAD });
				}
				else {
					threadSet2 = true;
				}
					
			}
		);
		queue.AddTask(task);
		ASSERT_EQ(task->GetStatus(), TaskStatus::TASK_IN_QUEUE);		// This might spuriously fail if the thread yields immediately after AddTask()
		ready = true;

		std::this_thread::sleep_for(std::chrono::milliseconds(30));
		ASSERT_TRUE(threadSet1);
		EXPECT_FALSE(threadSet2);
		EXPECT_EQ(task->GetStatus(), TaskStatus::TASK_IN_QUEUE_MAIN_THREAD);
	}
	TEST_F(TasksQueueTest, ReschedulesMainToWorker) {
		bool threadSet1 = false;

		auto task = std::make_shared<Task>(
			[&threadSet1](TasksQueue* queue, TaskPtr task) -> void {
				if (!threadSet1) {
					threadSet1 = true;
					task->Reschedule(TaskThreadTarget{ WORKER_THREAD });
				} else {
					std::this_thread::sleep_for(std::chrono::milliseconds(30));
					task->Reschedule();
				}
			},
			TaskThreadTarget{ MAIN_THREAD }
		);
		queue.AddTask(task);
		queue.Update();
		ASSERT_TRUE(threadSet1);
		EXPECT_TRUE((task->GetStatus() == TaskStatus::TASK_IN_QUEUE) || (task->GetStatus() == TaskStatus::TASK_WORKING));
		EXPECT_FALSE(task->GetOptions().isMainThread);
	}
	TEST_F(TasksQueueTest, ReschedulesToNewPriority) {
		std::uniform_int_distribution<int> dist(600, 20000);
		std::uniform_int_distribution<int> dist2(-500, 500);
		int priority = dist(randEng);
		int modifier = dist2(randEng);
		bool threadSet = false;

		auto task = std::make_shared<Task>(
			[priority, modifier, &threadSet](TasksQueue* queue, TaskPtr task)->void {
				threadSet = true;
				task->Reschedule(TaskPriority{ static_cast<uint32_t>( priority + modifier ) });
			},
			TaskPriority{ static_cast<uint32_t>( priority ) },
			TaskThreadTarget{ MAIN_THREAD }
		);

		queue.AddTask(task);
		ASSERT_EQ(task->GetStatus(), TaskStatus::TASK_IN_QUEUE_MAIN_THREAD);
		ASSERT_EQ(task->GetOptions().priority, priority);

		queue.Update();
		ASSERT_TRUE(threadSet);
		EXPECT_EQ(task->GetOptions().priority, priority + modifier);
	}
	TEST_F(TasksQueueTest, ReschedulesToNewBlocking) {
		std::uniform_int_distribution<unsigned> dist(0, 1);
		bool threadSet = false;
		bool blocking = static_cast<bool>(dist(randEng));

		auto task = std::make_shared<Task>(
			[blocking, &threadSet](TasksQueue* queue, TaskPtr task)->void {
				threadSet = true;
				task->Reschedule(TaskBlocking{ !blocking });
			},
			TaskBlocking{ blocking },
			TaskThreadTarget{ MAIN_THREAD }
		);

		queue.AddTask(task);
		ASSERT_EQ(task->GetStatus(), TaskStatus::TASK_IN_QUEUE_MAIN_THREAD);
		ASSERT_EQ(task->GetOptions().isBlocking, blocking);

		queue.Update();
		ASSERT_TRUE(threadSet);
		EXPECT_EQ(task->GetOptions().isBlocking, !blocking);
	}
	TEST_F(TasksQueueTest, ReschedulesWithDelay) {
		std::uniform_int_distribution<int> dist(10000, 30000);
		TaskDelay delay{ dist(randEng) };
		bool threadSet = false;

		auto task = std::make_shared<Task>(
			[delay, &threadSet](TasksQueue* queue, TaskPtr task)->void {
				threadSet = true;
				task->Reschedule( delay );
			},
			TaskThreadTarget{ MAIN_THREAD }
		);

		queue.AddTask(task);
		ASSERT_EQ(task->GetStatus(), TaskStatus::TASK_IN_QUEUE_MAIN_THREAD);

		queue.Update();
		ASSERT_TRUE(threadSet);
		EXPECT_EQ(task->GetStatus(), TaskStatus::TASK_SUSPENDED);
		EXPECT_EQ(task->GetOptions().suspendTime, delay);
	}
}
