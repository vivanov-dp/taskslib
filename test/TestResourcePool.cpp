#include "gtest/gtest.h"

#include <random>

#include "TestTools.h"

namespace TasksLib {

	class ResourcePoolTest : public ::testing::Test {
	public:
		ResourcePoolTest::ResourcePoolTest() {
			randEng.seed(randDev());
		}
		ResourcePoolTest::~ResourcePoolTest() {}

		std::random_device randDev;
		std::default_random_engine randEng;
	};

	TEST_F(ResourcePoolTest, Test1) {
		EXPECT_TRUE(true);
	}
}
