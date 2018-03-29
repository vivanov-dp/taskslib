#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "Types.h"
#include "TestTools.h"
#include "TasksQueue.h"

namespace TasksLib {

	using namespace ::testing;

	class TasksQueueTest : public TestWithRandom {
	public:
	};
	TEST_F(TasksQueueTest, Test) {
		EXPECT_TRUE(true);
	}

}
