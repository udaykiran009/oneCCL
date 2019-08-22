#include <cstdlib>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "base.h"

#define BUF_COUNT         (64) //(1024)
#define ELEM_COUNT        (1024 * 1024 / 512)
#define SINGLE_ELEM_COUNT (BUF_COUNT * ELEM_COUNT)
#define ALIGNMENT         (2 * 1024 * 1024)
#define DTYPE             float
#define CCL_DTYPE         (ccl_dtype_float)
#define MATCH_ID_SIZE     (64)

//#define DEFAULT_COLL_LIST "allgatherv,allreduce,bcast,reduce"
#define DEFAULT_COLL_LIST "allreduce,bcast,reduce"

#define PRINT_BY_ROOT(fmt, ...)         \
    if (rank == 0)                      \
    {                                   \
        printf(fmt"\n", ##__VA_ARGS__); \
    }

class base_coll;

using coll_list_t = std::vector<std::unique_ptr<base_coll>>;
using req_list_t = std::vector<ccl_request_t>;

template<class Dtype>
struct ccl_dtype_traits{};

template<>
struct ccl_dtype_traits<float>
{
    static constexpr ccl_datatype_t value = ccl_dtype_float;
};

constexpr const char* help_message = "\nplease specify comma-separated list of collective names\n\n"
                                     "example:\n\tallgatherv,allreduce,bcast,reduce\n";

std::list<std::string> tokenize(const std::string& input, char delimeter)
{
    std::stringstream ss(input);
    std::list<std::string> ret;
    std::string value;
    while (std::getline(ss, value, delimeter))
    {
        ret.push_back(value);
    }
    return ret;
}

void print_timings(double* timer, size_t elem_count, size_t buf_count)
{
    double* timers = (double*)malloc(size * sizeof(double));
    size_t* recv_counts = (size_t*)malloc(size * sizeof(size_t));

    size_t idx;
    for (idx = 0; idx < size; idx++)
        recv_counts[idx] = 1;

    ccl_request_t request = nullptr;
    ccl_coll_attr_t attr;
    memset(&attr, 0, sizeof(ccl_coll_attr_t));
    CCL_CALL(ccl_allgatherv(timer,
                            1,
                            timers,
                            recv_counts,
                            ccl_dtype_double,
                            &attr,
                            nullptr,
                            nullptr,
                            &request));
    CCL_CALL(ccl_wait(request));

    if (rank == 0)
    {
        double avg_timer = 0;
        double avg_timer_per_buf = 0;
        for (idx = 0; idx < size; idx++)
        {
            avg_timer += timers[idx];
        }
        avg_timer /= (ITERS * size);
        avg_timer_per_buf = avg_timer / buf_count;

        double stddev_timer = 0;
        double sum = 0;
        for (idx = 0; idx < size; idx++)
        {
            double val = timers[idx] / ITERS;
            sum += (val - avg_timer) * (val - avg_timer);
        }
        stddev_timer = sqrt(sum / size) / avg_timer * 100;
        printf("size %10zu x %5zu bytes, avg %10.2lf us, avg_per_buf %10.2f, stddev %5.1lf %%\n",
                elem_count * sizeof(DTYPE), buf_count, avg_timer, avg_timer_per_buf, stddev_timer);
    }
    ccl_barrier(nullptr, nullptr);
    free(timers);
    free(recv_counts);
}

struct base_coll
{
    base_coll() = default;
    virtual ~base_coll()
    {
        for (size_t idx = 0; idx < BUF_COUNT; idx++)
        {
            free(send_bufs[idx]);
            free(recv_bufs[idx]);
        }
        free(single_send_buf);
        free(single_recv_buf);
    }

    virtual const char* name() const noexcept = 0;

    virtual void prepare(size_t count) {};
    virtual void finalize(size_t count) {};

    virtual void start(size_t count, size_t buf_idx,
                       const ccl_coll_attr_t& coll_attr,
                       req_list_t& reqs) = 0;

    virtual void start_single(size_t count,
                              const ccl_coll_attr_t& coll_attr,
                              req_list_t& reqs) = 0;
    
    void* send_bufs[BUF_COUNT] = { nullptr };
    void* recv_bufs[BUF_COUNT] = { nullptr };
    void* single_send_buf = nullptr;
    void* single_recv_buf = nullptr;

    bool check_values = false;
};

template<class Dtype>
struct allgatherv_coll : public base_coll
{
    static constexpr const char* class_name() { return "allgatherv"; }

    virtual const char* name() const noexcept override
    {
        return class_name();
    }

    allgatherv_coll()
    {
        int result = 0;
        for (size_t idx = 0; idx < BUF_COUNT; idx++)
        {
            result = posix_memalign((void**)&send_bufs[idx], ALIGNMENT, ELEM_COUNT * sizeof(Dtype));
            result = posix_memalign((void**)&recv_bufs[idx], ALIGNMENT, ELEM_COUNT * sizeof(Dtype) * size);
        }
        result = posix_memalign((void**)&single_send_buf, ALIGNMENT, SINGLE_ELEM_COUNT * sizeof(Dtype));
        result = posix_memalign((void**)&single_recv_buf, ALIGNMENT, SINGLE_ELEM_COUNT * sizeof(Dtype) * size);
        result = posix_memalign((void**)&recv_counts, ALIGNMENT, size * sizeof(size_t));
        (void)result;
    }

    ~allgatherv_coll()
    {
        free(recv_counts);
    }

    virtual void prepare(size_t elem_count) override
    {
        if (!check_values)
            return;

        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
            {
                ((Dtype*)send_bufs[b_idx])[e_idx] = rank;
            }

            for (size_t idx = 0; idx < size; idx++)
            {
                for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
                {
                    ((Dtype*)recv_bufs[b_idx])[idx * elem_count + e_idx] = 0;
                }
            }
        }
    }

    virtual void finalize(size_t elem_count) override
    {
        if (!check_values)
            return;

        Dtype sbuf_expected = rank;
        Dtype value;
        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
            {
                value = ((Dtype*)send_bufs[b_idx])[e_idx];
                if (value != sbuf_expected)
                {
                    printf("send_bufs: buf_idx %zu, elem_idx %zu, expected %f, got %f\n",
                           b_idx, e_idx, sbuf_expected, value);
                    ASSERT(0, "unexpected value");
                }
            }

            for (size_t idx = 0; idx < size; idx++)
            {
                Dtype rbuf_expected = idx;
                for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
                {
                    value = ((Dtype*)recv_bufs[b_idx])[idx * elem_count + e_idx];
                    if (value != rbuf_expected)
                    {
                        printf("recv_bufs: buf_idx %zu, elem_idx %zu, expected %f, got %f\n",
                               b_idx, e_idx, rbuf_expected, value);
                        ASSERT(0, "unexpected value");
                    }
                }
            }
        }
    }

    void start_internal(size_t count, void* send_buf, void* recv_buf,
                        const ccl_coll_attr_t& coll_attr,
                        req_list_t& reqs)
    {
        for (size_t idx = 0; idx < size; idx++)
        {
            recv_counts[idx] = count;
        }

        ccl_request_t req;
        ccl_status_t ret = ccl_allgatherv(send_buf,
                                          count,
                                          recv_buf,
                                          recv_counts,
                                          ccl_dtype_traits<Dtype>::value,
                                          &coll_attr,
                                          nullptr, /* comm */
                                          nullptr, /* stream */
                                          &req);

        if (ret != ccl_status_success)
        {
            PRINT_BY_ROOT("ccl error: %d, count %zu", ret, count);
            ASSERT(0, "%s error", name());
        }
        reqs.push_back(req);
    }

    virtual void start(size_t count, size_t buf_idx,
                       const ccl_coll_attr_t& coll_attr,
                       req_list_t& reqs) override
    {
        start_internal(count, send_bufs[buf_idx], recv_bufs[buf_idx], coll_attr, reqs);
    }

    virtual void start_single(size_t count,
                              const ccl_coll_attr_t& coll_attr,
                              req_list_t& reqs) override
    {
        start_internal(count, single_send_buf, single_recv_buf, coll_attr, reqs);
    }

    size_t* recv_counts = nullptr;
};

