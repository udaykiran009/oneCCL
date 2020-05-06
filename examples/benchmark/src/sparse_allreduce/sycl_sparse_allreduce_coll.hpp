#ifndef SYCL_SPARSE_ALLREDUCE_COLL_HPP
#define SYCL_SPARSE_ALLREDUCE_COLL_HPP

#ifdef CCL_ENABLE_SYCL
template<class kernel_value_type, class kernel_index_type>
struct sparse_allreduce_kernel_name_bufs {};
template<class kernel_value_type, class kernel_index_type>
struct sparse_allreduce_kernel_name_single_bufs {};
    
template<class Dtype, class IType,
          template<class> class IndicesDistributorType = sparse_detail::incremental_indices_distributor>
struct sycl_sparse_allreduce_coll :
        base_sparse_allreduce_coll<cl::sycl::buffer<Dtype, 1>, 
                                   cl::sycl::buffer<IType, 1>,
                                   IndicesDistributorType>
{
    using sycl_indices_t = cl::sycl::buffer<IType, 1>;
    using sycl_values_t = cl::sycl::buffer<Dtype, 1>;
    using coll_base = base_sparse_allreduce_coll<sycl_values_t, 
                                                 sycl_indices_t,
                                                 IndicesDistributorType>;
    using coll_strategy = typename coll_base::coll_strategy;

    using coll_base::send_bufs;
    using coll_base::recv_bufs;
    using coll_base::stream;
    using coll_base::single_send_buf;
    using coll_base::single_send_ibuf;
    using coll_base::single_recv_buf;
    using coll_base::single_recv_ibuf;
    using coll_base::single_recv_icount;
    using coll_base::single_recv_vcount;
    using coll_base::comm;

    using coll_base::recv_ibufs;
    using coll_base::send_ibufs;
    using coll_base::recv_icount;
    using coll_base::recv_vcount;

    sycl_sparse_allreduce_coll(const std::string& args, 
                               size_t sbuf_size_modifier = 1,
                               size_t rbuf_size_modifier = 1) : coll_base(args)
    {
        for (size_t idx = 0; idx < BUF_COUNT; idx++)
        {
            send_bufs[idx] = new sycl_values_t(ELEM_COUNT * sbuf_size_modifier);
            send_ibufs[idx] = new sycl_indices_t(ELEM_COUNT * sbuf_size_modifier);
            recv_bufs[idx] = new sycl_values_t(ELEM_COUNT * rbuf_size_modifier *
                                               base_coll::comm->size());
            recv_ibufs[idx] = new sycl_indices_t(ELEM_COUNT * rbuf_size_modifier * 
                                                 base_coll::comm->size());

            sycl_queue.submit([&](handler& cgh)
            {
                auto send_buf = (static_cast<sycl_values_t*>(send_bufs[idx]));
                auto send_ibuf = (static_cast<sycl_indices_t*>(send_ibufs[idx]));

                auto recv_buf = (static_cast<sycl_values_t*>(recv_bufs[idx]));

                auto send_buf_acc = send_buf->template get_access<mode::write>(cgh);
                auto send_ibuf_acc = send_ibuf->template get_access<mode::write>(cgh);
                auto recv_buf_acc = recv_buf->template get_access<mode::write>(cgh);
                auto recv_ibuf_acc = recv_buf->template get_access<mode::write>(cgh);
                cgh.parallel_for<struct sparse_allreduce_kernel_name_bufs<Dtype,IType>>
                        (range<1>{ELEM_COUNT*base_coll::comm->size()}, [=](item<1> e_idx)
                {
                    if(e_idx.get_linear_id() < ELEM_COUNT)
                    {
                        send_buf_acc[e_idx] = 0;
                        send_ibuf_acc[e_idx] = 0;
                    }
                    recv_buf_acc[e_idx] = 0;
                    recv_ibuf_acc[e_idx] = 0;
                });
            });
        }

        single_send_buf = new sycl_values_t(SINGLE_ELEM_COUNT * sbuf_size_modifier);
        single_recv_buf = new sycl_values_t(SINGLE_ELEM_COUNT * rbuf_size_modifier *
                                            base_coll::comm->size());

        single_send_ibuf = new sycl_indices_t(SINGLE_ELEM_COUNT * sbuf_size_modifier);
        single_recv_ibuf = new sycl_indices_t(SINGLE_ELEM_COUNT * rbuf_size_modifier *
                                              base_coll::comm->size());
        sycl_queue.submit([&](handler& cgh)
        {
            auto send_buf = (static_cast<sycl_values_t*>(single_send_buf));
            auto send_ibuf = (static_cast<sycl_indices_t*>(single_send_ibuf));

            auto recv_buf = (static_cast<sycl_values_t*>(single_recv_buf));

            auto send_buf_acc = send_buf->template get_access<mode::write>(cgh);
            auto send_ibuf_acc = send_ibuf->template get_access<mode::write>(cgh);

            auto recv_buf_acc = recv_buf->template get_access<mode::write>(cgh);
            auto recv_ibuf_acc = recv_buf->template get_access<mode::write>(cgh);
            cgh.parallel_for<struct sparse_allreduce_kernel_name_single_bufs<Dtype, IType>>
                    (range<1>{SINGLE_ELEM_COUNT*base_coll::comm->size()}, [=](item<1> e_idx)
            {
                if(e_idx.get_linear_id() < SINGLE_ELEM_COUNT)
                {
                    send_buf_acc[e_idx] = 0;
                    send_ibuf_acc[e_idx] = 0;
                }
                recv_buf_acc[e_idx] = 0;
                recv_ibuf_acc[e_idx] = 0;
            });
        });
    }

    virtual void prepare(size_t elem_count) override
    {
        //TODO not implemented yet
    }

    virtual void finalize(size_t elem_count) override
    {
        //TODO not implemented yet
    }
    virtual void start(size_t count, size_t buf_idx,
                       const ccl_coll_attr_t& coll_attr,
                       req_list_t& reqs) override
    {
        coll_strategy::start_internal(*comm,
                                      *static_cast<const cl::sycl::buffer<IType> *>(send_ibufs[buf_idx]),
                                      count,
                                      *reinterpret_cast<const cl::sycl::buffer<Dtype> *>(send_bufs[buf_idx]),
                                      count,
                                      static_cast<cl::sycl::buffer<IType>**>(&recv_ibufs[buf_idx]),
                                      &recv_icount[buf_idx],
                                      reinterpret_cast<cl::sycl::buffer<Dtype>**>(&recv_bufs[buf_idx]),
                                      &recv_vcount[buf_idx],
                                      coll_attr, stream, reqs);
    }

    virtual void start_single(size_t count,
                              const ccl_coll_attr_t& coll_attr,
                              req_list_t& reqs) override
    {
        coll_strategy::start_internal(*comm,
                                      *static_cast<const cl::sycl::buffer<IType> *>(single_send_ibuf),
                                      count,
                                      *reinterpret_cast<const cl::sycl::buffer<Dtype> *>(single_send_buf),
                                      count,
                                      static_cast<cl::sycl::buffer<IType>**>(&single_recv_ibuf),
                                      &single_recv_icount,
                                      reinterpret_cast<cl::sycl::buffer<Dtype>**>(&single_recv_buf),
                                      &single_recv_vcount,
                                      coll_attr, stream, reqs);
    }
};
#endif /* CCL_ENABLE_SYCL */

#endif /* SYCL_SPARSE_ALLREDUCE_COLL_HPP */
