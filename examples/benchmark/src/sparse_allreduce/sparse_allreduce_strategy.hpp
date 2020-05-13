#ifndef SPARSE_ALLREDUCE_STRATEGY_HPP
#define SPARSE_ALLREDUCE_STRATEGY_HPP

/* specific benchmark const expressions */
constexpr size_t default_value_to_indices_ratio = 3;
constexpr size_t default_vdim_size = ELEM_COUNT / 3;

template<class type>
struct type_printer
{
    static constexpr const char* sparse_class_name() { return "sparse_allreduce"; }
};

template<>
struct type_printer<ccl::bfp16>
{
    static constexpr const char* sparse_class_name() { return "sparse_allreduce_bfp16"; }
};

template<class IType, template<class> class IndicesDistributorType>
struct sparse_allreduce_strategy_impl
{
    static constexpr const char* class_name()
    {
        return type_printer<IType>::sparse_class_name();
    }

    template<class T>
    using remove_ptr_t = typename std::remove_pointer<T>::type;
    template<class T>
    using remove_all_t = typename std::remove_const<remove_ptr_t<T>>::type;

    using IndicesDistributor = IndicesDistributorType<remove_all_t<IType>>;

    size_t value_to_indices_ratio;
    size_t vdim_size;
    size_t comm_size;
    const size_t minimal_indices_cout = 1;

    void init_distributor(const std::pair<size_t, size_t>& elem_range)
    {
        size_t indices_count = std::get<0>(get_expected_recv_counts(elem_range.second));

        indices_distributor_impl.reset( new IndicesDistributor(elem_range.first, 
                                                               indices_count));
    }

    sparse_allreduce_strategy_impl(const std::string& args, size_t size) :
        value_to_indices_ratio(),
        vdim_size(),
        comm_size(size)
    {
        std::vector<size_t> default_params { default_value_to_indices_ratio, default_vdim_size};
        if (!args.empty())
        {
            constexpr const char* masks = "[](){}";
            constexpr const char delim = ':';
            std::string arg_copy;
            arg_copy.reserve(args.size());
            std::remove_copy_if(args.begin(), args.end(),
                                std::back_inserter(arg_copy), [](char sym)
                                {
                                    return std::strchr(masks, sym);
                                });
            auto sparse_params = tokenize(arg_copy, delim);
            default_params.resize(std::max(sparse_params.size(), default_params.size()));
            std::transform(sparse_params.begin(), sparse_params.end(), default_params.begin(), [](const std::string& val)
            {
                return std::stoull(val);
            });
        }

        value_to_indices_ratio = default_params[0];
        vdim_size = default_params[1];
    }

    sparse_allreduce_strategy_impl(const allgatherv_strategy_impl&) = delete;
    sparse_allreduce_strategy_impl& operator=(const allgatherv_strategy_impl&) = delete;
    ~sparse_allreduce_strategy_impl() = default;

    std::tuple<size_t, size_t> get_expected_recv_counts(size_t elem_count) const
    {
        size_t indices_count = std::max(elem_count / value_to_indices_ratio,
                                        minimal_indices_cout);
        size_t vdim_count = (elem_count / indices_count);

        return std::tuple<size_t, size_t>( indices_count, indices_count * vdim_count );
    }

    template<class Dtype>
    void start_internal(ccl::communicator& comm, const IType send_ibuf, size_t send_icount,
                        const Dtype send_vbuf, size_t send_vcount,
                        remove_all_t<IType>** recv_ibuf, size_t* recv_icount,
                        remove_all_t<Dtype>** recv_vbuf, size_t* recv_vcount,
                        const ccl_coll_attr_t& coll_attr, ccl::stream_t& stream,
                        req_list_t& reqs)
    {
        //TODO sparse do not works with cache actuall yet
        auto& mod_attr = const_cast<ccl_coll_attr_t&>(coll_attr);
        bool old_to_cache = mod_attr.to_cache;
        mod_attr.to_cache = 0;

        auto expected = get_expected_recv_counts(send_icount);
        *recv_icount = std::get<0>(expected);
        *recv_vcount = std::get<1>(expected);

        reqs.push_back(comm.sparse_allreduce(send_ibuf, std::get<0>(expected),
                                             send_vbuf, send_vcount,
                                             recv_ibuf, recv_icount,
                                             recv_vbuf, recv_vcount,
                                             ccl::reduction::sum,
                                             &coll_attr, stream));
        //TODO sparse do not works with cache actuall yet
        mod_attr.to_cache = old_to_cache;
    }

    std::unique_ptr<IndicesDistributor> indices_distributor_impl;
};

#endif /* SPARSE_ALLREDUCE_STRATEGY_HPP */
