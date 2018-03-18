#include "gtest/gtest.h"

#include <random>

#include "TestTools.h"
#include "Task.h"

namespace TasksLib {

	class TaskTest : public ::testing::Test {
	public:
		TaskTest::TaskTest() {
			randEng.seed(randDev());
		}
		TaskTest::~TaskTest() {}

		Task task;

		std::random_device randDev;
		std::default_random_engine randEng;

		void ApplyReschedule(Task& task) const {
			task.ApplyReschedule_();
		}
		void SetDoReschedule(Task& task, const bool doReschedule) const {
			task.doReschedule_ = doReschedule;
		}
		void ResetReschedule(Task& task) const {
			task.ResetReschedule_();
		}
	};

	TEST_F(TaskTest, Creates) {
		EXPECT_EQ(task.GetStatus(), TaskStatus::TASK_INIT);
	}
	TEST_F(TaskTest, CreatesWithOptions) {
		ExecutableTester execTest(randEng);
		auto lambda = [&](TasksQueue* queue, TaskPtr task)->void { execTest.PerformTest(); };
		TaskOptions options = GenerateRandomOptions(randEng);
		options.SetOptions(lambda);
		Task newTask{ options };

		EXPECT_EQ(newTask.GetOptions(), options);

		TaskExecutable const& exec = newTask.GetOptions().executable;
		exec(nullptr, nullptr);
		EXPECT_EQ(execTest.test, execTest.testBase + execTest.generated);
	}
	TEST_F(TaskTest, SetsReschedule) {
		TaskOptions opt;
		ASSERT_FALSE(task.WillReschedule());
		ASSERT_EQ(task.GetRescheduleOptions(), opt);

		ExecutableTester execTest(randEng);
		auto lambda = [&](TasksQueue* queue, TaskPtr task)->void { execTest.PerformTest(); };
		opt = GenerateRandomOptions(randEng);
		opt.SetOptions(lambda);
		
		task.Reschedule(opt.priority, opt.isBlocking, static_cast<TaskThreadTarget>((int)(!opt.isMainThread)), opt.executable, opt.suspendTime);

		EXPECT_EQ(task.GetRescheduleOptions(), opt);
		TaskExecutable const& exec = task.GetRescheduleOptions().executable;
		exec(nullptr, nullptr);
		EXPECT_EQ(execTest.test, execTest.testBase + execTest.generated);
	}
	TEST_F(TaskTest, ResetsReschedule) {
		TaskOptions opt;
		SetDoReschedule(task, true);
		ExecutableTester execTest(randEng);
		auto lambda = [&](TasksQueue* queue, TaskPtr task)->void { execTest.PerformTest(); };
		opt = GenerateRandomOptions(randEng);
		opt.SetOptions(lambda);

		task.Reschedule(opt.priority, opt.isBlocking, static_cast<TaskThreadTarget>((int)(!opt.isMainThread)), opt.executable, opt.suspendTime);
		EXPECT_EQ(task.GetRescheduleOptions(), opt);

		ResetReschedule(task);
		EXPECT_EQ(task.GetRescheduleOptions(), task.GetOptions());
		EXPECT_FALSE(task.WillReschedule());
	}
	TEST_F(TaskTest, AppliesReschedule) {
		TaskOptions opt;
		ASSERT_EQ(task.GetOptions(), opt);
		
		ExecutableTester execTest(randEng);
		auto lambda = [&](TasksQueue* queue, TaskPtr task)->void { execTest.PerformTest(); };
		opt = GenerateRandomOptions(randEng);
		opt.SetOptions(lambda);

		task.Reschedule(opt.priority, opt.isBlocking, static_cast<TaskThreadTarget>((int)(!opt.isMainThread)), opt.executable, opt.suspendTime);
		ASSERT_NE(task.GetOptions(), task.GetRescheduleOptions());
		ApplyReschedule(task);
		ASSERT_EQ(task.GetOptions(), opt);
		TaskExecutable const& exec = task.GetOptions().executable;
		exec(nullptr, nullptr);
		EXPECT_EQ(execTest.test, execTest.testBase + execTest.generated);
	}
}
