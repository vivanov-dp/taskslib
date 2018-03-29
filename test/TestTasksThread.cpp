#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "Types.h"
#include "TestTools.h"
#include "TasksThread.h"

namespace TasksLib {

	using namespace ::testing;

	class TasksThreadTest : public TestWithRandom {
	public:
	};
	TEST_F(TasksThreadTest, CreatesAndRuns) {
		std::uniform_int_distribution<int> random(0, INT_MAX);
		int test = 0;
		int testRand = random(randEng);

		TasksThread thread{ false, [&test](int testR)->void {
			test = testR;
		}, testRand };
		thread.join();

		EXPECT_EQ(test, testRand);
	}

}
