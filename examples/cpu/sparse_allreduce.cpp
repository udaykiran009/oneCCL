#include <tuple>

#include "sparse_test_algo.hpp"

template <ccl::datatype ccl_idx_type>
struct sparse_algo_iterator {
    template <size_t i, typename v_type>
    void invoke(ccl::sparse_coalesce_mode coalesce_mode,
                sparse_test_callback_mode_t callback_mode,
                ccl::communicator* comm) {
        sparse_test_run<ccl_idx_type, v_type::ccl_type::value>(coalesce_mode, callback_mode, comm);
    }
};

template <ccl::datatype... ccl_value_type>
struct sparse_value_type_iterator {
    using types = std::tuple<ccl::type_info<ccl_value_type>...>;
    template <size_t index, typename i_type>
    void invoke(ccl::sparse_coalesce_mode coalesce_mode,
                sparse_test_callback_mode_t callback_mode,
                ccl::communicator* comm) {
        ccl_tuple_for_each_indexed<types>(
            sparse_algo_iterator<i_type::ccl_type::value>(), coalesce_mode, callback_mode, comm);
    }
};

template <ccl::datatype... ccl_index_type>
struct sparce_index_types {
    using types = std::tuple<ccl::type_info<ccl_index_type>...>;
};

// use -f command line parameter to run example with flag=0 and then with flag=1
int main(int argc, char** argv) {

    ccl::sparse_coalesce_mode coalesce_mode = ccl::sparse_coalesce_mode::regular;
    sparse_test_callback_mode_t callback_mode = sparse_test_callback_completion;

    if (argc >= 3) {
        for (int i = 1; i < argc; i++) {
            if ((i + 1) >= argc)
                break;

            if (std::string(argv[i]) == "-coalesce") {
                if (std::string(argv[i + 1]) == "regular")
                    coalesce_mode = ccl::sparse_coalesce_mode::regular;
                else if (std::string(argv[i + 1]) == "disable")
                    coalesce_mode = ccl::sparse_coalesce_mode::disable;
                else if (std::string(argv[i + 1]) == "keep_precision")
                    coalesce_mode = ccl::sparse_coalesce_mode::keep_precision;
                else {
                    printf("unexpected coalesce option '%s'\n", argv[i + 1]);
                    printf("FAILED\n");
                    return -1;
                }
                i++;
            }

            if (std::string(argv[i]) == "-callback") {
                if (std::string(argv[i + 1]) == "completion")
                    callback_mode = sparse_test_callback_completion;
                else if (std::string(argv[i + 1]) == "alloc")
                    callback_mode = sparse_test_callback_alloc;
                else {
                    printf("unexpected callback option '%s'\n", argv[i + 1]);
                    printf("FAILED\n");
                    return -1;
                }
                i++;
            }
        }
    }

    PRINT("\ncoalesce_mode = %d, callback_mode = %d\n", (int)coalesce_mode, (int)callback_mode);

    using supported_sparce_index_types =
        sparce_index_types<ccl::datatype::int8, ccl::datatype::int32, ccl::datatype::int64, ccl::datatype::uint64>::types;

#ifdef CCL_BF16_COMPILER
    using supported_sparce_value_types
        = sparse_value_type_iterator<ccl::datatype::int8,
                                     ccl::datatype::int32,
                                     ccl::datatype::bfloat16,
                                     ccl::datatype::float32,
                                     ccl::datatype::float64,
                                     ccl::datatype::int64,
                                     ccl::datatype::uint64>;
#else
    using supported_sparce_value_types
        = sparse_value_type_iterator<ccl::datatype::int8,
                                     ccl::datatype::int32,
                                     ccl::datatype::float32,
                                     ccl::datatype::float64,
                                     ccl::datatype::int64,
                                     ccl::datatype::uint64>;
#endif /* CCL_BF16_COMPILER */

    ccl::init();

    int size, rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ccl::shared_ptr_class<ccl::kvs> kvs;
    ccl::kvs::address_type main_addr;
    if (rank == 0) {
        kvs = ccl::create_main_kvs();
        main_addr = kvs->get_address();
        MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else {
        MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
        kvs = ccl::create_kvs(main_addr);
    }

    auto comm = ccl::create_communicator(size, rank, kvs);

    // run test for each combination of supported indexes and values
    ccl_tuple_for_each_indexed<supported_sparce_index_types>(
        supported_sparce_value_types(), coalesce_mode, callback_mode, &comm);

    if (rank == 0)
        printf("PASSED\n");

    MPI_Finalize();

    return 0;
}
