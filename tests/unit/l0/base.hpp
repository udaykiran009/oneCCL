
void set_test_device_indices(const char *indices_csv = nullptr);

#ifndef STANDALONE_UT
#include <gtest/gtest.h>
#else // STANDALONE_UT
namespace testing {
class Test {
public:
    Test() = default;
    virtual ~Test() = default;

    virtual void SetUp() = 0;
    virtual void TearDown() = 0;
};

template<class ...T>
struct Types {};
} // namespace testing

#define TEST_F(fixture, test_name)                      \
    struct Test_##test_name : public fixture {          \
        void invoke();                                  \
        void start() {                                  \
            SetUp();                                    \
            invoke();                                   \
            TearDown();                                 \
        }                                               \
    };                                                  \
    void Test_##test_name::invoke()

#define TYPED_TEST_CASE(fixture, test_name)

#define TYPED_TEST(fixture, test_name)                  \
    struct TypedTest_##test_name : public fixture {          \
        struct TypedParam                               \
        {                                               \
            using first_type = uint8_t;                 \
            using second_type = first_type;             \
        };                                              \
        void invoke();                                  \
        void start() {                                  \
            SetUp();                                    \
            invoke<TypedParam>();                       \
            TearDown();                                 \
        }                                               \
    };                                                  \
    void TypedTest_##test_name::invoke<TypedParam>()

#endif // STANDALONE_UT
