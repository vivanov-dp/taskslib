#include "gtest/gtest.h"

#include <tuple>
#include <random>

#include "TestTools.h"
#include "TaskOptions.h"

namespace TasksLib {

	class TaskOptionsTest : public TestWithRandom {
	public:
		TaskOptions opt;

		void TestLambda(TaskOptions& optTested, ExecutableTester& execTest) {
			ASSERT_NE(execTest.test, execTest.testBase + execTest.generated);
			ASSERT_NE(optTested.executable, nullptr);
			optTested.executable(nullptr, nullptr);
			EXPECT_EQ(execTest.test, execTest.testBase + execTest.generated);
		}
	};

	TEST_F(TaskOptionsTest, CreatesDefault) {
		EXPECT_EQ(opt.priority, TaskPriority{ 0 });
		EXPECT_FALSE(opt.isBlocking);
		EXPECT_FALSE(opt.isMainThread);
		EXPECT_EQ(opt.executable, nullptr);
		EXPECT_EQ(opt.suspendTime, TaskDelay{ 0 });
	}
	TEST_F(TaskOptionsTest, CreatesWithCopy) {
		TaskOptions otherOpt = GenerateRandomOptions(randEng);
		ExecutableTester execTest(randEng);
		TaskExecutable lambda = [&](TasksQueue* queue, TaskPtr task)->void { execTest.PerformTest(); };

		otherOpt.SetOptions(lambda);
		if (otherOpt == opt) {						// Tick that just in case we hit the jackpot and randomly generate the default values
			otherOpt.isBlocking = !otherOpt.isBlocking;
		}
		ASSERT_NE(opt, otherOpt);					// generated options have to differ from the default

		TaskOptions opt2(otherOpt);
		EXPECT_EQ(opt2, otherOpt);
		TestLambda(opt2, execTest);
	}
	TEST_F(TaskOptionsTest, CreatesWithMove) {
		TaskOptions otherOpt = GenerateRandomOptions(randEng);
		ExecutableTester execTest(randEng);
		TaskExecutable lambda = [&](TasksQueue* queue, TaskPtr task)->void { execTest.PerformTest(); };

		otherOpt.SetOptions(lambda);
		if (otherOpt == opt) {						// Tick that just in case we hit the jackpot and randomly generate the default values
			otherOpt.isBlocking = !otherOpt.isBlocking;
		}
		ASSERT_NE(opt, otherOpt);					// generated options have to differ from the default

		TaskOptions opt2(std::move(otherOpt));
		otherOpt.executable = opt2.executable;		// Hack that because after std::move() otherOpt.executable could be left with its target empty and then opt != otherOpt
		EXPECT_EQ(opt2, otherOpt);
		TestLambda(opt2, execTest);
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
		ExecutableTester execTest(randEng);
		TaskExecutable lambda = [&](TasksQueue* queue, TaskPtr task)->void { execTest.PerformTest(); };

		ASSERT_NE(execTest.test, execTest.testBase + execTest.generated);
		opt.SetOptions(lambda);
		EXPECT_NE(opt.executable, nullptr);
		TestLambda(opt, execTest);

		execTest.ResetTest();
		ASSERT_NE(execTest.test, execTest.testBase + execTest.generated);
		opt.SetOptions((TaskExecutable)[&](TasksQueue* queue, TaskPtr task)->void { execTest.PerformTest(); });
		TestLambda(opt, execTest);
	}
	TEST_F(TaskOptionsTest, SetsSuspendTime) {
		std::uniform_int_distribution<unsigned int> dist(1, INT_MAX);
		auto seed = randDev();

		opt.SetOptions(TaskDelay{ 0 });
		ASSERT_EQ(opt.suspendTime, TaskDelay{ 0 });

		TaskDelay ms;
		randEng.seed(seed);

		ms = TaskDelay{ dist(randEng) };
		opt.SetOptions(ms);
		EXPECT_EQ(opt.suspendTime, ms);

		randEng.seed(seed);
		opt.SetOptions(TaskDelay{ 0 });
		opt.SetOptions(TaskDelay{ dist(randEng) });
		EXPECT_EQ(opt.suspendTime, ms);
	}
	TEST_F(TaskOptionsTest, SetsMultipleOptions) {
		// I'm annoyingly unable to return and hold in a variable a (tuple) of random length and element types to pass to SetOptions(),
		// so this test is not covering the cases when SetOptions() is called with number of arguments between 2 and max-1.
		// Due to the usage of variadic templates however this test should still be a guarantee that it would work in such a case.

		TaskOptions other;
		ExecutableTester execTest(randEng);
		TaskExecutable lambda = [&](TasksQueue* queue, TaskPtr task)->void { execTest.PerformTest(); };
		other = GenerateRandomOptions(randEng);
		other.SetOptions(lambda);

		opt.SetOptions(other.priority, other.isBlocking, static_cast<TaskThreadTarget>((int)(!other.isMainThread)), other.executable, other.suspendTime);
		ASSERT_EQ(opt, other);
		opt.executable(nullptr, nullptr);
		EXPECT_EQ(execTest.test, execTest.testBase + execTest.generated);
	}
	
	TEST_F(TaskOptionsTest, AssignsFromOtherWithCopy) {
		ExecutableTester execTest(randEng);
		TaskExecutable lambda = [&](TasksQueue* queue, TaskPtr task)->void { execTest.PerformTest(); };
		TaskOptions otherOpt = GenerateRandomOptions(randEng);
		
		opt.SetOptions(!otherOpt.isBlocking);		// Tick that just in case we hit the jackpot and randomly generate the default values
		otherOpt.SetOptions(lambda);
		ASSERT_NE(opt, otherOpt);

		opt = otherOpt;
		EXPECT_EQ(opt, otherOpt);
		TestLambda(opt, execTest);
	}
	TEST_F(TaskOptionsTest, AssignsFromOtherWithMove) {
		ExecutableTester execTest(randEng);
		TaskExecutable lambda = [&](TasksQueue* queue, TaskPtr task)->void { execTest.PerformTest(); };
		TaskOptions otherOpt = GenerateRandomOptions(randEng);

		opt.SetOptions(!otherOpt.isBlocking);		// Tick that just in case we hit the jackpot and randomly generate the default values
		otherOpt.SetOptions(lambda);
		ASSERT_NE(opt, otherOpt);

		opt = std::move(otherOpt);
		otherOpt.executable = opt.executable;		// Hack that because after std::move() otherOpt.executable could be left with its target empty and then opt != otherOpt
		EXPECT_EQ(opt, otherOpt);
		TestLambda(opt, execTest);
	}

	TEST_F(TaskOptionsTest, ComparesToTaskOptions) {
		TaskOptions otherOpt = GenerateRandomOptions(randEng);
		
		opt.SetOptions(!otherOpt.isBlocking);		// Tick that just in case we hit the jackpot and randomly generate the default values
		EXPECT_NE(opt, otherOpt);
		opt = otherOpt;
		EXPECT_EQ(opt, otherOpt);
	}

}