template<class Dtype>
struct allreduce_coll : public base_coll
{
    static constexpr const char* class_name() { return "allreduce"; }

    virtual const char* name() const noexcept override
    {
        return class_name();
    }

    allreduce_coll()
    {
        int result = 0;
        for (size_t idx = 0; idx < BUF_COUNT; idx++)
        {
            result = posix_memalign((void**)&send_bufs[idx], ALIGNMENT, ELEM_COUNT * sizeof(Dtype));
            result = posix_memalign((void**)&recv_bufs[idx], ALIGNMENT, ELEM_COUNT * sizeof(Dtype));
        }
        result = posix_memalign((void**)&single_send_buf, ALIGNMENT, SINGLE_ELEM_COUNT * sizeof(Dtype));
        result = posix_memalign((void**)&single_recv_buf, ALIGNMENT, SINGLE_ELEM_COUNT * sizeof(Dtype));
        (void)result;
    }

    virtual void prepare(size_t elem_count) override
    {
        if (!check_values)
            return;

        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
            {
                ((Dtype*)send_bufs[b_idx])[e_idx] = rank;
                ((Dtype*)recv_bufs[b_idx])[e_idx] = 0;
            }
        }
    }

    virtual void finalize(size_t elem_count) override
    {
        if (!check_values)
            return;

        Dtype sbuf_expected = rank;
        Dtype rbuf_expected = (size - 1) * ((float)size / 2);
        Dtype value;
        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
            {
                value = ((Dtype*)send_bufs[b_idx])[e_idx];
                if (value != sbuf_expected)
                {
                    printf("send_bufs: buf_idx %zu, elem_idx %zu, expected %f, got %f\n",
                           b_idx, e_idx, sbuf_expected, value);
                    ASSERT(0, "unexpected value");
                }

                value = ((Dtype*)recv_bufs[b_idx])[e_idx];
                if (value != rbuf_expected)
                {
                    printf("recv_bufs: buf_idx %zu, elem_idx %zu, expected %f, got %f\n",
                           b_idx, e_idx, rbuf_expected, value);
                    ASSERT(0, "unexpected value");
                }
            }
        }
    }

    void start_internal(size_t count, void* send_buf, void* recv_buf,
                        const ccl_coll_attr_t& coll_attr,
                        req_list_t& reqs)
    {
        ccl_request_t req;
        ccl_status_t ret = ccl_allreduce(send_buf,
                                         recv_buf,
                                         count,
                                         ccl_dtype_traits<Dtype>::value,
                                         ccl_reduction_sum,
                                         &coll_attr,
                                         nullptr, /* comm */
                                         nullptr, /* stream */
                                         &req);

        if (ret != ccl_status_success)
        {
            PRINT_BY_ROOT("ccl error: %d, count %zu", ret, count);
            ASSERT(0, "%s error", name());
        }
        reqs.push_back(req);
    }

    virtual void start(size_t count, size_t buf_idx,
                       const ccl_coll_attr_t& coll_attr,
                       req_list_t& reqs) override
    {
        start_internal(count, send_bufs[buf_idx], recv_bufs[buf_idx], coll_attr, reqs);
    }

    virtual void start_single(size_t count,
                              const ccl_coll_attr_t& coll_attr,
                              req_list_t& reqs) override
    {
        start_internal(count, single_send_buf, single_recv_buf, coll_attr, reqs);
    }
};

