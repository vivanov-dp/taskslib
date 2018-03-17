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

		TaskOptions GenerateRandomOptions() {
			std::uniform_int_distribution<unsigned int> distInt(1, INT_MAX);
			std::uniform_int_distribution<unsigned int> distBool(0, 1);
			std::uniform_int_distribution<unsigned int> distThreadTarget(0, 1);

			TaskPriority priority{ distInt(randEng) };
			bool isBlocking{ (bool)distBool(randEng) };
			TaskThreadTarget target{ static_cast<TaskThreadTarget>(distThreadTarget(randEng)) };
			std::chrono::milliseconds ms{ distInt(randEng) };

			return TaskOptions{ priority, isBlocking, target, ms };
		}
	};

	TEST_F(TaskOptionsTest, CreatesDefault) {
		EXPECT_EQ(opt.priority, TaskPriority{ 0 });
		EXPECT_FALSE(opt.isBlocking);
		EXPECT_FALSE(opt.isMainThread);
		EXPECT_EQ(opt.executable, nullptr);
		EXPECT_EQ(opt.suspendTime, std::chrono::milliseconds(0));
	}
	TEST_F(TaskOptionsTest, CreatesWithCopy) {
		TaskOptions otherOpt = GenerateRandomOptions();

		if (otherOpt == opt) {						// Tick that just in case we hit the jackpot and randomly generate the default values
			otherOpt.isBlocking = !otherOpt.isBlocking;
		}
		ASSERT_NE(opt, otherOpt);

		TaskOptions opt2(otherOpt);
		EXPECT_EQ(opt2, otherOpt);
	}
	TEST_F(TaskOptionsTest, CreatesWithMove) {
		TaskOptions otherOpt = GenerateRandomOptions();

		if (otherOpt == opt) {						// Tick that just in case we hit the jackpot and randomly generate the default values
			otherOpt.isBlocking = !otherOpt.isBlocking;
		}
		ASSERT_NE(opt, otherOpt);

		TaskOptions opt2(std::move(otherOpt));
		EXPECT_EQ(opt2, otherOpt);
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
		std::uniform_int_distribution<unsigned int> distInt(1, INT_MAX-20);
		std::uniform_int_distribution<unsigned int> distInt2(1, 20);

		unsigned int zen = 0;
		unsigned int zen_init = distInt(randEng);
		unsigned int gen = 0;
		unsigned int gen2 = 0;
		auto lambda = [&](TasksQueue* queue, TaskPtr task)->void { gen = distInt2(randEng); zen = zen_init + gen; };

		opt.SetOptions(lambda);
		EXPECT_NE(opt.executable, nullptr);
		opt.executable(nullptr, nullptr);
		EXPECT_EQ(zen, zen_init + gen);

		opt.SetOptions([&](TasksQueue* queue, TaskPtr task)->void { gen2 = distInt2(randEng); zen = zen_init + gen2; });
		opt.executable(nullptr, nullptr);
		EXPECT_EQ(zen, zen_init + gen2);
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

		auto opt = new TaskOptions();
	}
	
	TEST_F(TaskOptionsTest, AssignsFromOtherWithCopy) {
		std::uniform_int_distribution<unsigned int> distInt(1, INT_MAX - 20);
		std::uniform_int_distribution<unsigned int> distInt2(1, 20);

		unsigned int zen = 0;
		unsigned int zen_init = distInt(randEng);
		unsigned int gen = 0;
		unsigned int gen2 = 0;
		auto lambda = [&](TasksQueue* queue, TaskPtr task)->void { gen = distInt2(randEng); zen = zen_init + gen; };
		TaskOptions otherOpt = GenerateRandomOptions();
		
		opt.SetOptions(!otherOpt.isBlocking);		// Tick that just in case we hit the jackpot and randomly generate the default values
		otherOpt.SetOptions(lambda);
		ASSERT_NE(opt, otherOpt);

		opt = otherOpt;
		EXPECT_EQ(opt, otherOpt);
		opt.executable(nullptr, nullptr);
		EXPECT_EQ(zen, zen_init + gen);
	}
	TEST_F(TaskOptionsTest, AssignsFromOtherWithMove) {
		std::uniform_int_distribution<unsigned int> distInt(1, INT_MAX - 20);
		std::uniform_int_distribution<unsigned int> distInt2(1, 20);

		unsigned int zen = 0;
		unsigned int zen_init = distInt(randEng);
		unsigned int gen = 0;
		unsigned int gen2 = 0;
		auto lambda = [&](TasksQueue* queue, TaskPtr task)->void { gen = distInt2(randEng); zen = zen_init + gen; };
		TaskOptions otherOpt = GenerateRandomOptions();

		opt.SetOptions(!otherOpt.isBlocking);		// Tick that just in case we hit the jackpot and randomly generate the default values
		otherOpt.SetOptions(lambda);
		ASSERT_NE(opt, otherOpt);

		opt = std::move(otherOpt);
		otherOpt.executable = opt.executable;		// Hack that because after std::move() otherOpt.executable could be left with its target empty and then opt != otherOpt
		EXPECT_EQ(opt, otherOpt);
		opt.executable(nullptr, nullptr);
		EXPECT_EQ(zen, zen_init + gen);
	}

	TEST_F(TaskOptionsTest, ComparesToTaskOptions) {
		TaskOptions otherOpt = GenerateRandomOptions();
		
		opt.SetOptions(!otherOpt.isBlocking);		// Tick that just in case we hit the jackpot and randomly generate the default values
		EXPECT_NE(opt, otherOpt);
		opt.SetOptions(otherOpt.priority, otherOpt.isBlocking, static_cast<TaskThreadTarget>((int)(!otherOpt.isMainThread)), otherOpt.suspendTime);
		EXPECT_EQ(opt, otherOpt);
	}

}
