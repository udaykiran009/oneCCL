#include <tuple>

#include "sparse_test_algo.hpp"

template<ccl_datatype_t ccl_idx_type>
struct sparse_algo_iterator
{
    template<size_t i, typename v_type>
    void invoke (const std::string& algo)
    {
        sparse_test_run<ccl_idx_type, v_type::ccl_type::value>(algo);
    }
};

template<ccl_datatype_t ...ccl_value_type>
struct sparse_value_type_iterator
{
    using types = std::tuple<ccl::type_info<ccl_value_type>...>;
    template<size_t index, typename i_type>
    void invoke (const std::string& algo)
    {
        ccl_tuple_for_each_indexed<types>(sparse_algo_iterator<i_type::ccl_type::value>(),
                                          algo);
    }
};

template<ccl_datatype_t ...ccl_index_type>
struct sparce_index_types
{
    using types = std::tuple<ccl::type_info<ccl_index_type>...>;
};

int main()
{
    test_init();

    const char* sparse_algo = std::getenv("CCL_SPARSE_ALLREDUCE");
    if (!sparse_algo)
       sparse_algo = "mask";


    using supported_sparce_index_types = sparce_index_types<ccl_dtype_char,
                                                            ccl_dtype_int,
                                                            ccl_dtype_int64,
                                                            ccl_dtype_uint64>::types;
    using supported_sparce_value_types = sparse_value_type_iterator<ccl_dtype_char,
                                                             ccl_dtype_int,
                                                             ccl_dtype_int64,
                                                             ccl_dtype_uint64,
                                                             ccl_dtype_float,
                                                             ccl_dtype_double>;

    // run test for each combination of supported indexes and values
    ccl_tuple_for_each_indexed<supported_sparce_index_types>(supported_sparce_value_types(),
                                                             sparse_algo);

    test_finalize();

    if (rank == 0)
        printf("PASSED\n");

    return 0;
}
