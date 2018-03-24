#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <random>
#include <memory>

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
	};
	TEST_F(ResourcePoolTest, Test1) {
		EXPECT_TRUE(true);
	}
}
