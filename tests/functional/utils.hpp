#pragma once

#include <ctime>
#include <cstdlib>
#include <fstream>
#include <numeric>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/syscall.h>

#include "gtest/gtest.h"

#include "mpi.h"

#include "conf.hpp"
#include "oneapi/ccl.hpp"

#define sizeofa(arr)   (sizeof(arr) / sizeof(*arr))
#define DTYPE          float
#define CACHELINE_SIZE 64

#define ITER_COUNT          2
#define ERR_MESSAGE_MAX_LEN 180

#define TIMEOUT 30

#define GETTID()    syscall(SYS_gettid)
#define UNUSED_ATTR __attribute__((unused))

#define TEST_SUCCESS 0
#define TEST_FAILURE 1

#if 0

#define PRINT(fmt, ...) \
    do { \
        fflush(stdout); \
        printf("\n(%ld): %s: " fmt "\n", GETTID(), __FUNCTION__, ##__VA_ARGS__); \
        fflush(stdout); \
    } while (0)

#define PRINT_BUFFER(buf, bufSize, prefix) \
    do { \
        std::string strToPrint; \
        for (size_t idx = 0; idx < bufSize; idx++) { \
            strToPrint += std::to_string(buf[idx]); \
            if (idx != bufSize - 1) \
                strToPrint += ", "; \
        } \
        strToPrint = std::string(prefix) + strToPrint; \
        PRINT("%s", strToPrint.c_str()); \
    } while (0)

#else /* ENABLE_DEBUG */

#define PRINT(fmt, ...) \
    {}
#define PRINT_BUFFER(buf, bufSize, prefix) \
    {}

#endif /* ENABLE_DEBUG */

#define OUTPUT_NAME_ARG "--gtest_output="
#define PATCH_OUTPUT_NAME_ARG(argc, argv) \
    do { \
        auto& comm = gd.comms[0]; \
        if (comm.size() > 1) { \
            for (int idx = 1; idx < argc; idx++) { \
                if (strstr(argv[idx], OUTPUT_NAME_ARG)) { \
                    std::string patchedArg; \
                    std::string originArg = std::string(argv[idx]); \
                    size_t extPos = originArg.find(".xml"); \
                    size_t argLen = strlen(OUTPUT_NAME_ARG); \
                    patchedArg = originArg.substr(argLen, extPos - argLen) + "_" + \
                                 std::to_string(comm.rank()) + ".xml"; \
                    PRINT("originArg %s, extPos %zu, argLen %zu, patchedArg %s", \
                          originArg.c_str(), \
                          extPos, \
                          argLen, \
                          patchedArg.c_str()); \
                    argv[idx][0] = '\0'; \
                    if (comm.rank()) \
                        ::testing::GTEST_FLAG(output) = ""; \
                    else \
                        ::testing::GTEST_FLAG(output) = patchedArg.c_str(); \
                } \
            } \
        } \
    } while (0)

#define SHOW_ALGO(coll_name) \
    do { \
        char* algo_name = getenv(coll_name); \
        if (algo_name) \
            printf("%s  = %s\n", coll_name, algo_name); \
    } while (0)

#define RUN_METHOD_DEFINITION(ClassName) \
    template <typename T> \
    int MainTest::run(ccl_test_conf tParam) { \
        ClassName<T> className; \
        typed_test_param<T> typed_param(tParam); \
        std::ostringstream output; \
        if (typed_param.comm_rank == 0) \
            printf("%s", output.str().c_str()); \
        int result = className.run(typed_param); \
        int result_final = 0; \
        static int glob_idx = 0; \
        auto& comm = GlobalData::instance().comms[0]; \
        ccl::allreduce(&result, &result_final, 1, ccl::reduction::sum, comm).wait(); \
        if (result_final > 0) { \
            print_err_message(className.get_err_message(), output); \
            if (typed_param.comm_rank == 0) { \
                printf("%s", output.str().c_str()); \
                if (glob_idx) { \
                    typed_test_param<T> test_conf(test_params[glob_idx - 1]); \
                    output << "Previous case:\n"; \
                    test_conf.print(output); \
                } \
                output << "Current case:\n"; \
                typed_param.print(output); \
                EXPECT_TRUE(false) << output.str(); \
            } \
            output.str(""); \
            output.clear(); \
            glob_idx++; \
            return TEST_FAILURE; \
        } \
        glob_idx++; \
        return TEST_SUCCESS; \
    }

#define ASSERT(cond, fmt, ...) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, \
                    "(%ld): %s:%s:%d: ASSERT '%s' FAILED: " fmt "\n", \
                    GETTID(), \
                    __FILE__, \
                    __FUNCTION__, \
                    __LINE__, \
                    #cond, \
                    ##__VA_ARGS__); \
            fflush(stderr); \
            exit(0); \
        } \
    } while (0)

#define MAIN_FUNCTION() \
    int main(int argc, char** argv, char* envs[]) { \
        init_test_params(); \
        ccl::init(); \
        int mpi_inited = 0; \
        MPI_Initialized(&mpi_inited); \
        if (!mpi_inited) { \
            MPI_Init(NULL, NULL); \
        } \
        int size, rank; \
        MPI_Comm_size(MPI_COMM_WORLD, &size); \
        MPI_Comm_rank(MPI_COMM_WORLD, &rank); \
        ccl::kvs::address_type main_addr; \
        auto& gd = GlobalData::instance(); \
        if (rank == 0) { \
            gd.kvs = ccl::create_main_kvs(); \
            main_addr = gd.kvs->get_address(); \
            MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD); \
        } \
        else { \
            MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD); \
            gd.kvs = ccl::create_kvs(main_addr); \
        } \
        gd.comms.emplace_back(ccl::create_communicator(size, rank, gd.kvs)); \
        PATCH_OUTPUT_NAME_ARG(argc, argv); \
        testing::InitGoogleTest(&argc, argv); \
        int res = RUN_ALL_TESTS(); \
        MPI_Finalize(); \
        return res; \
    }

#define TEST_CASES_DEFINITION(FuncName) \
    TEST_P(MainTest, FuncName) { \
        ccl_test_conf param = GetParam(); \
        EXPECT_EQ(TEST_SUCCESS, this->test(param)); \
    } \
    INSTANTIATE_TEST_CASE_P(test_params, MainTest, ::testing::ValuesIn(test_params));

class CustomPrinter : public ::testing::EmptyTestEventListener {
    virtual void OnTestCaseStart(const ::testing::TestCase& test_case) {
        printf("Overall %d tests from %s\n", test_case.test_to_run_count(), test_case.name());
        fflush(stdout);
    }

    virtual void OnTestCaseEnd(const ::testing::TestCase& test_case) {
        if (!::testing::GTEST_FLAG(print_time))
            return;

        printf("Overall %d tests from %s (%s ms total)\n\n",
               test_case.test_to_run_count(),
               test_case.name(),
               ::testing::internal::StreamableToString(test_case.elapsed_time()).c_str());
        fflush(stdout);
    }

    virtual void OnTestPartResult(const ::testing::TestPartResult& test_part_result) {
        if (test_part_result.failed()) {
            printf("%s in %s:%d\n%s\n",
                   "*** Failure",
                   test_part_result.file_name(),
                   test_part_result.line_number(),
                   test_part_result.summary());
        }
        else
            printf("*** Success");
    }

    virtual void OnTestEnd(const ::testing::TestInfo& test_info) {}

protected:
    testing::TestEventListener* listener;
};
