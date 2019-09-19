#include <tuple>

#include "sparse_test_algo.hpp"

template<ccl_datatype_t ccl_idx_type>
struct value_type_iterator
{
    value_type_iterator(const std::string& algo) : algo(algo) {};
    template<typename v_type>
    void operator () (v_type& t)
    {
        sparse_test_run<ccl_idx_type, v_type::ccl_type::value>(algo);
    }
private:
    std::string algo;
};

template<ccl_datatype_t ...ccl_type>
struct index_type_iterator
{
    index_type_iterator(const std::string& algo) : algo(algo) {};
    template<typename i_type>
    void operator () (i_type& t)
    {
        ccl_tuple_for_each(val_types, value_type_iterator<i_type::ccl_type::value>(algo));
    }
private:
    std::string algo;
    std::tuple<type_info<ccl_type>...> val_types;
};


int main()
{
    test_init();

    const char* sparse_algo = std::getenv("CCL_SPARSE_ALLREDUCE");
    if (!sparse_algo)
       sparse_algo = "basic";

    auto idx_types = ccl_type_list<CCL_IDX_TYPE_LIST>();

    ccl_tuple_for_each(idx_types, index_type_iterator<CCL_DATA_TYPE_LIST>(sparse_algo));

    test_finalize();

    if (rank == 0)
        printf("PASSED\n");

    return 0;
}
