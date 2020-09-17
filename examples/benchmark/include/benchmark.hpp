#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include <algorithm>
#include <chrono>
#include <cstring>
#include <getopt.h>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <math.h>
#include <stdexcept>
#include <stdio.h>
#include <sys/time.h>
#include <vector>
#include <fstream>

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
using namespace cl::sycl;
using namespace cl::sycl::access;
#endif /* CCL_ENABLE_SYCL */

#include "base_utils.hpp"
#include "bfp16.h"
#include "coll.hpp"
#include "sparse_allreduce/sparse_detail.hpp"

/* specific benchmark variables */
// TODO: add ccl::bfp16
constexpr std::initializer_list<ccl::datatype> all_dtypes = { ccl::dt_char,  ccl::dt_int,
                                                              ccl::dt_float, ccl::dt_double,
                                                              ccl::dt_int64, ccl::dt_uint64 };

/* specific benchmark defines */
// different collectives with duplications
#define DEFAULT_COLL_LIST \
    "allgatherv,allreduce,alltoall,alltoallv,bcast,reduce," \
    "sparse_allreduce,sparse_allreduce_bfp16," \
    "allgatherv,allreduce,alltoall,alltoallv,bcast,reduce," \
    "sparse_allreduce,sparse_allreduce_bfp16"

#define DEFAULT_DTYPES_LIST "float"
#define ALL_DTYPES_LIST     "char,int,float,double,int64_t,uint64_t"

#define DEFAULT_REDUCTIONS_LIST "sum"
#define ALL_REDUCTIONS_LIST     "sum,prod,min,max"

#define PRINT(fmt, ...) printf(fmt "\n", ##__VA_ARGS__);

