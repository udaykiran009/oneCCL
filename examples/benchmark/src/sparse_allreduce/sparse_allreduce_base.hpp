#ifndef SPARSE_ALLREDUCE_BASE_HPP
#define SPARSE_ALLREDUCE_BASE_HPP

#include "sparse_allreduce_strategy.hpp"

template<class Dtype, class IType, template<class> class IndicesDistributorType>
struct base_sparse_allreduce_coll :
        virtual base_coll,
        protected sparse_allreduce_strategy_impl<IType,
                                                 IndicesDistributorType>
{
    using ITypeNonMod = typename std::remove_pointer<IType>::type;

    using coll_base = base_coll;
    using coll_strategy = sparse_allreduce_strategy_impl<IType,
                                                         IndicesDistributorType>;

    using coll_base::send_bufs;
    using coll_base::recv_bufs;
    using coll_base::stream;
    using coll_base::single_send_buf;
    using coll_base::single_recv_buf;
    using coll_base::check_values;
    using coll_base::comm;


    using coll_strategy::value_to_indices_ratio;
    using coll_strategy::vdim_size;
    using coll_strategy::minimal_indices_cout;

    ITypeNonMod* recv_ibufs[BUF_COUNT] = { nullptr };
    ITypeNonMod* send_ibufs[BUF_COUNT] = { nullptr };
    ITypeNonMod* single_send_ibuf = nullptr;
    ITypeNonMod* single_recv_ibuf = nullptr;

    size_t* recv_icount = nullptr;
    size_t* recv_vcount = nullptr;
    size_t single_recv_icount {};
    size_t single_recv_vcount {};

    base_sparse_allreduce_coll(const std::string& args) :
     coll_strategy(args, base_coll::comm->size())
    {
        int result = 0;
        result = posix_memalign((void**)&recv_icount, ALIGNMENT,
                                BUF_COUNT * sizeof(size_t) * base_coll::comm->size());
        result = posix_memalign((void**)&recv_vcount, ALIGNMENT,
                                BUF_COUNT * sizeof(size_t) * base_coll::comm->size());

        memset(recv_icount, 1, BUF_COUNT * sizeof(size_t) * base_coll::comm->size());
        memset(recv_vcount, 1, BUF_COUNT * sizeof(size_t) * base_coll::comm->size());
        (void)result;
    }

    virtual ~base_sparse_allreduce_coll()
    {
        free (recv_icount);
        free (recv_vcount);
    }

    const char* name() const noexcept override
    {
        return coll_strategy::class_name();
    }
};

#endif /* SPARSE_ALLREDUCE_BASE_HPP */
