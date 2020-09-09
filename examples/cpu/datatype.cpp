#include "base.hpp"
#include "mpi.h"
#include "ccl.h"

void check_create_and_free()
{
    auto &env = ccl::environment::instance();
    auto datatype_attr = env.create_datatype_attr(
                                ccl::attr_arg<ccl::ccl_datatype_attributes::size>(1)
                            );

    const size_t max_dtype_count = 16 * 1024;
    const size_t iter_count = 16;
    std::vector<ccl_datatype_t> dtypes(max_dtype_count);

    for (size_t iter = 0; iter < iter_count; iter++)
    {
        dtypes.clear();

        for (size_t idx = 0; idx < max_dtype_count; idx++)
        {
            datatype_attr.set<ccl::ccl_datatype_attributes::size>(idx + 1);
            dtypes[idx] = env.register_datatype(datatype_attr);
            size_t dtype_size = env.get_datatype_size(dtypes[idx]);

            if (dtype_size != (idx + 1))
            {
                printf("FAILED\n");
                throw std::runtime_error("unexpected datatype size: got " +
                        std::to_string(dtype_size) +
                        " expected " + std::to_string((idx + 1)));
            }
        }

        for (size_t idx = 0; idx < max_dtype_count; idx++)
        {
            env.deregister_datatype(dtypes[idx]);
        }
    }
}

int main()
{
    MPI_Init(NULL, NULL);
    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    auto &env = ccl::environment::instance();
    (void)env;

    printf("\n - Check register and unregister\n");
    check_create_and_free();
    printf("PASSED\n");

    MPI_Finalize();
    return 0;
}
