
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <numeric>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/syscall.h>

#include "gtest/gtest.h"

#include "ccl.hpp"

#define sizeofa(arr)        (sizeof(arr) / sizeof(*arr))
#define DTYPE               float
#define CACHELINE_SIZE      64

#define ITER_COUNT          2
#define ERR_MESSAGE_MAX_LEN 180

#define TIMEOUT 30

#define GETTID() syscall(SYS_gettid)

#if 0

#define PRINT(fmt, ...)                               \
  do {                                                \
      fflush(stdout);                                 \
      printf("\n(%ld): %s: " fmt "\n",                \
              GETTID(), __FUNCTION__, ##__VA_ARGS__); \
      fflush(stdout);                                 \
     } while (0)

#define PRINT_BUFFER(buf, bufSize, prefix)           \
  do {                                               \
      std::string strToPrint;                        \
      for (size_t idx = 0; idx < bufSize; idx++)     \
      {                                              \
     strToPrint += std::to_string(buf[idx]);         \
          if (idx != bufSize - 1)                    \
              strToPrint += ", ";                    \
      }                                              \
      strToPrint = std::string(prefix) + strToPrint; \
      PRINT("%s", strToPrint.c_str());               \
} while (0)

#else /* ENABLE_DEBUG */

#define PRINT(fmt, ...) {}
#define PRINT_BUFFER(buf, bufSize, prefix) {}

#endif /* ENABLE_DEBUG */


#define OUTPUT_NAME_ARG "--gtest_output="
#define PATCH_OUTPUT_NAME_ARG(argc, argv)                                                 \
    do                                                                                    \
    {                                                                                     \
        auto comm = ccl::environment::instance().create_communicator();                   \
        if (comm->size() > 1)                                                             \
        {                                                                                 \
            for (int idx = 1; idx < argc; idx++)                                          \
            {                                                                             \
                if (strstr(argv[idx], OUTPUT_NAME_ARG))                                   \
                {                                                                         \
                    std::string patchedArg;                                               \
                    std::string originArg = std::string(argv[idx]);                       \
                    size_t extPos = originArg.find(".xml");                               \
                    size_t argLen = strlen(OUTPUT_NAME_ARG);                              \
                    patchedArg = originArg.substr(argLen, extPos - argLen) + "_"          \
                                + std::to_string(comm->rank())                            \
                                + ".xml";                                                 \
                    PRINT("originArg %s, extPos %zu, argLen %zu, patchedArg %s",          \
                    originArg.c_str(), extPos, argLen, patchedArg.c_str());               \
                    argv[idx][0] = '\0';                                                  \
                    if (comm->rank())                                                     \
                            ::testing::GTEST_FLAG(output) = "";                           \
                        else                                                              \
                            ::testing::GTEST_FLAG(output) = patchedArg.c_str();           \
                }                                                                         \
            }                                                                             \
        }                                                                                 \
    } while (0)

#define SHOW_ALGO(Collective_Name)                         \
do                                                         \
{                                                          \
    char* algo_name = getenv(Collective_Name);             \
    if (algo_name)                                         \
        printf("%s  = %s\n", Collective_Name, algo_name);  \
} while (0)

#define SSHOW_ALGO()                                       \
( {                                                        \
    std::string algo_name = getenv(Collective_Name);       \
    std::string transp_name = getenv("CCL_ATL_TRANSPORT"); \
    std::string res_str = algo_name + transp_name;         \
    res_str;                                               \
} )

#define RUN_METHOD_DEFINITION(ClassName)                                    \
    template <typename T>                                                   \
    int MainTest::run(ccl_test_conf tParam)                                 \
    {                                                                       \
      ClassName<T> className;                                               \
      typed_test_param<T> typed_param(tParam);                              \
      std::ostringstream output;                                            \
      if (typed_param.process_idx == 0)                                     \
      printf("%s", output.str().c_str());                                   \
      int result = className.run(typed_param);                              \
      int result_final = 0;                                                 \
      static int glob_idx = 0;                                              \
      auto comm = ccl::environment::instance().create_communicator();       \
      auto stream = ccl::environment::instance().create_stream();           \
      std::shared_ptr <ccl::request> reqs;                                  \
      ccl::coll_attr coll_attr{};                                           \
      init_coll_attr(&coll_attr);                                           \
      reqs = comm->allreduce(&result, &result_final, 1,                     \
                         ccl::reduction::sum, &coll_attr, stream);          \
      reqs->wait();                                                         \
      if (result_final > 0)                                                 \
      {                                                                     \
          print_err_message(className.get_err_message(), output);           \
          if (typed_param.process_idx == 0)                                 \
          {                                                                 \
              printf("%s", output.str().c_str());                           \
              if (glob_idx) {                                               \
                  typed_test_param<T> test_conf(test_params[glob_idx - 1]); \
                  output << "Previous case:\n";                             \
                  test_conf.print(output);                                  \
              }                                                             \
              output << "Current case:\n";                                  \
              typed_param.print(output);                                    \
              EXPECT_TRUE(false) << output.str();                           \
          }                                                                 \
          output.str("");                                                   \
          output.clear();                                                   \
          glob_idx++;                                                       \
          return TEST_FAILURE;                                              \
      }                                                                     \
      glob_idx++;                                                           \
      return TEST_SUCCESS;                                                  \
    }

#define ASSERT(cond, fmt, ...)                                                       \
    do {                                                                             \
           if (!(cond))                                                              \
           {                                                                         \
                fprintf(stderr, "(%ld): %s:%s:%d: ASSERT '%s' FAILED: " fmt "\n",    \
                GETTID(), __FILE__, __FUNCTION__, __LINE__, #cond, ##__VA_ARGS__);   \
                fflush(stderr);                                                      \
                exit(0);                                                             \
            }                                                                        \
        } while (0)

#define MAIN_FUNCTION()                           \
    int main(int argc, char **argv, char* envs[]) \
    {                                             \
        init_test_params();                       \
        ccl::environment::instance();             \
        PATCH_OUTPUT_NAME_ARG(argc, argv);        \
        testing::InitGoogleTest(&argc, argv);     \
        int res = RUN_ALL_TESTS();                \
        return res;                               \
    }

#define UNUSED_ATTR __attribute__((unused))

#define TEST_SUCCESS 0
#define TEST_FAILURE 1

#define TEST_CASES_DEFINITION(FuncName)                               \
    TEST_P(MainTest, FuncName) {                                      \
        ccl_test_conf param = GetParam();                             \
        EXPECT_EQ(TEST_SUCCESS, this->test(param));                   \
    }                                                                 \
    INSTANTIATE_TEST_CASE_P(test_params,                              \
                            MainTest,                                 \
::testing::ValuesIn(test_params));


void init_coll_attr(ccl::coll_attr* coll_attr)
{
    coll_attr->prologue_fn = NULL;
    coll_attr->epilogue_fn = NULL;
    coll_attr->reduction_fn = NULL;
    coll_attr->priority = 0;
    coll_attr->synchronous = 0;
    coll_attr->match_id = NULL;
    coll_attr->to_cache = 0;
    coll_attr->vector_buf = 0;
}

void print_err_message(char* err_message, std::ostream& output)
{
    int message_len = strlen(err_message);
    auto comm = ccl::environment::instance().create_communicator();
    auto stream = ccl::environment::instance().create_stream();
    std::shared_ptr <ccl::request> reqs;
    ccl::coll_attr coll_attr{};
    init_coll_attr(&coll_attr);
    int process_count = comm->size();
    int process_idx = comm->rank();
    size_t* arr_message_len = new size_t[process_count];
    int* arr_message_len_copy = new int[process_count];
    size_t* displs = new size_t[process_count];
    displs[0] = 1;
    std::fill(displs, displs + process_count, 1);
    reqs = comm->allgatherv(&message_len, 1, arr_message_len_copy, displs, &coll_attr, stream);
    reqs->wait();
    std::copy(arr_message_len_copy, arr_message_len_copy + process_count, arr_message_len);
    int full_message_len = std::accumulate(arr_message_len, arr_message_len + process_count, 0);
    if (full_message_len == 0)
    {
        delete[] arr_message_len;
        delete[] displs;
        return;
    }
    char* arrerr_message = new char[full_message_len];
    reqs = comm->allgatherv(err_message, message_len, arrerr_message, arr_message_len, &coll_attr, stream);
    reqs->wait();
    if (process_idx == 0)
    {
        output << arrerr_message;
    }
    delete[] arr_message_len;
    delete[] arr_message_len_copy;
    delete[] arrerr_message;
    delete[] displs;

}


std::ostream& operator<<(std::ostream& stream, ccl_test_conf const& test_conf)
{
    return stream <<
                    "data_type = "    << ccl_data_type_str[test_conf.data_type] <<
                    "\nplace_type = " << ccl_place_type_str[test_conf.place_type] <<
                    "\ncache_type = "   << ccl_cache_type_str[test_conf.cache_type] <<
                    "\nsize_type = "     << ccl_size_type_str[test_conf.size_type] <<
                    "\ncompletion_type = " << ccl_completion_type_str[test_conf.completion_type] <<
                    "\nreduction_type = " << ccl_reduction_type_str[test_conf.reduction_type] <<
                    "\ncomplete_order_type = " << ccl_order_type_str[test_conf.complete_order_type] <<
                    "\nstart_order_type = "    << ccl_order_type_str[test_conf.start_order_type] <<
                    "\nbuffer_count = "        << ccl_buffer_count_str[test_conf.buffer_count] <<
                    "\nprolog_type = " << ccl_prolog_type_str[test_conf.prolog_type] <<
                    "\nepilog_type = " << ccl_epilog_type_str[test_conf.epilog_type] << std::endl;
}
template <typename T>
T get_expected_min(size_t i, size_t buf_idx, size_t process_count, size_t coeff = 1)
{
    if ((T)(coeff * (i + buf_idx + process_count - 1)) < T(coeff * (i + buf_idx)))
        return (T)(coeff * (i + buf_idx + process_count - 1));
    return (T)(coeff * (i + buf_idx));
}
template <typename T>
T get_expected_max(size_t i, size_t buf_idx, size_t process_count, size_t coeff = 1)
{
    if ((T)(coeff *(i + buf_idx + process_count - 1)) > T(coeff * (i + buf_idx)))
        return (T)(coeff * (i + buf_idx + process_count - 1));
    return (T)(coeff * (i + buf_idx));
}
