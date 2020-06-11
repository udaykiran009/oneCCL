#include <tuple>

#include "sparse_test_algo.hpp"

template<ccl_datatype_t ccl_idx_type>
struct sparse_algo_iterator
{
    template<size_t i, typename v_type>
    void invoke(int sparse_coalesce_mode)
    {
        sparse_test_run<ccl_idx_type, v_type::ccl_type::value>(sparse_coalesce_mode);
    }
};

template<ccl_datatype_t ...ccl_value_type>
struct sparse_value_type_iterator
{
    using types = std::tuple<ccl::type_info<ccl_value_type>...>;
    template<size_t index, typename i_type>
    void invoke(int sparse_coalesce_mode)
    {
        ccl_tuple_for_each_indexed<types>(sparse_algo_iterator<i_type::ccl_type::value>(),
                                          sparse_coalesce_mode);
    }
};

template<ccl_datatype_t ...ccl_index_type>
struct sparce_index_types
{
    using types = std::tuple<ccl::type_info<ccl_index_type>...>;
};

// use -f command line parameter to run example with flag=0 and then with flag=1
int main(int argc, char** argv)
{
    test_init();

    int sparse_coalesce_mode = argc == 3 && std::string(argv[1]) == "-f" ? std::atoi(argv[2]) : 0;

    using supported_sparce_index_types = sparce_index_types<ccl_dtype_char,
                                                            ccl_dtype_int,
                                                            ccl_dtype_int64,
                                                            ccl_dtype_uint64>::types;
#ifdef CCL_BFP16_COMPILER
    using supported_sparce_value_types = sparse_value_type_iterator<ccl_dtype_char,
                                                            ccl_dtype_int,
                                                            ccl_dtype_bfp16,
                                                            ccl_dtype_float,
                                                            ccl_dtype_double,
                                                            ccl_dtype_int64,
                                                            ccl_dtype_uint64>;
#else
    using supported_sparce_value_types = sparse_value_type_iterator<ccl_dtype_char,
                                                            ccl_dtype_int,
                                                            ccl_dtype_float,
                                                            ccl_dtype_double,
                                                            ccl_dtype_int64,
                                                            ccl_dtype_uint64>;
#endif /* CCL_BFP16_COMPILER */

    // run test for each combination of supported indexes and values
    ccl_tuple_for_each_indexed<supported_sparce_index_types>(supported_sparce_value_types(),
                                                             sparse_coalesce_mode);

    test_finalize();

    if (rank == 0)
        printf("PASSED\n");

    return 0;
}
