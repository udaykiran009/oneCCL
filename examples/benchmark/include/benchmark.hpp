#pragma once

#include <algorithm>
#include <chrono>
#include <cstring>
#include <getopt.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <iomanip>
#include <numeric>
#include <map>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <cstdio>
#include <sys/time.h>
#include <vector>

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
using namespace cl::sycl;
using namespace cl::sycl::access;
#endif /* CCL_ENABLE_SYCL */

#include "base.hpp"
#include "base_utils.hpp"
#include "bf16.hpp"
#include "coll.hpp"
#include "sparse_allreduce/sparse_detail.hpp"

void print_help_usage(const char* app) {
    PRINT("\nUSAGE:\n"
          "\t%s [OPTIONS]\n\n"
          "OPTIONS:\n"
          "\t[-b,--backend <backend>]: %s\n"
          "\t[-e,--loop <execution loop>]: %s\n"
          "\t[-i,--iters <iteration count>]: %d\n"
          "\t[-w,--warmup_iters <warm up iteration count>]: %d\n"
          "\t[-n,--buf_count <number of parallel operations within single collective>]: %d\n"
          "\t[-f,--min_elem_count <minimum number of elements for single collective>]: %d\n"
          "\t[-t,--max_elem_count <maximum number of elements for single collective>]: %d\n"
          "\t[-y,--elem_counts <list of element counts for single collective>]: [%d-%d]\n"
          "\t[-c,--check <check result correctness>]: %d\n"
          "\t[-p,--cache <use persistent operations>]: %d\n"
#ifdef CCL_ENABLE_SYCL
          "\t[-a,--sycl_dev_type <sycl device type>]: %s\n"
          "\t[-m,--sycl_mem_type <sycl memory type>]: %s\n"
          "\t[-u,--sycl_usm_type <sycl usm type>]: %s\n"
#endif
          "\t[-k,--ranks_per_proc <number of ranks per process>]: %d\n"
          "\t[-l,--coll <collectives list/all>]: %s\n"
          "\t[-d,--dtype <datatypes list/all>]: %s\n"
          "\t[-r,--reduction <reductions list/all>]: %s\n"
          "\t[-o,--csv_filepath <file to store CSV-formatted data into>]: %s\n"
          "\t[-h,--help]\n\n"
          "example:\n\t--coll allgatherv,allreduce --backend host --loop regular\n"
          "example:\n\t--coll bcast,reduce --backend sycl --loop unordered \n",
          app,
          backend_names[DEFAULT_BACKEND].c_str(),
          loop_names[DEFAULT_LOOP].c_str(),
          DEFAULT_ITERS,
          DEFAULT_WARMUP_ITERS,
          DEFAULT_BUF_COUNT,
          DEFAULT_MIN_ELEM_COUNT,
          DEFAULT_MAX_ELEM_COUNT,
          DEFAULT_MIN_ELEM_COUNT,
          DEFAULT_MAX_ELEM_COUNT,
          DEFAULT_CHECK_VALUES,
          DEFAULT_CACHE_OPS,
#ifdef CCL_ENABLE_SYCL
          sycl_dev_names[DEFAULT_SYCL_DEV_TYPE].c_str(),
          sycl_mem_names[DEFAULT_SYCL_MEM_TYPE].c_str(),
          sycl_usm_names[DEFAULT_SYCL_USM_TYPE].c_str(),
#endif
          DEFAULT_RANKS_PER_PROC,
          DEFAULT_COLL_LIST,
          DEFAULT_DTYPES_LIST,
          DEFAULT_REDUCTIONS_LIST,
          DEFAULT_CSV_FILEPATH);
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
        PRINT("supported values: %s", sstream.str().c_str());
        return -1;
    }

    return 0;
}

