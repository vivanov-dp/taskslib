#include "gtest/gtest.h"

#include "Task.h"

namespace TasksLib {

	class TaskTest : public ::testing::Test {
	public:
		TaskTest::TaskTest() {}
		TaskTest::~TaskTest() {}
	};

	TEST_F(TaskTest, Test1) {
		EXPECT_TRUE(true);
	}
}
