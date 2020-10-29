#ifndef BASE_HPP
#define BASE_HPP

#include <chrono>
#include <functional>
#include <vector>

#include "oneapi/ccl.hpp"
#include "ccl_test_conf.hpp"

class GlobalData {
public:
    std::vector<ccl::communicator> comms;
    ccl::shared_ptr_class<ccl::kvs> kvs;

    GlobalData(GlobalData& gd) = delete;
    void operator=(const GlobalData&) = delete;
    static GlobalData& instance() {
        static GlobalData gd;
        return gd;
    };

protected:
    GlobalData(){};
    ~GlobalData(){};
};

#include "utils.hpp"

#define SEED_STEP 10

template <typename T>
struct typed_test_param {
    ccl_test_conf test_conf;
    size_t elem_count;
    size_t buffer_count;
    size_t process_count;
    size_t process_idx;
    static size_t priority;
    std::vector<size_t> buf_indexes;
    std::vector<std::vector<T>> send_buf;
    std::vector<std::vector<T>> recv_buf;

    // buffers for bf16
    std::vector<std::vector<short>> send_buf_bf16;
    std::vector<std::vector<short>> recv_buf_bf16;

    std::vector<ccl::event> reqs;
    ccl::string_class match_id;

    typed_test_param(ccl_test_conf tconf)
            : test_conf(tconf),
              elem_count(get_ccl_elem_count(test_conf)),
              buffer_count(get_ccl_buffer_count(test_conf)) {
        process_count = GlobalData::instance().comms[0].size();
        process_idx = GlobalData::instance().comms[0].rank();
        buf_indexes.resize(buffer_count);
    }

    template <class coll_attr_type>
    void prepare_coll_attr(coll_attr_type& coll_attr, size_t idx);

    std::string create_match_id(size_t buf_idx);
    bool complete_request(ccl::event& e);
    void define_start_order();
    bool complete();
    void swap_buffers(size_t iter);
    size_t generate_priority_value(size_t buf_idx);

    const ccl_test_conf& get_conf() {
        return test_conf;
    }

    void print(std::ostream& output);

    void* get_send_buf(size_t buf_idx) {
        if (test_conf.datatype == DT_BFLOAT16)
            return static_cast<void*>(send_buf_bf16[buf_idx].data());
        else
            return static_cast<void*>(send_buf[buf_idx].data());
    }

    void* get_recv_buf(size_t buf_idx) {
        if (test_conf.datatype == DT_BFLOAT16)
            return static_cast<void*>(recv_buf_bf16[buf_idx].data());
        else
            return static_cast<void*>(recv_buf[buf_idx].data());
    }
};

template <typename T>
class base_test {
public:
    size_t global_process_idx;
    size_t global_process_count;
    char err_message[ERR_MESSAGE_MAX_LEN]{};

    char* get_err_message() {
        return err_message;
    }

    base_test();

    virtual void alloc_buffers(typed_test_param<T>& param);
    virtual void fill_buffers(typed_test_param<T>& param);

    int run(typed_test_param<T>& param);
    virtual void run_derived(typed_test_param<T>& param) = 0;

    virtual size_t get_recv_buf_size(typed_test_param<T>& param) = 0;

    virtual int check(typed_test_param<T>& param) = 0;
    virtual int check_error(typed_test_param<T>& param,
                            T expected,
                            size_t buf_idx,
                            size_t elem_idx);
};

class MainTest : public ::testing ::TestWithParam<ccl_test_conf> {
    template <typename T>
    int run(ccl_test_conf param);

public:
    int test(ccl_test_conf& param) {
        switch (param.datatype) {
            case DT_INT8: return run<int8_t>(param);
            /*case DT_UINT8: return run<uint8_t>(param);*/
            case DT_INT16: return run<int16_t>(param);
            /*case DT_UINT16: return run<uint16_t>(param);*/
            case DT_INT32: return run<int32_t>(param);
            /*case DT_UINT32: return run<uint32_t>(param);
            case DT_INT64: return run<int64_t>(param);
            case DT_UINT64: return run<uint64_t>(param);
            case DT_FLOAT16: return TEST_SUCCESS;*/
            case DT_FLOAT32: return run<float>(param);
            /*case DT_FLOAT64: return run<double>(param);*/
#ifdef CCL_BF16_COMPILER
            case DT_BFLOAT16: return run<float>(param);
#endif
            default:
                EXPECT_TRUE(false) << "Unexpected data type: " << param.datatype;
                return TEST_FAILURE;
        }
    }
};
#endif /* BASE_HPP */
