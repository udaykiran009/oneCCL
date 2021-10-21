#pragma once

#include "config.hpp"
#include "transport.hpp"
#include "types.hpp"

#ifdef CCL_ENABLE_SYCL
template <typename Dtype>
using sycl_buffer_t = cl::sycl::buffer<Dtype, 1>;
#endif

#define COLL_ROOT (0)

#define BF16_COEF 0.00001
#define FP16_COEF 0.0001

struct base_coll;

using coll_list_t = std::vector<std::shared_ptr<base_coll>>;
using req_list_t = std::vector<ccl::event>;

typedef struct bench_exec_attr {
    bench_exec_attr() = default;
    template <ccl::operation_attr_id attrId, class value_t>
    struct setter {
        setter(value_t v) : val(v) {}
        template <class attr_t>
        void operator()(ccl::shared_ptr_class<attr_t>& attr) {
            attr->template set<attrId, value_t>(val);
        }
        value_t val;
    };
    struct factory {
        template <class attr_t>
        void operator()(ccl::shared_ptr_class<attr_t>& attr) {
            attr = std::make_shared<attr_t>(ccl::create_operation_attr<attr_t>());
        }
    };

    using supported_op_attr_t = std::tuple<ccl::shared_ptr_class<ccl::allgatherv_attr>,
                                           ccl::shared_ptr_class<ccl::allreduce_attr>,
                                           ccl::shared_ptr_class<ccl::alltoall_attr>,
                                           ccl::shared_ptr_class<ccl::alltoallv_attr>,
                                           ccl::shared_ptr_class<ccl::reduce_attr>,
                                           ccl::shared_ptr_class<ccl::broadcast_attr>,
                                           ccl::shared_ptr_class<ccl::reduce_scatter_attr>/*,
                                           ccl::shared_ptr_class<ccl::sparse_allreduce_attr>*/>;

    template <class attr_t>
    attr_t& get_attr() {
        return *(ccl_tuple_get<ccl::shared_ptr_class<attr_t>>(coll_attrs).get());
    }

    template <class attr_t>
    const attr_t& get_attr() const {
        return *(ccl_tuple_get<ccl::shared_ptr_class<attr_t>>(coll_attrs).get());
    }

    template <ccl::operation_attr_id attrId, class Value>
    typename ccl::detail::ccl_api_type_attr_traits<ccl::operation_attr_id, attrId>::return_type set(
        const Value& v) {
        ccl_tuple_for_each(coll_attrs, setter<attrId, Value>(v));
        return v;
    }

    void init_all() {
        ccl_tuple_for_each(coll_attrs, factory{});
    }

    ccl::reduction reduction;

private:
    supported_op_attr_t coll_attrs;
} bench_exec_attr;

typedef struct bench_init_attr {
    size_t buf_count;
    size_t max_elem_count;
    int inplace;
    size_t ranks_per_proc;
    int numa_node;
#ifdef CCL_ENABLE_SYCL
    sycl_mem_type_t sycl_mem_type;
    sycl_usm_type_t sycl_usm_type;
#endif
} bench_init_attr;

template <class OutDtype, class InDtype = OutDtype>
inline OutDtype get_val(InDtype value) {
    return value;
}

template <>
inline ccl::bfloat16 get_val<ccl::bfloat16, float>(float value) {
    return fp32_to_bf16(BF16_COEF * value);
}

template <>
inline ccl::float16 get_val<ccl::float16, float>(float value) {
    return fp32_to_fp16(FP16_COEF * value);
}

/* base polymorph collective wrapper class */
struct base_coll {
    base_coll(bench_init_attr init_attr) : init_attr(init_attr) {
        send_bufs.resize(init_attr.buf_count);
        recv_bufs.resize(init_attr.buf_count);

        for (size_t idx = 0; idx < init_attr.buf_count; idx++) {
            send_bufs[idx].resize(init_attr.ranks_per_proc);
            recv_bufs[idx].resize(init_attr.ranks_per_proc);
        }
    }

    base_coll() = delete;
    virtual ~base_coll() = default;

    virtual const char* name() const noexcept {
        return nullptr;
    };

    template <class T>
    std::vector<T> get_initial_values(size_t elem_count, int fill_value) {
        std::vector<T> res(elem_count);
        ccl::datatype dt = ccl::native_type_info<typename std::remove_pointer<T>::type>::dtype;
        if (dt == ccl::datatype::bfloat16) {
            for (size_t elem_idx = 0; elem_idx < elem_count; elem_idx++) {
                res[elem_idx] = fp32_to_bf16(BF16_COEF * fill_value).get_data();
            }
        }
        else if (dt == ccl::datatype::float16) {
            for (size_t elem_idx = 0; elem_idx < elem_count; elem_idx++) {
                res[elem_idx] = fp32_to_fp16(FP16_COEF * fill_value).get_data();
            }
        }
        else {
            std::fill(res.begin(), res.end(), fill_value);
        }
        return res;
    }

    virtual void prepare(size_t elem_count) {
        auto& transport = transport_data::instance();
        auto& comms = transport.get_comms();
        auto streams = transport.get_bench_streams();
        size_t ranks_per_proc = base_coll::get_ranks_per_proc();

        for (size_t rank_idx = 0; rank_idx < ranks_per_proc; rank_idx++) {
            prepare_internal(elem_count, comms[rank_idx], streams[rank_idx], rank_idx);
        }
    }

    virtual void finalize(size_t elem_count) {
        auto& transport = transport_data::instance();
        auto& comms = transport.get_comms();
        auto streams = transport.get_bench_streams();
        size_t ranks_per_proc = base_coll::get_ranks_per_proc();

        for (size_t rank_idx = 0; rank_idx < ranks_per_proc; rank_idx++) {
            finalize_internal(elem_count, comms[rank_idx], streams[rank_idx], rank_idx);
        }
    }

    virtual void prepare_internal(size_t elem_count,
                                  ccl::communicator& comm,
                                  ccl::stream& stream,
                                  size_t rank_idx) = 0;

    virtual void finalize_internal(size_t elem_count,
                                   ccl::communicator& comm,
                                   ccl::stream& stream,
                                   size_t rank_idx) = 0;

    virtual ccl::datatype get_dtype() const = 0;

    size_t get_dtype_size() const {
        return ccl::get_datatype_size(get_dtype());
    }

    virtual void start(size_t count,
                       size_t buf_idx,
                       const bench_exec_attr& attr,
                       req_list_t& reqs) = 0;

    /* to get buf_count from initialized private member */
    size_t get_buf_count() const noexcept {
        return init_attr.buf_count;
    }

    size_t get_max_elem_count() const noexcept {
        return init_attr.max_elem_count;
    }

#ifdef CCL_ENABLE_SYCL
    sycl_mem_type_t get_sycl_mem_type() const noexcept {
        return init_attr.sycl_mem_type;
    }

    sycl_usm_type_t get_sycl_usm_type() const noexcept {
        return init_attr.sycl_usm_type;
    }
#endif

    size_t get_ranks_per_proc() const noexcept {
        return init_attr.ranks_per_proc;
    }

    int get_inplace() const noexcept {
        return init_attr.inplace;
    }

    int get_numa_node() const noexcept {
        return init_attr.numa_node;
    }

    // first dim - per buf_count, second dim - per local rank
    std::vector<std::vector<void*>> send_bufs;
    std::vector<std::vector<void*>> recv_bufs;

private:
    bench_init_attr init_attr;
};
