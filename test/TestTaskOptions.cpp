#include "gtest/gtest.h"

#include <chrono>
#include <tuple>
#include <random>

#include "TaskOptions.h"

namespace TasksLib {

	class TaskOptionsTest : public ::testing::Test {
	public:
		TaskOptionsTest() {
			randEng.seed(randDev());
		}
		~TaskOptionsTest() {}

		TaskOptions opt;

		std::random_device randDev;
		std::default_random_engine randEng;
	};

	TEST_F(TaskOptionsTest, CreatesDefault) {
		EXPECT_EQ(opt.priority, TaskPriority{ 0 });
		EXPECT_FALSE(opt.isBlocking);
		EXPECT_FALSE(opt.isMainThread);
		EXPECT_EQ(opt.executable, nullptr);
		EXPECT_EQ(opt.suspendTime, std::chrono::milliseconds(0));
	}
	TEST_F(TaskOptionsTest, SetsPriority) {
		std::uniform_int_distribution<unsigned int> dist(1, INT_MAX);
		auto seed = randDev();

		opt.SetOptions(TaskPriority{ 0 });
		ASSERT_EQ(opt.priority, TaskPriority{ 0 });

		TaskPriority pri;
		randEng.seed(seed);
		
		pri = TaskPriority{ dist(randEng) };
		opt.SetOptions(pri);
		EXPECT_EQ(opt.priority, pri);
		
		randEng.seed(seed);
		opt.SetOptions(TaskPriority{ 0 });
		opt.SetOptions(TaskPriority{ dist(randEng) });
		EXPECT_EQ(opt.priority, pri);
	}
	TEST_F(TaskOptionsTest, SetsBlocking) {
		opt.SetOptions(TaskBlocking{ false });
		EXPECT_FALSE(opt.isBlocking);
		opt.SetOptions(TaskBlocking{ true });
		EXPECT_TRUE(opt.isBlocking);
	}
	TEST_F(TaskOptionsTest, SetsMainThread) {
		opt.SetOptions(TaskThreadTarget{ WORKER_THREAD });
		EXPECT_FALSE(opt.isMainThread);
		opt.SetOptions(TaskThreadTarget{ MAIN_THREAD });
		EXPECT_TRUE(opt.isMainThread);
	}
	TEST_F(TaskOptionsTest, SetsExecutable) {
		int zen = 15;
		auto lambda = [&](TasksQueue* queue, TaskPtr task)->void { zen = 17; };

		opt.SetOptions(lambda);
		EXPECT_NE(opt.executable, nullptr);
		opt.executable(nullptr, nullptr);
		EXPECT_EQ(zen, 17);

		opt.SetOptions([&](TasksQueue* queue, TaskPtr task)->void { zen = 27; });
		opt.executable(nullptr, nullptr);
		EXPECT_EQ(zen, 27);
	}
	TEST_F(TaskOptionsTest, SetsSuspendTime) {
		std::uniform_int_distribution<unsigned int> dist(1, INT_MAX);
		auto seed = randDev();

		opt.SetOptions(std::chrono::milliseconds(0));
		ASSERT_EQ(opt.suspendTime, std::chrono::milliseconds(0));

		std::chrono::milliseconds ms;
		randEng.seed(seed);

		ms = std::chrono::milliseconds{ dist(randEng) };
		opt.SetOptions(ms);
		EXPECT_EQ(opt.suspendTime, ms);

		randEng.seed(seed);
		opt.SetOptions(std::chrono::milliseconds(0));
		opt.SetOptions(std::chrono::milliseconds( dist(randEng) ));
		EXPECT_EQ(opt.suspendTime, ms);
	}
	TEST_F(TaskOptionsTest, SetsMultipleOptions) {
		// I'm annoyingly unable to generate a tuple of random length and element types to pass to SetOptions(), so this test is not good
		opt.SetOptions(TaskPriority{ 42 }, TaskThreadTarget{ MAIN_THREAD }, std::chrono::milliseconds(15));

		EXPECT_EQ(opt.priority, TaskPriority{ 42 });
		EXPECT_TRUE(opt.isMainThread);
		EXPECT_EQ(opt.suspendTime, std::chrono::milliseconds(15));
	}

}
