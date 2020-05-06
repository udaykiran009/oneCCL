#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include "base.hpp"
#include "bfp16.h"
#include "coll.hpp"
#include "sparse_detail.hpp"

/* specific benchmark defines */
//different collectives with duplications
#define DEFAULT_COLL_LIST "allgatherv,allreduce,alltoall,alltoallv,bcast,reduce," \
                          "sparse_allreduce,sparse_allreduce_bfp16," \
                          "allgatherv,allreduce,alltoall,alltoallv,bcast,reduce," \
                          "sparse_allreduce,sparse_allreduce_bfp16"

#define DEFAULT_LOOP "regular"

/* specific benchmark types */
typedef enum
{
    LOOP_REGULAR,
    LOOP_UNORDERED
} loop_type_t;

/* specific benchmark const expressions */
constexpr size_t default_value_to_indices_ratio = 3;
constexpr size_t default_vdim_size = ELEM_COUNT / 3;
constexpr const char* help_message = "\nplease specify backend, loop type and comma-separated list of collective names\n\n"
                                     "example:\n\tcpu regular allgatherv,allreduce,sparse_allreduce,sparse_allreduce_bfp16\n"
                                     "example:\n\tsycl unordered bcast,reduce\n"
                                     "\n\n\tThe collectives \"sparse_*\" support additional configuration parameters:\n"
                                     "\n\t\t\"indices_to_value_ratio\" - to produce indices count not more than 'elem_count/indices_to_value_ratio\n"
                                     "\t\t\t(default value is 3)\n"
                                     "\n\t\t\"vdim_count\" - maximum value counts for index\n"
                                     "\t\t\t(default values determines all elapsed elements after \"indices_to_value_ratio\" recalculation application)\n"
                                     "\n\tUser can set this additional parameters to sparse collective in the way:\n"
                                     "\n\t\tsparse_allreduce[4:99]\n"
                                     "\t\t\t - to set \"indices_to_value_ratio\" in 4 and \"vdim_cout\" in 99\n"
                                     "\t\tsparse_allreduce[6]\n"
                                     "\t\t\t - to set \"indices_to_value_ratio\" in 6 and \"vdim_cout\" in default\n"
                                     "\n\tPlease use default configuration in most cases! You do not need to change it in general benchmark case\n";

/* specific benchmark's functions */
std::list<std::string> tokenize(const std::string& input, char delimeter)
{
    std::stringstream ss(input);
    std::list<std::string> ret;
    std::string value;
    while (std::getline(ss, value, delimeter))
    {
        ret.push_back(value);
    }
    return ret;
}

#endif /* BENCHMARK_HPP */