template<class Dtype>
struct bcast_coll : public base_coll
{
    static constexpr const char* class_name() { return "bcast"; }

    virtual const char* name() const noexcept override
    {
        return class_name();
    }

    bcast_coll()
    {
        int result = 0;
        for (size_t idx = 0; idx < BUF_COUNT; idx++)
        {
            result = posix_memalign((void**)&recv_bufs[idx], ALIGNMENT, ELEM_COUNT * sizeof(Dtype));
        }
        result = posix_memalign((void**)&single_recv_buf, ALIGNMENT, SINGLE_ELEM_COUNT * sizeof(Dtype));
        (void)result;
    }

    virtual void prepare(size_t elem_count) override
    {
        if (!check_values)
            return;

        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
            {
                if (rank == COLL_ROOT)
                    ((Dtype*)recv_bufs[b_idx])[e_idx] = e_idx;
                else
                    ((Dtype*)recv_bufs[b_idx])[e_idx] = 0;
            }
        }
    }

    virtual void finalize(size_t elem_count) override
    {
        if (!check_values)
            return;

        Dtype value;
        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
            {
                value = ((Dtype*)recv_bufs[b_idx])[e_idx];
                if (value != e_idx)
                {
                    printf("recv_bufs: buf_idx %zu, elem_idx %zu, expected %f, got %f\n",
                           b_idx, e_idx, (Dtype)e_idx, value);
                    ASSERT(0, "unexpected value");
                }
            }
        }
    }

    void start_internal(size_t count, void* buf,
                        const ccl_coll_attr_t& coll_attr,
                        req_list_t& reqs)
    {
        ccl_request_t req;
        ccl_status_t ret = ccl_bcast(buf,
                                     count,
                                     ccl_dtype_traits<Dtype>::value,
                                     COLL_ROOT,
                                     &coll_attr,
                                     nullptr, /* comm */
                                     nullptr, /* stream */
                                     &req);

        if (ret != ccl_status_success)
        {
            PRINT_BY_ROOT("ccl error: %d, count %zu", ret, count);
            ASSERT(0, "%s error", name());
        }
        reqs.push_back(req);
    }

    virtual void start(size_t count, size_t buf_idx,
                       const ccl_coll_attr_t& coll_attr,
                       req_list_t& reqs) override
    {
        start_internal(count, recv_bufs[buf_idx], coll_attr, reqs);
    }

    virtual void start_single(size_t count,
                              const ccl_coll_attr_t& coll_attr,
                              req_list_t& reqs) override
    {
        start_internal(count, single_recv_buf, coll_attr, reqs);
    }
};