#define PRINT_BY_ROOT(comm, fmt, ...) \
    if (comm->rank() == 0) { \
        printf(fmt "\n", ##__VA_ARGS__); \
    }

#define ASSERT(cond, fmt, ...) \
    do { \
        if (!(cond)) { \
            printf("FAILED\n"); \
            fprintf(stderr, "ASSERT '%s' FAILED " fmt "\n", #cond, ##__VA_ARGS__); \
            throw std::runtime_error("ASSERT FAILED"); \
        } \
    } while (0)

/* specific benchmark dtypes */
typedef enum { LOOP_REGULAR, LOOP_UNORDERED } loop_type_t;
typedef enum { BUF_SINGLE, BUF_MULTI } buf_type_t;

#define DEFAULT_BACKEND ccl::stream_type::host
#define DEFAULT_LOOP    LOOP_REGULAR
#define DEFAULT_BUF     BUF_SINGLE

std::map<ccl::stream_type, std::string> backend_names = {
    std::make_pair(ccl::stream_type::host, "cpu"),
    std::make_pair(ccl::stream_type::gpu, "sycl") /* TODO: align names */
};

std::map<loop_type_t, std::string> loop_names = { std::make_pair(LOOP_REGULAR, "regular"),
                                                  std::make_pair(LOOP_UNORDERED, "unordered") };

std::map<buf_type_t, std::string> buf_names = { std::make_pair(BUF_MULTI, "multi"),
                                                std::make_pair(BUF_SINGLE, "single") };

// TODO: add ccl::bfp16
std::map<ccl::datatype, std::string> dtype_names = {
    std::make_pair(ccl::datatype::dt_char, "char"),
    std::make_pair(ccl::datatype::dt_int, "int"),
    std::make_pair(ccl::datatype::dt_float, "float"),
    std::make_pair(ccl::datatype::dt_double, "double"),
    std::make_pair(ccl::datatype::dt_int64, "int64_t"),
    std::make_pair(ccl::datatype::dt_uint64, "uint64_t"),
};

std::map<ccl::reduction, std::string> reduction_names = {
    std::make_pair(ccl::reduction::sum, "sum"),
    std::make_pair(ccl::reduction::prod, "prod"),
    std::make_pair(ccl::reduction::min, "min"),
    std::make_pair(ccl::reduction::max, "max"),
};

// variables for setting dtypes to launch benchmark
// TODO: add ccl::bfp16
template <class native_type>
using checked_dtype_t = std::pair<bool, native_type>;
using supported_dtypes_t = std::tuple<checked_dtype_t<char>,
                                      checked_dtype_t<int>,
                                      checked_dtype_t<float>,
                                      checked_dtype_t<double>,
                                      checked_dtype_t<int64_t>,
                                      checked_dtype_t<uint64_t>>;
supported_dtypes_t launch_dtypes;

/* specific benchmark functions */
void print_help_usage(const char* app) {
    PRINT(
        "\nUSAGE:\n"
        "\t%s [OPTIONS]\n\n"
        "OPTIONS:\n"
        "\t[-b,--backend <backend>]\n"
        "\t[-e,--loop <execution loop>]\n"
        "\t[-l,--coll <collectives list>]\n"
        "\t[-i,--iters <iteration count>]\n"
        "\t[-w,--warmup_iters <warm up iteration count>]\n"
        "\t[-p,--buf_count <number of parallel operations within single collective>]\n"
        "\t[-f,--min_elem_count <minimum number of elements for single collective>]\n"
        "\t[-t,--max_elem_count <maximum number of elements for single collective>]\n"
        "\t[-c,--check <check result correctness>]\n"
        "\t[-v,--v2i_ratio <values to indices ratio in sparse_allreduce>]\n"
        "\t[-d,--dtype <datatypes list/all>]\n"
        "\t[-r,--reduction <reductions list/all>]\n"
        "\t[-n,--buf_type <buffer type>]\n"
        "\t[-o,--csv_filepath <file to store CSV-formatted data into>]\n"
        "\t[-h,--help]\n\n"
        "example:\n\t--coll allgatherv,allreduce,sparse_allreduce,sparse_allreduce_bfp16 --backend cpu --loop regular\n"
        "example:\n\t--coll bcast,reduce --backend sycl --loop unordered \n",
        app);
}

double when(void) {
    struct timeval tv;
    static struct timeval tv_base;
    static int is_first = 1;

    if (gettimeofday(&tv, NULL)) {
        perror("gettimeofday");
        return 0;
    }

    if (is_first) {
        tv_base = tv;
        is_first = 0;
    }
    return (double)(tv.tv_sec - tv_base.tv_sec) * 1.0e6 + (double)(tv.tv_usec - tv_base.tv_usec);
}

std::list<std::string> tokenize(const std::string& input, char delimeter) {
    std::stringstream ss(input);
    std::list<std::string> ret;
    std::string value;
    while (std::getline(ss, value, delimeter)) {
        ret.push_back(value);
    }
    return ret;
}

template <class Dtype, class Container>
std::string find_str_val(Container& mp, const Dtype& key) {
    typename std::map<Dtype, std::string>::iterator it;
    it = mp.find(key);
    if (it != mp.end())
        return it->second;
    return NULL;
}

template <class Dtype, class Container>
bool find_key_val(ccl::reduction& key, Container& mp, const Dtype& val) {
    for (auto& i : mp) {
        if (i.second == val) {
            key = i.first;
            return true;
        }
    }
    return false;
}

int check_supported_options(const std::string& option_name,
                            const std::string& option_value,
                            const std::set<std::string>& supported_option_values) {
    std::stringstream sstream;

    if (supported_option_values.find(option_value) == supported_option_values.end()) {
        PRINT("unsupported %s: %s", option_name.c_str(), option_value.c_str());

        std::copy(supported_option_values.begin(),
                  supported_option_values.end(),
                  std::ostream_iterator<std::string>(sstream, " "));
        PRINT("supported %s: %s", option_name.c_str(), sstream.str().c_str());
        return -1;
    }

    return 0;
}

int set_backend(const std::string& option_value, ccl::stream_type& backend) {
    std::string option_name = "backend";
    std::set<std::string> supported_option_values{ backend_names[ccl::stream_type::host] };

#ifdef CCL_ENABLE_SYCL
    supported_option_values.insert(backend_names[ccl::stream_type::gpu]);
#endif

    if (check_supported_options(option_name, option_value, supported_option_values))
        return -1;

    backend = (option_value == backend_names[ccl::stream_type::gpu]) ? ccl::stream_type::gpu
                                                                     : ccl::stream_type::host;

    return 0;
}

int set_loop(const std::string& option_value, loop_type_t& loop) {
    std::string option_name = "loop";
    std::set<std::string> supported_option_values{ loop_names[LOOP_REGULAR],
                                                   loop_names[LOOP_UNORDERED] };

    if (check_supported_options(option_name, option_value, supported_option_values))
        return -1;

    loop = (option_value == loop_names[LOOP_REGULAR]) ? LOOP_REGULAR : LOOP_UNORDERED;

    if (loop == LOOP_UNORDERED) {
        setenv("CCL_UNORDERED_COLL", "1", 1);
    }

    return 0;
}

int set_buf_type(const std::string& option_value, buf_type_t& buf) {
    std::string option_name = "buf_type";
    std::set<std::string> supported_option_values{ buf_names[BUF_SINGLE], buf_names[BUF_MULTI] };

    if (check_supported_options(option_name, option_value, supported_option_values))
        return -1;

    buf = (option_value == buf_names[BUF_SINGLE]) ? BUF_SINGLE : BUF_MULTI;

    return 0;
}

// leave this dtype here because of tokenize() call
typedef struct user_options_t {
    ccl::stream_type backend;
    loop_type_t loop;
    size_t iters;
    size_t warmup_iters;
    size_t buf_count;
    size_t min_elem_count;
    size_t max_elem_count;
    int check_values;
    buf_type_t buf_type;
    size_t v2i_ratio;
    std::list<std::string> coll_names;
    std::list<std::string> dtypes;
    std::list<std::string> reductions;
    std::string csv_filepath;

    user_options_t() {
        backend = ccl::stream_type::host;
        loop = LOOP_REGULAR;
        coll_names = tokenize(DEFAULT_COLL_LIST, ',');
        iters = ITERS;
        warmup_iters = WARMUP_ITERS;
        buf_count = BUF_COUNT;
        min_elem_count = 1;
        max_elem_count = MAX_ELEM_COUNT;
        check_values = 1;
        buf_type = DEFAULT_BUF;
        v2i_ratio = V2I_RATIO;
        dtypes = tokenize(DEFAULT_DTYPES_LIST, ','); // default: float
        reductions = tokenize(DEFAULT_REDUCTIONS_LIST, ','); // default: sum
        csv_filepath = std::string();
    }
} user_options_t;

/* placing print_timings() here is because of declaration of user_options_t */
// FIXME FS: what?
void print_timings(ccl::communicator& comm,
                   const std::vector<double>& timer,
                   const user_options_t& options,
                   const size_t elem_count,
                   ccl::datatype dtype,
                   ccl::reduction op) {
    const size_t buf_count = options.buf_type == BUF_SINGLE ? 1 : options.buf_count;
    const size_t ncolls = options.coll_names.size();
    std::vector<double> all_timers(ncolls * comm.size());
    std::vector<size_t> recv_counts(comm.size());

    size_t idx;
    for (idx = 0; idx < comm.size(); idx++)
        recv_counts[idx] = ncolls;

    ccl::coll_attr attr;
    memset((void*)&attr, 0, sizeof(ccl_coll_attr_t));

    comm.allgatherv(timer.data(), ncolls, all_timers.data(), recv_counts.data(), &attr, nullptr)
        ->wait();

    if (comm.rank() == 0) {
        std::vector<double> timers(comm.size(), 0);
        for (size_t r = 0; r < comm.size(); ++r) {
            for (size_t c = 0; c < ncolls; ++c) {
                timers[r] += all_timers[r * ncolls + c];
            }
        }
        double avg_timer(0);
        double avg_timer_per_buf(0);
        for (idx = 0; idx < comm.size(); idx++) {
            avg_timer += timers[idx];
        }
        avg_timer /= (options.iters * comm.size());
        avg_timer_per_buf = avg_timer / buf_count;

        double stddev_timer = 0;
        double sum = 0;
        for (idx = 0; idx < comm.size(); idx++) {
            double val = timers[idx] / options.iters;
            sum += (val - avg_timer) * (val - avg_timer);
        }
        stddev_timer = sqrt(sum / comm.size()) / avg_timer * 100;
        if (options.buf_type == BUF_SINGLE) {
            printf("%10zu %12.2lf %11.1lf\n",
                   elem_count * ccl::datatype_get_size(dtype) * buf_count,
                   avg_timer,
                   stddev_timer);
        }
        else {
            printf("%10zu %13.2lf %18.2lf %11.1lf\n",
                   elem_count * ccl::datatype_get_size(dtype) * buf_count,
                   avg_timer,
                   avg_timer_per_buf,
                   stddev_timer);
        }

        // in case csv export is requested
        // we write one line per collop, dtype and reduction
        // hence average is per collop, not the aggregate over all
        if (!options.csv_filepath.empty()) {
            std::ofstream csvf;
            csvf.open(options.csv_filepath, std::ios::app);
            if (csvf.is_open()) {
                std::vector<double> avg_timer(ncolls, 0);
                for (size_t r = 0; r < comm.size(); ++r) {
                    for (size_t c = 0; c < ncolls; ++c) {
                        avg_timer[c] += all_timers[r * ncolls + c];
                    }
                }
                for (size_t c = 0; c < ncolls; ++c) {
                    avg_timer[c] /= (options.iters * comm.size());
                }

                int idx = 0;
                for (auto cop = options.coll_names.begin(); cop != options.coll_names.end();
                     ++cop, ++idx) {
                    csvf << comm.size() << "," << (*cop) << "," << reduction_names[op] << ","
                         << dtype_names[dtype] << "," << ccl::datatype_get_size(dtype) << ","
                         << elem_count << "," << buf_count << "," << avg_timer[idx] << std::endl;
                }
                csvf.close();
            }
        }
    }
    comm.barrier();
}

/* specific benchmark functors */
class set_dtypes_func {
private:
    const std::list<std::string>& dtypes;

public:
    set_dtypes_func(const std::list<std::string>& dtypes) : dtypes(dtypes) {}

    template <class Dtype>
    void operator()(checked_dtype_t<Dtype>& val) {
        auto it = std::find(dtypes.begin(), dtypes.end(), ccl::native_type_info<Dtype>::name());
        if (it != std::end(dtypes)) {
            val.first = true;
        }
    }
};

int parse_user_options(int& argc, char**(&argv), user_options_t& options) {
    int ch;
    int errors = 0;

    // values needed by getopt
    const char* const short_options = "b:e:i:w:p:f:t:c:v:l:d:r:n:o:h:";
    struct option getopt_options[] = {
        { "backend", required_argument, 0, 'b' },
        { "loop", required_argument, 0, 'e' },
        { "iters", required_argument, 0, 'i' },
        { "warmup_iters", required_argument, 0, 'w' },
        { "buf_count", required_argument, 0, 'p' },
        { "min_elem_count", required_argument, 0, 'f' },
        { "max_elem_count", required_argument, 0, 't' },
        { "check", required_argument, 0, 'c' },
        { "v2i_ratio", required_argument, 0, 'v' },
        { "coll", required_argument, 0, 'l' },
        { "dtype", required_argument, 0, 'd' },
        { "reduction", required_argument, 0, 'r' },
        { "buf_type", required_argument, 0, 'n' },
        { "csv_filepath", required_argument, 0, 'o' },
        { "help", no_argument, 0, 'h' },
        { 0, 0, 0, 0 } // required at end of array.
    };

    while ((ch = getopt_long(argc, argv, short_options, getopt_options, NULL)) != -1) {
        switch (ch) {
            case 'b':
                if (set_backend(optarg, options.backend))
                    errors++;
                break;
            case 'e':
                if (set_loop(optarg, options.loop))
                    errors++;
                break;
            case 'i': options.iters = atoll(optarg); break;
            case 'w': options.warmup_iters = atoll(optarg); break;
            case 'p': options.buf_count = atoll(optarg); break;
            case 'f': options.min_elem_count = atoll(optarg); break;
            case 't': options.max_elem_count = atoll(optarg); break;
            case 'c': options.check_values = atoi(optarg); break;
            case 'v': options.v2i_ratio = atoll(optarg); break;
            case 'l': options.coll_names = tokenize(optarg, ','); break;
            case 'd':
                if (strcmp("all", optarg) == 0) {
                    options.dtypes = tokenize(ALL_DTYPES_LIST, ',');
                }
                else
                    options.dtypes = tokenize(optarg, ',');
                break;
            case 'r':
                if (strcmp("all", optarg) == 0) {
                    options.reductions = tokenize(ALL_REDUCTIONS_LIST, ',');
                }
                else
                    options.reductions = tokenize(optarg, ',');
                break;
            case 'n':
                if (set_buf_type(optarg, options.buf_type))
                    errors++;
                break;
            case 'o': options.csv_filepath = std::string(optarg); break;
            case 'h': print_help_usage(argv[0]); return -1;
            default: errors++; break;
        }
    }

    if (optind < argc) {
        PRINT("non-option ARGV-elements given");
        errors++;
    }

    if (errors > 0) {
        PRINT("failed to parse user options, errors %d", errors);
        print_help_usage(argv[0]);
        return -1;
    }

    return 0;
}

void print_user_options(const user_options_t& options, ccl::communicator* comm) {
    std::stringstream ss;
    ss << "colls:          ";
    std::copy(options.coll_names.begin(),
              options.coll_names.end(),
              std::ostream_iterator<std::string>(ss, " "));
    ss << "\n  dtypes:         ";
    std::copy(
        options.dtypes.begin(), options.dtypes.end(), std::ostream_iterator<std::string>(ss, " "));
    ss << "\n  reductions:     ";
    std::copy(options.reductions.begin(),
              options.reductions.end(),
              std::ostream_iterator<std::string>(ss, " "));

    std::string backend_str = find_str_val(backend_names, options.backend);
    std::string loop_str = find_str_val(loop_names, options.loop);
    std::string buf_type_str = find_str_val(buf_names, options.buf_type);

    PRINT_BY_ROOT(comm,
                  "options:"
                  "\n  ranks:          %zu"
                  "\n  backend:        %s"
                  "\n  loop:           %s"
                  "\n  iters:          %zu"
                  "\n  warmup_iters:   %zu"
                  "\n  buf_count:      %zu"
                  "\n  min_elem_count: %zu"
                  "\n  max_elem_count: %zu"
                  "\n  check:          %d"
                  "\n  buf_type:       %s"
                  "\n  v2i_ratio:      %zu"
                  "\n  %s"
                  "\n  csv_filepath:   %s",
                  comm->size(),
                  backend_str.c_str(),
                  loop_str.c_str(),
                  options.iters,
                  options.warmup_iters,
                  options.buf_count,
                  options.min_elem_count,
                  options.max_elem_count,
                  options.check_values,
                  buf_type_str.c_str(),
                  options.v2i_ratio,
                  ss.str().c_str(),
                  options.csv_filepath.c_str());
}

#endif /* BENCHMARK_HPP */
