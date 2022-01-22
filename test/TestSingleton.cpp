#include "gtest/gtest.h"

#include <memory>
#include <utility>

#include "TestTools.h"
#include "taskslib/ResourcePool.h"

namespace TasksLib {

    using namespace ::testing;
    using namespace std;

    class Resource {
    private:
        uint32_t a;

    public:
        Resource()
            : a(0) {}

        [[nodiscard]] uint32_t getA() const {
            return a;
        }
        void setA(const uint32_t i_a) {
            a = i_a;
        }
    };

    class ParamResource {
    private:
        uint32_t a;
        std::string s;

    public:
        ParamResource(const uint32_t i_a, std::string i_str)
            : a(i_a), s(std::move(i_str)) {}

        [[nodiscard]] uint32_t getA() const {
            return a;
        }
        [[nodiscard]] const std::string &getS() const {
            return s;
        }
    };



    class SingletonTest : public TestWithRandom {
    public:
        SingletonTest() = default;
    };

    TEST_F(SingletonTest, Creates) {
        shared_ptr<Resource> ptr;

        EXPECT_EQ(ptr.use_count(), 0);
        ptr = Singleton<Resource>::getInstance();
        EXPECT_EQ(ptr.use_count(), 1);
        EXPECT_EQ(ptr->getA(), 0);
    }

    TEST_F(SingletonTest, CreatesWithParams) {
        shared_ptr<ParamResource> ptr;
        std::uniform_int_distribution<uint32_t> distInt(1, UINT32_MAX);
        const uint32_t number = distInt(randEng);
        const std::string str = GenerateRandomString(12, 26, randEng);

        ptr = Singleton<ParamResource>::getInstance(number, str);
        EXPECT_EQ(ptr->getA(), number);
        EXPECT_EQ(ptr->getS(), str);
    }

    TEST_F(SingletonTest, EnsuresSingleInstance) {
        shared_ptr<Resource> ptr, ptr2;
        std::uniform_int_distribution<uint32_t> distInt(1, UINT32_MAX);
        const uint32_t number = distInt(randEng);

        ptr = Singleton<Resource>::getInstance();
        ptr->setA(number);

        ptr2 = Singleton<Resource>::getInstance();
        EXPECT_EQ(ptr2->getA(), number);

        EXPECT_EQ(ptr2.use_count(), 2);
    }

    TEST_F(SingletonTest, Deletes) {
        shared_ptr<Resource> ptr, ptr2;
        std::uniform_int_distribution<uint32_t> distInt(1, UINT32_MAX);
        const uint32_t number = distInt(randEng);

        ptr = Singleton<Resource>::getInstance();
        ptr->setA(number);
        ptr.reset();

        ptr2 = Singleton<Resource>::getInstance();
        EXPECT_NE(ptr2->getA(), number);
    }

}
