#include <list>
#include <string>
#include <memory>
#include <iostream>

#include "Task.h"
#include "gtest/gtest.h"

namespace TasksLib {

	class TaskTestCreate : public ::testing::Test {
	public:
		TaskTestCreate::TaskTestCreate() {}
		TaskTestCreate::~TaskTestCreate() {}

		std::list<std::string> FindNonDefaults(Task* task) {
			std::list<std::string> result;

			if (task->GetPriority() != 0) {
				result.push_back("priority");
			}
			if (task->IsBlocking() != false) {
				result.push_back("blocking");
			}
			if (task->IsMainThread() != false) {
				result.push_back("mainThread");
			}
			if (task->GetStatus() != TASK_INIT) {
				result.push_back("status");
			}
			if (task->GetExecutable() != nullptr) {
				result.push_back("executable");
			}
			if (task->GetSuspendTime() != std::chrono::milliseconds(0)) {
				result.push_back("suspendTime");
			}

			return result;
		}
	};

	TEST_F(TaskTestCreate, DoesCreateDefault) {
		auto task = std::make_unique<Task>();

		ASSERT_TRUE(task);

		std::list<std::string> nonDefaults = FindNonDefaults(task.get());
		EXPECT_EQ(nonDefaults, std::list<std::string>{});
	}
	TEST_F(TaskTestCreate, DoesCreateWithPriority) {
		auto task = std::make_unique<Task>(TaskPriority{ 15 });

		ASSERT_TRUE(task);

		std::list<std::string> nonDefaults = FindNonDefaults(task.get());

		EXPECT_EQ(task->GetPriority(), 15);
		EXPECT_EQ(nonDefaults, std::list<std::string>{"priority"});
	}
	TEST_F(TaskTestCreate, DoesCreateWithBlocking) {
		auto task = std::make_unique<Task>(TaskBlocking{ true });

		ASSERT_TRUE(task);

		std::list<std::string> nonDefaults = FindNonDefaults(task.get());

		EXPECT_TRUE(task->IsBlocking());
		EXPECT_EQ(nonDefaults, std::list<std::string>{"blocking"});
	}
	TEST_F(TaskTestCreate, DoesCreateWithMainThread) {
		auto task = std::make_unique<Task>(TaskThreadTarget{MAIN_THREAD});

		ASSERT_TRUE(task);

		std::list<std::string> nonDefaults = FindNonDefaults(task.get());

		EXPECT_TRUE(task->IsMainThread());
		EXPECT_EQ(nonDefaults, std::list<std::string>{"mainThread"});
	}
	TEST_F(TaskTestCreate, DoesCreateWithExecutable) {
		int zen = 15;
		auto lambda = [&](TasksQueue* queue, TaskPtr task)->void { zen=17; };
		auto task = std::make_unique<Task>(lambda);

		ASSERT_TRUE(task);

		std::list<std::string> nonDefaults = FindNonDefaults(task.get());
		EXPECT_EQ(nonDefaults, std::list<std::string>{"executable"});

		auto lambda2 = task->GetExecutable();
		ASSERT_TRUE(lambda2);

		lambda2(nullptr, std::make_shared<Task>());
		EXPECT_EQ(zen, 17);
		
	}
	TEST_F(TaskTestCreate, DoesCreateWithSuspendTime) {
		std::chrono::milliseconds delay = std::chrono::milliseconds(200);
		auto task = std::make_unique<Task>(std::chrono::milliseconds(200));

		ASSERT_TRUE(task);

		std::list<std::string> nonDefaults = FindNonDefaults(task.get());

		EXPECT_EQ(nonDefaults, std::list<std::string>{"suspendTime"});
		EXPECT_EQ(task->GetSuspendTime(), delay);
	}
	TEST_F(TaskTestCreate, DoesCreateWithOptions) {
		auto task = std::make_unique<Task>(TaskPriority{ 42 }, TaskThreadTarget{ MAIN_THREAD }, std::chrono::milliseconds(15));

		ASSERT_TRUE(task);

		std::list<std::string> nonDefaults = FindNonDefaults(task.get());

		EXPECT_EQ(nonDefaults, std::list<std::string>({ "priority", "mainThread", "suspendTime" }));
		EXPECT_EQ(task->GetPriority(), 42);
		EXPECT_TRUE(task->IsMainThread());
		EXPECT_EQ(task->GetSuspendTime(), std::chrono::milliseconds(15));
	}
}
