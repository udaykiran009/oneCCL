#ifndef COLL_HPP
#define COLL_HPP

#include "base.hpp"
#include "config.hpp"
#ifdef CCL_ENABLE_SYCL
#include "sycl_base.hpp"

template <typename Dtype>
using sycl_buffer_t = cl::sycl::buffer<Dtype, 1>;
cl::sycl::queue sycl_queue;
#endif

struct base_coll;

using coll_list_t = std::vector<std::shared_ptr<base_coll>>;
using req_list_t = std::vector<ccl::request>;

typedef struct bench_coll_exec_attr {
    ccl::reduction reduction;

    bench_coll_exec_attr() = default;
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
            attr = std::make_shared<attr_t>(
                ccl::environment::instance().create_operation_attr<attr_t>());
        }
    };

    using supported_op_attr_t = std::tuple<ccl::shared_ptr_class<ccl::allgatherv_attr>,
                                           ccl::shared_ptr_class<ccl::allreduce_attr>,
                                           ccl::shared_ptr_class<ccl::alltoall_attr>,
                                           ccl::shared_ptr_class<ccl::alltoallv_attr>,
                                           ccl::shared_ptr_class<ccl::reduce_attr>,
                                           ccl::shared_ptr_class<ccl::broadcast_attr>,
                                           ccl::shared_ptr_class<ccl::reduce_scatter_attr>,
                                           ccl::shared_ptr_class<ccl::sparse_allreduce_attr>>;
    using match_id_t = std::array<char, MATCH_ID_SIZE - 1>;

    template <class attr_t>
    attr_t& get_attr() {
        return *(ccl_tuple_get<ccl::shared_ptr_class<attr_t>>(coll_attrs).get());
    }

    template <class attr_t>
    const attr_t& get_attr() const {
        return *(ccl_tuple_get<ccl::shared_ptr_class<attr_t>>(coll_attrs).get());
    }

    template <ccl::operation_attr_id attrId, class Value>
    typename ccl::details::ccl_api_type_attr_traits<ccl::operation_attr_id, attrId>::return_type
    set(const Value& v) {
        ccl_tuple_for_each(coll_attrs, setter<attrId, Value>(v));

        set_common_fields(std::integral_constant<ccl::operation_attr_id, attrId>{}, v);
        return v;
    }

    match_id_t& get_match_id() {
        return match_id;
    }

    void init_all() {
        ccl_tuple_for_each(coll_attrs, factory{});
    }

private:
    void set_common_fields(...){};

    void set_common_fields(
        std::integral_constant<ccl::operation_attr_id, ccl::operation_attr_id::match_id>,
        const std::string& match) {
        if (match.size() >= MATCH_ID_SIZE) {
            throw ccl::ccl_error(std::string("Too long `match_id` size: ") +
                                 std::to_string(match.size()) +
                                 ", expected less than: " + std::to_string(MATCH_ID_SIZE));
        }
        std::copy(match.begin(), match.end(), match_id.begin());
        match_id[match.size()] = '\0';
    }

    supported_op_attr_t coll_attrs;
    match_id_t match_id;
} bench_coll_exec_attr;

typedef struct bench_coll_init_attr {
    size_t buf_count;
    size_t max_elem_count;
    size_t v2i_ratio;
} bench_coll_init_attr;

/* base polymorph collective wrapper class */
struct base_coll {
    base_coll(bench_coll_init_attr init_attr) : init_attr(init_attr) {
        send_bufs.resize(init_attr.buf_count);
        recv_bufs.resize(init_attr.buf_count);
    }

    base_coll() = delete;
    virtual ~base_coll() = default;

    virtual const char* name() const noexcept {
        return nullptr;
    };

    virtual void prepare(size_t elem_count){};
    virtual void finalize(size_t elem_count){};

    virtual ccl::datatype get_dtype() const = 0;

    virtual void start(size_t count,
                       size_t buf_idx,
                       const bench_coll_exec_attr& attr,
                       req_list_t& reqs) = 0;

    virtual void start_single(size_t count, const bench_coll_exec_attr& attr, req_list_t& reqs) = 0;

    /* to get buf_count from initialized private member */
    size_t get_buf_count() const noexcept {
        return init_attr.buf_count;
    }
    size_t get_max_elem_count() const noexcept {
        return init_attr.max_elem_count;
    }
    size_t get_single_buf_max_elem_count() const noexcept {
        return init_attr.buf_count * init_attr.max_elem_count;
    }

    std::vector<void*> send_bufs;
    std::vector<void*> recv_bufs;

    void* single_send_buf = nullptr;
    void* single_recv_buf = nullptr;

private:
    bench_coll_init_attr init_attr;
};

struct cpu_specific_data {
    static ccl::shared_ptr_class<ccl::communicator> comm_ptr;
    static void init(size_t size, size_t rank, ccl::shared_ptr_class<ccl::kvs_interface> kvs) {
        if (comm_ptr) {
            throw ccl::ccl_error(std::string(__FUNCTION__) + " - reinit is not allowed");
        }

        comm_ptr = std::make_shared<ccl::communicator>(
            ccl::environment::instance().create_communicator(size, rank, kvs));
    }

    static void deinit() {
        comm_ptr.reset();
    }
};

ccl::shared_ptr_class<ccl::communicator> cpu_specific_data::comm_ptr{};

#ifdef CCL_ENABLE_SYCL
struct device_specific_data {
    using device_communicator_ptr = ccl::unique_ptr_class<ccl::device_communicator>;
    static device_communicator_ptr comm_ptr;
    static ccl::vector_class<ccl::device_communicator> comm_array;
    static ccl::shared_ptr_class<ccl::stream> stream_ptr;
    static void init(size_t size,
                     size_t rank,
                     cl::sycl::device& device,
                     cl::sycl::context ctx,
                     ccl::shared_ptr_class<ccl::kvs_interface> kvs) {
        if (stream_ptr or comm_ptr or !comm_array.empty()) {
            throw ccl::ccl_error(std::string(__FUNCTION__) + " - reinit is not allowed");
        }

        comm_array = ccl::environment::instance().create_device_communicators(
            size,
            ccl::vector_class<ccl::pair_class<ccl::rank_t, cl::sycl::device>>{ { rank, device } },
            ctx,
            kvs);
        //single device version
        comm_ptr =
            device_communicator_ptr(new ccl::device_communicator(std::move(*comm_array.begin())));
        stream_ptr =
            std::make_shared<ccl::stream>(ccl::environment::instance().create_stream(sycl_queue));
    }

    static void deinit() {
        comm_ptr.reset();
        stream_ptr.reset();
    }
};

device_specific_data::device_communicator_ptr device_specific_data::comm_ptr{};
ccl::vector_class<ccl::device_communicator> device_specific_data::comm_array{};
ccl::shared_ptr_class<ccl::stream> device_specific_data::stream_ptr{};
#endif //CCL_ENABLE_SYCL
#endif /* COLL_HPP */