int set_backend(const std::string& option_value, backend_type_t& backend) {
    std::string option_name = "backend";
    std::set<std::string> supported_option_values{ backend_names[BACKEND_HOST] };

#ifdef CCL_ENABLE_SYCL
    supported_option_values.insert(backend_names[BACKEND_SYCL]);
#endif

    if (check_supported_options(option_name, option_value, supported_option_values))
        return -1;

    backend = (option_value == backend_names[BACKEND_SYCL]) ? BACKEND_SYCL : BACKEND_HOST;

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

#ifdef CCL_ENABLE_SYCL
int set_sycl_dev_type(const std::string& option_value, sycl_dev_type_t& dev) {
    std::string option_name = "sycl_dev_type";
    std::set<std::string> supported_option_values{ sycl_dev_names[SYCL_DEV_HOST],
                                                   sycl_dev_names[SYCL_DEV_CPU],
                                                   sycl_dev_names[SYCL_DEV_GPU] };

    if (check_supported_options(option_name, option_value, supported_option_values))
        return -1;

    if (option_value == sycl_dev_names[SYCL_DEV_HOST])
        dev = SYCL_DEV_HOST;
    else if (option_value == sycl_dev_names[SYCL_DEV_CPU])
        dev = SYCL_DEV_CPU;
    else if (option_value == sycl_dev_names[SYCL_DEV_GPU])
        dev = SYCL_DEV_GPU;

    return 0;
}

int set_sycl_mem_type(const std::string& option_value, sycl_mem_type_t& mem) {
    std::string option_name = "sycl_mem_type";
    std::set<std::string> supported_option_values{ sycl_mem_names[SYCL_MEM_USM],
                                                   /*sycl_mem_names[SYCL_MEM_BUF]*/ };

    if (check_supported_options(option_name, option_value, supported_option_values))
        return -1;

    mem = (option_value == sycl_mem_names[SYCL_MEM_USM]) ? SYCL_MEM_USM : SYCL_MEM_BUF;

    return 0;
}

int set_sycl_usm_type(const std::string& option_value, sycl_usm_type_t& usm) {
    std::string option_name = "sycl_usm_type";
    std::set<std::string> supported_option_values{ sycl_usm_names[SYCL_USM_SHARED],
                                                   sycl_usm_names[SYCL_USM_DEVICE] };

    if (check_supported_options(option_name, option_value, supported_option_values))
        return -1;

    usm = (option_value == sycl_usm_names[SYCL_USM_SHARED]) ? SYCL_USM_SHARED : SYCL_USM_DEVICE;

    return 0;
}
#endif /* CCL_ENABLE_SYCL */

int set_datatypes(std::string option_value, int check_values, std::list<std::string>& datatypes) {
    datatypes.clear();
    if (option_value == "all") {
        if (check_values) {
            datatypes = tokenize<std::string>(ALL_DTYPES_LIST_WITH_CHECK, ',');
        }
        else {
            datatypes = tokenize<std::string>(ALL_DTYPES_LIST, ',');
        }
    }
    else {
        datatypes = tokenize<std::string>(option_value, ',');

        std::string option_name = "dtype";
        std::set<std::string> supported_option_values;

        for (auto p : dtype_names) {
            if ((p.first == ccl::datatype::float16 || p.first == ccl::datatype::bfloat16) &&
                check_values)
                continue;
            supported_option_values.insert(p.second);
        }

        for (auto dt : datatypes) {
            if (check_supported_options(option_name, dt, supported_option_values)) {
                if ((dt == dtype_names[ccl::datatype::float16] ||
                     dt == dtype_names[ccl::datatype::bfloat16]) &&
                    check_values) {
                    PRINT("WARN: correctness checking is not implemented for '%s'", dt.c_str());
                }
            }
        }
    }
    return 0;
}

int set_reductions(std::string option_value, int check_values, std::list<std::string>& reductions) {
    reductions.clear();
    if (option_value == "all") {
        if (check_values) {
            reductions = tokenize<std::string>(ALL_REDUCTIONS_LIST_WITH_CHECK, ',');
        }
        else {
            reductions = tokenize<std::string>(ALL_REDUCTIONS_LIST, ',');
        }
    }
    else {
        reductions = tokenize<std::string>(option_value, ',');

        std::string option_name = "reduction";
        std::set<std::string> supported_option_values;

        for (auto p : reduction_names) {
            if ((p.first != ccl::reduction::sum) && check_values)
                continue;
            supported_option_values.insert(p.second);
        }

        for (auto r : reductions) {
            if (check_supported_options(option_name, r, supported_option_values)) {
                if ((r != reduction_names[ccl::reduction::sum]) && check_values) {
                    PRINT("WARN: correctness checking is not implemented for '%s'", r.c_str());
                }
            }
        }
    }
    return 0;
}

size_t get_iter_count(size_t bytes, size_t max_iter_count) {
    size_t n, res = max_iter_count;
    n = bytes >> 18;
    while (n) {
        res >>= 1;
        n >>= 1;
    }

    if (!res && max_iter_count)
        res = 1;

    return res;
}

void store_to_csv(const user_options_t& options,
                  size_t nranks,
                  size_t elem_count,
                  size_t iter_count,
                  ccl::datatype dtype,
                  ccl::reduction op,
                  double min_time,
                  double max_time,
                  double avg_time,
                  double stddev) {
    std::ofstream csvf;
    csvf.open(options.csv_filepath, std::ofstream::out | std::ofstream::app);

    if (csvf.is_open()) {
        const size_t buf_count = options.buf_count;

        for (const auto& cop : options.coll_names) {
            auto get_op_name = [&]() {
                if (cop == "allreduce" || cop == "reduce_scatter" || cop == "reduce") {
                    return reduction_names.at(op);
                }
                return std::string{};
            };

            csvf << nranks << "," << cop << "," << get_op_name() << "," << dtype_names.at(dtype)
                 << "," << ccl::get_datatype_size(dtype) << "," << elem_count << "," << buf_count
                 << "," << iter_count << "," << min_time << "," << max_time << "," << avg_time
                 << "," << stddev << std::endl;
        }
        csvf.close();
    }
}

/* timer array contains one number per collective, one collective corresponds to ranks_per_proc */
void print_timings(const ccl::communicator& comm,
                   const std::vector<double>& local_timers,
                   const user_options_t& options,
                   size_t elem_count,
                   size_t iter_count,
                   ccl::datatype dtype,
                   ccl::reduction op) {
    const size_t buf_count = options.buf_count;
    const size_t ncolls = options.coll_names.size();
    const size_t nranks = comm.size();

    // get timers from other ranks
    std::vector<double> all_ranks_timers(ncolls * nranks);
    std::vector<size_t> recv_counts(nranks, ncolls);
    ccl::allgatherv(local_timers.data(), ncolls, all_ranks_timers.data(), recv_counts, comm).wait();

    if (comm.rank() == 0) {
        std::vector<double> timers(nranks, 0);
        std::vector<double> min_timers(ncolls, 0);
        std::vector<double> max_timers(ncolls, 0);

        // parse timers from all ranks
        for (size_t rank_idx = 0; rank_idx < nranks; ++rank_idx) {
            for (size_t coll_idx = 0; coll_idx < ncolls; ++coll_idx) {
                const double time = all_ranks_timers.at(rank_idx * ncolls + coll_idx);
                timers.at(rank_idx) += time;

                double& min = min_timers.at(coll_idx);
                min = (min != 0) ? std::min(min, time) : time;

                double& max = max_timers.at(coll_idx);
                max = std::max(max, time);
            }
        }

        double avg_time = std::accumulate(timers.begin(), timers.end(), 0);
        avg_time /= (iter_count * nranks);

        double sum = 0;
        for (const double& timer : timers) {
            double latency = (double)timer / iter_count;
            sum += (latency - avg_time) * (latency - avg_time);
        }
        double stddev = std::sqrt((double)sum / nranks) / avg_time * 100;

        double min_time = std::accumulate(min_timers.begin(), min_timers.end(), 0);
        min_time /= iter_count;

        double max_time = std::accumulate(max_timers.begin(), max_timers.end(), 0);
        max_time /= iter_count;

        size_t bytes = elem_count * ccl::get_datatype_size(dtype) * buf_count;
        std::cout << std::right << std::fixed << std::setw(COL_WIDTH) << bytes
                  << std::setw(COL_WIDTH) << iter_count << std::setw(COL_WIDTH)
                  << std::setprecision(COL_PRECISION) << min_time << std::setw(COL_WIDTH)
                  << std::setprecision(COL_PRECISION) << max_time << std::setw(COL_WIDTH)
                  << std::setprecision(COL_PRECISION) << avg_time << std::setw(COL_WIDTH)
                  << std::setprecision(COL_PRECISION) << stddev << std::endl;

        if (!options.csv_filepath.empty()) {
            store_to_csv(options,
                         nranks,
                         elem_count,
                         iter_count,
                         dtype,
                         op,
                         min_time,
                         max_time,
                         avg_time,
                         stddev);
        }
    }

    ccl::barrier(comm);
}

void adjust_elem_counts(user_options_t& options) {
    if (options.max_elem_count < options.min_elem_count)
        options.max_elem_count = options.min_elem_count;

    if (options.elem_counts_set) {
        /* adjust min/max_elem_count or elem_counts */
        if (options.min_elem_count_set) {
            /* apply user-supplied count as limiter */
            options.elem_counts.remove_if([&options](const size_t& count) {
                return (count < options.min_elem_count);
            });
        }
        else {
            if (options.elem_counts.empty())
                options.min_elem_count = 0;
            else
                options.min_elem_count =
                    *(std::min_element(options.elem_counts.begin(), options.elem_counts.end()));
        }
        if (options.max_elem_count_set) {
            /* apply user-supplied count as limiter */
            options.elem_counts.remove_if([&options](const size_t& count) {
                return (count > options.max_elem_count);
            });
        }
        else {
            if (options.elem_counts.empty())
                options.max_elem_count = options.min_elem_count;
            else
                options.max_elem_count =
                    *(std::max_element(options.elem_counts.begin(), options.elem_counts.end()));
        }
    }
    else {
        generate_counts(options.elem_counts, options.min_elem_count, options.max_elem_count);
    }
}

bool is_valid_integer_option(const char* option) {
    std::string str(option);
    bool only_digits = (str.find_first_not_of("0123456789") == std::string::npos);
    return (only_digits && atoi(option) >= 0);
}

bool is_valid_integer_option(int option) {
    return (option >= 0);
}

void adjust_user_options(user_options_t& options) {
    adjust_elem_counts(options);
}

int parse_user_options(int& argc, char**(&argv), user_options_t& options) {
    int ch;
    int errors = 0;
    std::list<int> elem_counts_int;

    bool should_parse_datatypes = false;
    bool should_parse_reductions = false;

#ifdef CCL_ENABLE_SYCL
    const char* const short_options = "b:e:i:w:n:f:t:c:p:o:a:m:u:k:l:d:r:y:h";
#else
    const char* const short_options = "b:e:i:w:n:f:t:c:p:o:k:l:d:r:y:h";
#endif

    struct option getopt_options[] = {
        { "backend", required_argument, 0, 'b' },
        { "loop", required_argument, 0, 'e' },
        { "iters", required_argument, 0, 'i' },
        { "warmup_iters", required_argument, 0, 'w' },
        { "buf_count", required_argument, 0, 'n' },
        { "min_elem_count", required_argument, 0, 'f' },
        { "max_elem_count", required_argument, 0, 't' },
        { "elem_counts", required_argument, 0, 'y' },
        { "check", required_argument, 0, 'c' },
        { "cache", required_argument, 0, 'p' },
    /*{ "v2i_ratio", required_argument, 0, 'v' },*/
#ifdef CCL_ENABLE_SYCL
        { "sycl_dev_type", required_argument, 0, 'a' },
        { "sycl_mem_type", required_argument, 0, 'm' },
        { "sycl_usm_type", required_argument, 0, 'u' },
#endif
        { "ranks_per_proc", required_argument, 0, 'k' },
        { "coll", required_argument, 0, 'l' },
        { "dtype", required_argument, 0, 'd' },
        { "reduction", required_argument, 0, 'r' },
        { "csv_filepath", required_argument, 0, 'o' },
        { "help", no_argument, 0, 'h' },
        { 0, 0, 0, 0 } // required at end of array.
    };

    while ((ch = getopt_long(argc, argv, short_options, getopt_options, NULL)) != -1) {
        switch (ch) {
            case 'b':
                if (set_backend(optarg, options.backend)) {
                    PRINT("failed to parse 'backend' option");
                    errors++;
                }
                break;
            case 'e':
                if (set_loop(optarg, options.loop)) {
                    PRINT("failed to parse 'loop' option");
                    errors++;
                }
                break;
            case 'i':
                if (is_valid_integer_option(optarg)) {
                    options.iters = atoll(optarg);
                }
                else
                    errors++;
                break;
            case 'w':
                if (is_valid_integer_option(optarg)) {
                    options.warmup_iters = atoll(optarg);
                }
                else
                    errors++;
                break;
            case 'n':
                if (is_valid_integer_option(optarg)) {
                    options.buf_count = atoll(optarg);
                }
                else
                    errors++;
                break;
            case 'f':
                if (is_valid_integer_option(optarg)) {
                    options.min_elem_count = atoll(optarg);
                    options.min_elem_count_set = true;
                }
                else
                    errors++;
                break;
            case 't':
                if (is_valid_integer_option(optarg)) {
                    options.max_elem_count = atoll(optarg);
                    options.max_elem_count_set = true;
                }
                else
                    errors++;
                break;
            case 'y':
                elem_counts_int = tokenize<int>(optarg, ',');
                elem_counts_int.remove_if([](const size_t& count) {
                    return !is_valid_integer_option(count);
                });
                options.elem_counts = tokenize<size_t>(optarg, ',');
                if (elem_counts_int.size() == options.elem_counts.size())
                    options.elem_counts_set = true;
                else
                    errors++;
                break;
            case 'c': options.check_values = atoi(optarg); break;
            case 'p':
                options.cache_ops = atoi(optarg);
                break;
                /*case 'v': options.v2i_ratio = atoll(optarg); break;*/
#ifdef CCL_ENABLE_SYCL
            case 'a':
                if (set_sycl_dev_type(optarg, options.sycl_dev_type)) {
                    PRINT("failed to parse 'sycl_dev_type' option");
                    errors++;
                }
                break;
            case 'm':
                if (set_sycl_mem_type(optarg, options.sycl_mem_type)) {
                    PRINT("failed to parse 'sycl_mem_type' option");
                    errors++;
                }
                break;
            case 'u':
                if (set_sycl_usm_type(optarg, options.sycl_usm_type)) {
                    PRINT("failed to parse 'sycl_usm_type' option");
                    errors++;
                }
                break;
#endif
            case 'k':
                if (is_valid_integer_option(optarg)) {
                    options.ranks_per_proc = atoll(optarg);
                }
                else
                    errors++;
                break;
            case 'l':
                if (strcmp("all", optarg) == 0) {
                    options.coll_names = tokenize<std::string>(ALL_COLLS_LIST, ',');
                }
                else
                    options.coll_names = tokenize<std::string>(optarg, ',');
                break;
            case 'd':
                options.dtypes.clear();
                options.dtypes.push_back(optarg);
                should_parse_datatypes = true;
                break;
            case 'r':
                options.reductions.clear();
                options.reductions.push_back(optarg);
                should_parse_reductions = true;
                break;
            case 'o': options.csv_filepath = std::string(optarg); break;
            case 'h': return -1;
            default:
                PRINT("failed to parse unknown option");
                errors++;
                break;
        }
    }

    if (optind < argc) {
        PRINT("non-option ARGV-elements given");
        errors++;
    }

    if (should_parse_datatypes &&
        set_datatypes(options.dtypes.back(), options.check_values, options.dtypes)) {
        PRINT("failed to parse 'dtype' option");
        errors++;
    }

    if (should_parse_reductions &&
        set_reductions(options.reductions.back(), options.check_values, options.reductions)) {
        PRINT("failed to parse 'reduction' option");
        errors++;
    }

    if (errors > 0) {
        PRINT("found %d errors while parsing user options", errors);
        for (int idx = 0; idx < argc; idx++) {
            PRINT("arg %d: %s", idx, argv[idx]);
        }
        return -1;
    }

    adjust_user_options(options);

    return 0;
}

void print_user_options(const user_options_t& options, const ccl::communicator& comm) {
    std::stringstream ss;
    std::string elem_counts_str;
    std::string collectives_str;
    std::string datatypes_str;
    std::string reductions_str;

    ss.str("");
    ss << "[";
    std::copy(options.elem_counts.begin(),
              options.elem_counts.end(),
              std::ostream_iterator<size_t>(ss, " "));
    if (*ss.str().rbegin() == ' ')
        ss.seekp(-1, std::ios_base::end);
    ss << "]";
    elem_counts_str = ss.str();
    ss.str("");

    std::copy(options.coll_names.begin(),
              options.coll_names.end(),
              std::ostream_iterator<std::string>(ss, " "));
    collectives_str = ss.str();
    ss.str("");

    std::copy(
        options.dtypes.begin(), options.dtypes.end(), std::ostream_iterator<std::string>(ss, " "));
    datatypes_str = ss.str();
    ss.str("");

    std::copy(options.reductions.begin(),
              options.reductions.end(),
              std::ostream_iterator<std::string>(ss, " "));
    reductions_str = ss.str();
    ss.str("");

    std::string backend_str = find_str_val(backend_names, options.backend);
    std::string loop_str = find_str_val(loop_names, options.loop);

#ifdef CCL_ENABLE_SYCL
    std::string sycl_dev_type_str = find_str_val(sycl_dev_names, options.sycl_dev_type);
    std::string sycl_mem_type_str = find_str_val(sycl_mem_names, options.sycl_mem_type);
    std::string sycl_usm_type_str = find_str_val(sycl_usm_names, options.sycl_usm_type);
#endif

    PRINT_BY_ROOT(comm,
                  "options:"
                  "\n  processes:      %d"
                  "\n  backend:        %s"
                  "\n  loop:           %s"
                  "\n  iters:          %zu"
                  "\n  warmup_iters:   %zu"
                  "\n  buf_count:      %zu"
                  "\n  min_elem_count: %zu"
                  "\n  max_elem_count: %zu"
                  "\n  elem_counts:    %s"
                  "\n  check:          %d"
                  "\n  cache:          %d"
    /*"\n  v2i_ratio:      %zu"*/
#ifdef CCL_ENABLE_SYCL
                  "\n  sycl_dev_type:  %s"
                  "\n  sycl_mem_type:  %s"
                  "\n  sycl_usm_type:  %s"
#endif
                  "\n  ranks_per_proc: %zu"
                  "\n  collectives:    %s"
                  "\n  datatypes:      %s"
                  "\n  reductions:     %s"
                  "\n  csv_filepath:   %s",
                  comm.size(),
                  backend_str.c_str(),
                  loop_str.c_str(),
                  options.iters,
                  options.warmup_iters,
                  options.buf_count,
                  options.min_elem_count,
                  options.max_elem_count,
                  elem_counts_str.c_str(),
                  options.check_values,
                  options.cache_ops,
    /*options.v2i_ratio,*/
#ifdef CCL_ENABLE_SYCL
                  sycl_dev_type_str.c_str(),
                  sycl_mem_type_str.c_str(),
                  sycl_usm_type_str.c_str(),
#endif
                  options.ranks_per_proc,
                  collectives_str.c_str(),
                  datatypes_str.c_str(),
                  reductions_str.c_str(),
                  options.csv_filepath.c_str());
}
