#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include <getopt.h>
#include <iostream>
#include <iterator>
#include <map>

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

/* specific benchmark types */
typedef enum
{
    LOOP_REGULAR,
    LOOP_UNORDERED
} loop_type_t;

#define DEFAULT_BACKEND ccl::stream_type::host
#define DEFAULT_LOOP    LOOP_REGULAR


std::map<ccl::stream_type, std::string> backend_names =
  {
    std::make_pair(ccl::stream_type::host, "cpu"),
    std::make_pair(ccl::stream_type::device, "sycl") /* TODO: align names */
  };

std::map<loop_type_t, std::string> loop_names =
  {
    std::make_pair(LOOP_REGULAR, "regular"),
    std::make_pair(LOOP_UNORDERED, "unordered")
  };

constexpr const char* help_message = "\nspecify backend, loop type and comma-separated list of collective names\n\n"
                                     "example:\n\t--coll allgatherv,allreduce,sparse_allreduce,sparse_allreduce_bfp16 --backend cpu --loop regular\n"
                                     "example:\n\t--coll bcast,reduce --backend sycl --loop unordered \n"
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

/* specific benchmark functions */
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

template<class Dtype, class Container>
std::string find_str_val(Container& mp, const Dtype& key)
{
    typename std::map<Dtype, std::string>::iterator it;
    it = mp.find(key);
    if (it != mp.end())
         return it->second;
    return NULL;
}

int check_supported_options(const std::string& option_name, const std::string& option_value,
                            const std::set<std::string>& supported_option_values)
{
    std::stringstream sstream;

    if (supported_option_values.find(option_value) == supported_option_values.end())
    {
        PRINT("unsupported %s: %s", option_name.c_str(), option_value.c_str());

        std::copy(supported_option_values.begin(), supported_option_values.end(),
                  std::ostream_iterator<std::string>(sstream, " "));
        PRINT("supported %s: %s", option_name.c_str(), sstream.str().c_str());
        return -1;
    }

    return 0;
}

int set_backend(const std::string& option_value, ccl::stream_type& backend)
{
    std::string option_name = "backend";
    std::set<std::string> supported_option_values { backend_names[ccl::stream_type::host] };

#ifdef CCL_ENABLE_SYCL
    supported_option_values.insert(backend_names[ccl::stream_type::device]);
#endif

    if (check_supported_options(option_name, option_value, supported_option_values))
        return -1;

    backend = (option_value == backend_names[ccl::stream_type::device]) ?
        ccl::stream_type::device : ccl::stream_type::host;

    return 0;
}

int set_loop(const std::string& option_value, loop_type_t& loop)
{
    std::string option_name = "loop";
    std::set<std::string> supported_option_values { loop_names[LOOP_REGULAR], loop_names[LOOP_UNORDERED] };

    if (check_supported_options(option_name, option_value, supported_option_values))
        return -1;

    loop = (option_value == loop_names[LOOP_REGULAR]) ?
        LOOP_REGULAR : LOOP_UNORDERED;

    if (loop == LOOP_UNORDERED)
    {
        setenv("CCL_UNORDERED_COLL", "1", 1);
    }

    return 0;
}

// leave this type here because of tokenize() call
typedef struct user_options_t
{
    ccl::stream_type backend;
    loop_type_t loop;
    std::list<std::string> coll_names;
    int check_values;
} user_options_t;

int parse_user_options(int& argc, char** (&argv), user_options_t& options)
{
    int errors = 0;
    int ch, index;

    // set values by default
    options.backend = ccl::stream_type::host;
    options.loop = LOOP_REGULAR;
    options.coll_names = tokenize(DEFAULT_COLL_LIST, ',');
    options.check_values = 1;

    struct option getopt_options[] =
    {
        { "backend", required_argument, NULL, 'B' },
        { "loop", required_argument, NULL, 'L' },
        { "coll", required_argument, NULL, 'C' }
    };

    while ((ch = getopt_long(argc, argv, "C:B:L:", getopt_options, NULL)) != -1)
    {
        switch (ch)
        {
            case 'B':
                if (set_backend(optarg, options.backend))
                    errors++;
                break;
            case 'L':
                if (set_loop(optarg, options.loop))
                    errors++;
                break;
            case 'C':
                options.coll_names = tokenize(optarg, ',');
                break;
            default:
                errors++;
                break;
        }
    }

    for (index = optind; index < argc; index++)
        errors++;

    if (errors > 0)
    {
        PRINT("failed to parse user options, errors %d", errors);
        return -1;
    }

    return 0;
}

void print_user_options(const user_options_t& options, ccl::communicator* comm)
{
    std::string backend_str = find_str_val(backend_names, options.backend);
    std::string loop_str = find_str_val(loop_names, options.loop);

    std::stringstream sstream;
    std::copy(options.coll_names.begin(), options.coll_names.end(),
              std::ostream_iterator<std::string>(sstream, " "));

    PRINT_BY_ROOT("start colls: %s, iters: %d, buf_count: %d, ranks %zu, check_values %d, backend %s, loop %s",
                  sstream.str().c_str(), ITERS, BUF_COUNT, comm->size(), options.check_values, backend_str.c_str(),
                  loop_str.c_str());
}

#endif /* BENCHMARK_HPP */
