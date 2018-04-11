#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <chrono>

#include "Types.h"
#include "TestTools.h"
#include "TasksQueueContainer.h"

namespace TasksLib {

	using namespace ::testing;

	class TasksQueueContainerTest : public TestWithRandom {
	public:
	};

	TEST_F(TasksQueueContainerTest, Test) {
		EXPECT_TRUE(true);
	}
}
