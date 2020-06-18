#ifndef SPARSE_ALLREDUCE_BASE_HPP
#define SPARSE_ALLREDUCE_BASE_HPP

#include "sparse_allreduce_strategy.hpp"

template<class VType, class IType, template<class> class IndicesDistributorType>
struct base_sparse_allreduce_coll :
        base_coll, protected sparse_allreduce_strategy_impl<IType,
                                                            IndicesDistributorType>
{
    using ITypeNonMod = typename std::remove_pointer<IType>::type;
    using VTypeNonMod = typename std::remove_pointer<VType>::type;

    using coll_base = base_coll;
    using coll_strategy = sparse_allreduce_strategy_impl<IType,
                                                         IndicesDistributorType>;

    using coll_base::stream;
    using coll_base::comm;

    std::vector<ITypeNonMod*> send_ibufs;
    std::vector<VTypeNonMod*> send_vbufs;

    /* buffers from these arrays will be reallocated inside completion callback */
    std::vector<ITypeNonMod*> recv_ibufs;
    std::vector<VTypeNonMod*> recv_vbufs;

    size_t* recv_icount = nullptr;
    size_t* recv_vcount = nullptr;
    std::vector<sparse_allreduce_fn_ctx_t> fn_ctxs;

    ITypeNonMod* single_send_ibuf = nullptr;
    VTypeNonMod* single_send_vbuf = nullptr;
    ITypeNonMod* single_recv_ibuf = nullptr;
    VTypeNonMod* single_recv_vbuf = nullptr;
    size_t single_recv_icount {};
    size_t single_recv_vcount {};
    sparse_allreduce_fn_ctx_t single_fn_ctx;

    base_sparse_allreduce_coll(bench_coll_init_attr init_attr) :
        base_coll(init_attr), coll_strategy(init_attr.v2i_ratio, base_coll::comm->size())
    {
        int result = 0;

        result = posix_memalign((void**)&recv_icount, ALIGNMENT,
                                init_attr.buf_count * sizeof(size_t));
        result = posix_memalign((void**)&recv_vcount, ALIGNMENT,
                                init_attr.buf_count * sizeof(size_t));

        std::memset(recv_icount, 0, init_attr.buf_count * sizeof(size_t));
        std::memset(recv_vcount, 0, init_attr.buf_count * sizeof(size_t));
        (void)result;

        fn_ctxs.resize(init_attr.buf_count);
        send_ibufs.resize(init_attr.buf_count);
        send_vbufs.resize(init_attr.buf_count);
        recv_ibufs.resize(init_attr.buf_count);
        recv_vbufs.resize(init_attr.buf_count);
    }

    virtual ~base_sparse_allreduce_coll()
    {
        free(recv_icount);
        free(recv_vcount);
        fn_ctxs.clear();
    }

    const char* name() const noexcept override
    {
        return coll_strategy::class_name();
    }

    ccl::datatype get_dtype() const override final
    {
        return ccl::native_type_info<typename std::remove_pointer<VType>::type>::ccl_datatype_value;
    }
};

#endif /* SPARSE_ALLREDUCE_BASE_HPP */