template<class Dtype>
struct reduce_coll : public base_coll
{
    static constexpr const char* class_name() { return "reduce"; }

    virtual const char* name() const noexcept override
    {
        return class_name();
    }

    reduce_coll()
    {
        int result = 0;
        for (size_t idx = 0; idx < BUF_COUNT; idx++)
        {
            result = posix_memalign((void**)&send_bufs[idx], ALIGNMENT, ELEM_COUNT * sizeof(Dtype));
            result = posix_memalign((void**)&recv_bufs[idx], ALIGNMENT, ELEM_COUNT * sizeof(Dtype));
        }
        result = posix_memalign((void**)&single_send_buf, ALIGNMENT, SINGLE_ELEM_COUNT * sizeof(Dtype));
        result = posix_memalign((void**)&single_recv_buf, ALIGNMENT, SINGLE_ELEM_COUNT * sizeof(Dtype));
        (void)result;
    }

    virtual void prepare(size_t elem_count) override
    {
        if (!check_values)
            return;

        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
            {
                ((Dtype*)send_bufs[b_idx])[e_idx] = rank;
                ((Dtype*)recv_bufs[b_idx])[e_idx] = 0;
            }
        }
    }

    virtual void finalize(size_t elem_count) override
    {
        if (!check_values)
            return;

        Dtype sbuf_expected = rank;
        Dtype rbuf_expected = (size - 1) * ((float)size / 2);
        Dtype value;
        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
            {
                value = ((Dtype*)send_bufs[b_idx])[e_idx];
                if (value != sbuf_expected)
                {
                    printf("send_bufs: buf_idx %zu, elem_idx %zu, expected %f, got %f\n",
                           b_idx, e_idx, sbuf_expected, value);
                    ASSERT(0, "unexpected value");
                }

                if (rank != COLL_ROOT)
                    continue;

                value = ((Dtype*)recv_bufs[b_idx])[e_idx];
                if (value != rbuf_expected)
                {
                    printf("recv_bufs: buf_idx %zu, elem_idx %zu, expected %f, got %f\n",
                           b_idx, e_idx, rbuf_expected, value);
                    ASSERT(0, "unexpected value");
                }
            }
        }
    }

    void start_internal(size_t count, void* send_buf, void* recv_buf,
                        const ccl_coll_attr_t& coll_attr,
                        req_list_t& reqs)
    {
        ccl_request_t req;
        ccl_status_t ret = ccl_reduce(send_buf,
                                      recv_buf,
                                      count,
                                      ccl_dtype_traits<Dtype>::value,
                                      ccl_reduction_sum,
                                      COLL_ROOT,
                                      &coll_attr,
                                      nullptr, /* comm */
                                      nullptr, /* stream */
                                      &req);

        if (ret != ccl_status_success)
        {
            PRINT_BY_ROOT("ccl error: %d, count %zu", ret, count);
            ASSERT(0, "%s error", name());
        }
        reqs.push_back(req);
    }

    virtual void start(size_t count, size_t buf_idx,
                       const ccl_coll_attr_t& coll_attr,
                       req_list_t& reqs) override
    {
        start_internal(count, send_bufs[buf_idx], recv_bufs[buf_idx], coll_attr, reqs);
    }

    virtual void start_single(size_t count,
                              const ccl_coll_attr_t& coll_attr,
                              req_list_t& reqs) override
    {
        start_internal(count, single_send_buf, single_recv_buf, coll_attr, reqs);
    }
};


