
void set_test_device_indices(const char *indices_csv);

#ifndef STANDALONE_UT
    #include <gtest/gtest.h>
#else
namespace testing
{
    class Test
    {
    public:
        Test() = default;
        virtual ~Test() = default;

        virtual void SetUp() = 0;
        virtual void TearDown() = 0;
    };
}

#define TEST_F(fixture, test_name) \
struct Test_##test_name : public fixture \
{ \
  void invoke(); \
  void start() \
  { \
      SetUp(); \
      invoke(); \
      TearDown(); \
  }\
};\
void Test_##test_name::invoke() \

#endif
