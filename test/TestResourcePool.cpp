#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <random>
#include <memory>
#include <sstream>

#include "Types.h"
#include "TestTools.h"
#include "ResourcePool.h"

namespace TasksLib {

	using namespace ::testing;

	template <class T>
	class MockResourcePool : public ResourcePool<T> {
	public:
		virtual void Add(std::unique_ptr<T> elem) {
			Add_(elem.get());
		}

		MOCK_METHOD1_T(Add_, void(T* elem));
	};
	class ResourceDeleterTest : public TestWithRandom {
	public:
		std::shared_ptr<ResourcePool<int> *> resPool;
		std::weak_ptr<ResourcePool<int> *> resPoolWeak;

		void initPointers(ResourcePool<int> *ptr) {
			resPool = std::make_shared<ResourcePool<int>*>(ptr);
			resPoolWeak = std::weak_ptr<ResourcePool<int>*>(resPool);
		}
	};
	TEST_F(ResourceDeleterTest, Creates) {
		initPointers(new ResourcePool<int>{});
		EXPECT_NO_THROW(ResourceDeleter<int> resDel{ resPoolWeak });
	}
	TEST_F(ResourceDeleterTest, WithPoolCallsAdd) {
		MockResourcePool<int> mockPool;

		initPointers(&mockPool);
		ResourceDeleter<int> resDel{ resPoolWeak };

		std::unique_ptr<int> ptr(new int{ 174 });

		EXPECT_CALL(mockPool, Add_(ptr.get()));
		resDel(ptr.release());
	}
	TEST_F(ResourceDeleterTest, WithoutPoolDoesntCallAdd) {
		MockResourcePool<int> mockPool;

		initPointers(&mockPool);
		ResourceDeleter<int> resDel{ resPoolWeak };

		std::unique_ptr<int> ptr(new int{ 174 });
		resPool.reset();

		EXPECT_CALL(mockPool, Add_(_)).Times(0);
		resDel(ptr.release());
	}

	class ResourcePoolTest : public TestWithRandom {
	public:
		ResourcePoolTest() {
			std::uniform_int_distribution<unsigned int> distCount(3, 12);
			unsigned int i;

			str = GenerateRandomString(15, 25, randEng);
			Count = distCount(randEng);
			for (i = 0; i < Count; i++) {
				pool.Add(std::make_unique<std::string>(str));
			}
		}

		std::string str;
		unsigned int Count;
		ResourcePool<std::string> pool;
	};
	TEST_F(ResourcePoolTest, Creates) {
		ResourcePool<int> newPool;

		EXPECT_TRUE(newPool.IsEmpty());
		EXPECT_EQ(0, newPool.Size());
	}
	TEST_F(ResourcePoolTest, Adds) {
		ASSERT_FALSE(pool.IsEmpty());
		EXPECT_EQ(pool.Size(), Count);

		std::string str2 = *(pool.Acquire().get());
		EXPECT_EQ(str2, str);
	}
	TEST_F(ResourcePoolTest, AcquiresAndReturns) {
		ASSERT_FALSE(pool.IsEmpty());
		EXPECT_EQ(pool.Size(), Count);

		{
			auto strPtr = pool.Acquire();

			EXPECT_EQ(*strPtr, str);
			EXPECT_EQ(pool.Size(), Count - 1);
		}

		EXPECT_EQ(pool.Size(), Count);
	}
	TEST_F(ResourcePoolTest, AddAcquire) {
		ResourcePool<std::string> otherPool;
		ASSERT_TRUE(otherPool.IsEmpty());

		{
			auto strPtr = otherPool.AddAcquire(std::make_unique<std::string>(str));
			EXPECT_EQ(*strPtr, str);
		}
		EXPECT_EQ(otherPool.Size(), 1);

		std::string str2 = *(otherPool.Acquire().get());
		EXPECT_EQ(str, str2);
	}
}