template<class Dtype>
void create_colls(const std::list<std::string>& names, coll_list_t& colls)
{
    for (const auto& name : names)
    {
        if (name == allgatherv_coll<Dtype>::class_name())
        {
           colls.emplace_back(new allgatherv_coll<Dtype>());
        }
        else if (name == allreduce_coll<Dtype>::class_name())
        {
            colls.emplace_back(new allreduce_coll<Dtype>);
        }
        else if (name == bcast_coll<Dtype>::class_name())
        {
            colls.emplace_back(new bcast_coll<Dtype>());
        }
        else if (name == reduce_coll<Dtype>::class_name())
        {
            colls.emplace_back(new reduce_coll<Dtype>());
        }
        else
        {
            PRINT_BY_ROOT("unknown coll name: %s", name.c_str());
            ASSERT(0, "create_colls error");
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        PRINT_BY_ROOT("%s", help_message);
        return -1;
    }

    test_init();

    std::list<std::string> coll_names;
    coll_list_t colls;
    req_list_t reqs;

    try
    {
        coll_names = tokenize((argc == 2) ? argv[1] : DEFAULT_COLL_LIST, ',');
        create_colls<DTYPE>(coll_names, colls);
    }
    catch (const std::runtime_error& e)
    {
        PRINT_BY_ROOT("cannot create coll objects: %s\n%s", e.what(), help_message);
        test_finalize();
        return -1;
    }

    if (colls.empty())
    {
        PRINT_BY_ROOT("%s", help_message);
        test_finalize();
        return -1;
    }

    int check_values = 1;

    ccl_barrier(NULL, NULL);

    char match_id[MATCH_ID_SIZE];
    coll_attr.match_id = match_id;
    std::stringstream sstream;
    std::copy(coll_names.begin(), coll_names.end(),
              std::ostream_iterator<std::string>(sstream, " "));

    PRINT_BY_ROOT("start colls: %s, iters: %d, buf_count: %d, ranks %zu, check_values %d",
                  sstream.str().c_str(), ITERS, BUF_COUNT, size, check_values);

    for (auto& coll : colls)
    {
        coll->check_values = check_values;
    }

    reqs.reserve(colls.size() * BUF_COUNT);

    // warm up
    PRINT_BY_ROOT("do warm up");
    coll_attr.to_cache = 0;
    for (size_t count = 1; count < ELEM_COUNT; count *= 2)
    {
        for (auto& coll : colls)
        {
            for (size_t buf_idx = 0; buf_idx < BUF_COUNT; buf_idx++)
            {
                // PRINT_BY_ROOT("start_coll: %s, count %zu, buf_idx %zu", coll->name(), count, buf_idx);
                coll->start(count, buf_idx, coll_attr, reqs);
            }
        }
        for (auto &req : reqs)
        {
            ccl_wait(req);
        }
        reqs.clear();
    }

    // benchmark with multiple equal sized buffer per collective
    PRINT_BY_ROOT("do multi-buffers benchmark");
    coll_attr.to_cache = 1;
    for (size_t count = 1; count < ELEM_COUNT; count *= 2)
    {
        t = 0;
        for (size_t iter_idx = 0; iter_idx < ITERS; iter_idx++)
        {
            for (auto& coll : colls)
            {
                coll->prepare(count);
            }

            t1 = when();
            for (auto& coll : colls)
            {
                for (size_t buf_idx = 0; buf_idx < BUF_COUNT; buf_idx++)
                {
                    snprintf(match_id, sizeof(match_id), "coll_%s_count_%zu_buf_%zu", coll->name(), count, buf_idx);
                    coll->start(count, buf_idx, coll_attr, reqs);
                }
            }
            for (auto &req : reqs)
            {
                ccl_wait(req);
            }
            t2 = when();
            t += (t2 - t1);

            reqs.clear();

            for (auto& coll : colls)
            {
                coll->finalize(count);
            }
        }
        print_timings(&t, count, BUF_COUNT);
    }

    ccl_barrier(NULL, NULL);

    // benchmark with single buffer per collective
    PRINT_BY_ROOT("do single-buffer benchmark");
    coll_attr.to_cache = 1;
    for (size_t count = BUF_COUNT; count <= SINGLE_ELEM_COUNT; count *= 2)
    {
        t = 0;
        for (size_t iter_idx = 0; iter_idx < ITERS; iter_idx++)
        {
            t1 = when();
            for (auto& coll : colls)
            {
                snprintf(match_id, sizeof(match_id), "coll_%s_single_count_%zu", coll->name(), count);
                coll->start_single(count, coll_attr, reqs);
            }
            for (auto &req : reqs)
            {
                ccl_wait(req);
            }
            t2 = when();
            t += (t2 - t1);

            reqs.clear();
        }
        print_timings(&t, count, 1);
    }

    test_finalize();

    PRINT_BY_ROOT("PASSED\n");

    return 0;
}
