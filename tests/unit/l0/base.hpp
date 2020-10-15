
void set_test_device_indices(const char *indices_csv = nullptr);

#ifndef STANDALONE_UT
#include <gtest/gtest.h>
#else // STANDALONE_UT
#include <tuple>
namespace testing {
class Test {
public:
    Test() = default;
    virtual ~Test() = default;

    virtual void SetUp() = 0;
    virtual void TearDown() = 0;
};

template<class ...T>
struct Types {
    using list_t = std::tuple<T...>;
};
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

#define TYPED_TEST_CASE(fixture, types)                                 \
using type_list_##fixture = typename types::list_t;                               \
using first_type_##fixture = typename std::tuple_element<0, type_list_##fixture>::type;

using default_data_type = float;

// TODO Only first type from TYPED_TEST_CASE is invoking
#define TYPED_TEST(fixture, test_name)                  \
    struct TypedTest_##test_name : public fixture<first_type_##fixture> {          \
                                                        \
        template<class T>                               \
        void invoke();                                  \
        void start() {                                  \
            SetUp();                                    \
            invoke<first_type_##fixture>();             \
            TearDown();                                 \
        }                                               \
    };                                                  \
    template<class TypeParam>                           \
    void TypedTest_##test_name::invoke()

#endif // STANDALONE_UT
