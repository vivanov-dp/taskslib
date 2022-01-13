#include "gtest/gtest.h"

#include <random>

#include "TestTools.h"
#include "Task.h"

namespace TasksLib {

	class [[maybe_unused]] TaskTest : public TestWithRandom {
	public:
		Task task;

		void ApplyReschedule(Task& task) const {
			task.ApplyReschedule_();
		}
		void SetDoReschedule(Task& task, const bool doReschedule) const {
			task._doReschedule = doReschedule;
		}
		void ResetReschedule(Task& task) const {
			task.ResetReschedule_();
		}
	};

	TEST_F(TaskTest, CreatesDefault) {
		EXPECT_EQ(task.GetStatus(), TaskStatus::TASK_INIT);
	}
	TEST_F(TaskTest, CreatesWithOptions) {
		ExecutableTester execTest(randEng);
		auto lambda = [&](TasksQueue* queue, TaskPtr task)->void { execTest.PerformTest(); };
		TaskOptions options = GenerateRandomOptions(randEng);
		options.SetOptions(lambda);
		Task newTask{ options };

		EXPECT_EQ(task.GetStatus(), TaskStatus::TASK_INIT);
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


	// ====== TaskWithData ==============================================================

	class TaskWithDataTest : public TestWithRandom {
	public:
		TaskWithData<int> task;
	};

	TEST_F(TaskWithDataTest, SetsAndGetsData) {
		std::uniform_int_distribution<int> random(INT_MIN, INT_MAX);

		ASSERT_EQ(task.GetData(), nullptr);
		int i = random(randEng);
		task.SetData(std::make_shared<int>(i));
		EXPECT_EQ(*(task.GetData()), i);
	}
}
