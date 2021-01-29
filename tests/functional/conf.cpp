#include "conf.hpp"
#include "lp.hpp"

std::vector<ccl_test_conf> test_params;

ccl_place_type first_ccl_place_type = PT_IN;
#ifdef TEST_CCL_BCAST
ccl_place_type last_ccl_place_type = static_cast<ccl_place_type>(first_ccl_place_type + 1);
#else
ccl_place_type last_ccl_place_type = PT_LAST;
#endif
std::map<int, const char*> ccl_place_type_str = { { PT_IN, "PT_IN" }, { PT_OOP, "PT_OOP" } };

ccl_size_type first_ccl_size_type = ST_SMALL;
ccl_size_type last_ccl_size_type = ST_LAST;
std::map<int, const char*> ccl_size_type_str = { { ST_SMALL, "ST_SMALL" },
                                                 { ST_MEDIUM, "ST_MEDIUM" },
                                                 { ST_LARGE, "ST_LARGE" } };
std::map<int, size_t> ccl_size_type_values = { { ST_SMALL, 17 },
                                               { ST_MEDIUM, 32769 },
                                               { ST_LARGE, 262144 } };

ccl_buffer_count first_ccl_buffer_count = BC_SMALL;
ccl_buffer_count last_ccl_buffer_count = BC_LAST;
std::map<int, const char*> ccl_buffer_count_str = { { BC_SMALL, "BC_SMALL" },
                                                    { BC_LARGE, "BC_LARGE" } };
std::map<int, size_t> ccl_buffer_count_values = { { BC_SMALL, 1 }, { BC_LARGE, 3 } };

ccl_completion_type first_ccl_completion_type = CMPT_WAIT;
ccl_completion_type last_ccl_completion_type = CMPT_LAST;
std::map<int, const char*> ccl_completion_type_str = { { CMPT_WAIT, "CMPT_WAIT" },
                                                       { CMPT_TEST, "CMPT_TEST" } };

ccl_prolog_type first_ccl_prolog_type = PTYPE_NULL;
ccl_prolog_type last_ccl_prolog_type = PTYPE_LAST;
std::map<int, const char*> ccl_prolog_type_str = { { PTYPE_NULL, "PTYPE_NULL" },
#ifdef TEST_CCL_CUSTOM_PROLOG
                                                   { PTYPE_T_TO_2X, "PTYPE_T_TO_2X" },
                                                   { PTYPE_T_TO_CHAR, "PTYPE_T_TO_CHAR" }
#endif
};

ccl_epilog_type first_ccl_epilog_type = ETYPE_NULL;
ccl_epilog_type last_ccl_epilog_type = ETYPE_LAST;
std::map<int, const char*> ccl_epilog_type_str = { { ETYPE_NULL, "ETYPE_NULL" },
#ifdef TEST_CCL_CUSTOM_EPILOG
                                                   { ETYPE_T_TO_2X, "ETYPE_T_TO_2X" },
                                                   { ETYPE_CHAR_TO_T, "ETYPE_CHAR_TO_T" }
#endif
};

ccl_data_type first_ccl_data_type = DT_INT8;
ccl_data_type last_ccl_data_type = DT_LAST;
std::map<int, const char*> ccl_data_type_str = {
    { DT_INT8, "DT_INT8" },       { DT_UINT8, "DT_UINT8" },     { DT_INT16, "DT_INT16" },
    { DT_UINT16, "DT_UINT16" },   { DT_INT32, "DT_INT32" },     { DT_UINT32, "DT_UINT32" },
    { DT_INT64, "DT_INT64" },     { DT_UINT64, "DT_UINT64" },   { DT_FLOAT16, "DT_FLOAT16" },
    { DT_FLOAT32, "DT_FLOAT32" }, { DT_FLOAT64, "DT_FLOAT64" }, { DT_BFLOAT16, "DT_BFLOAT16" },
};
std::map<int, ccl::datatype> ccl_datatype_values = {
    { DT_INT8, ccl::datatype::int8 },       { DT_UINT8, ccl::datatype::uint8 },
    { DT_INT16, ccl::datatype::int16 },     { DT_UINT16, ccl::datatype::uint16 },
    { DT_INT32, ccl::datatype::int32 },     { DT_UINT32, ccl::datatype::uint32 },
    { DT_INT64, ccl::datatype::int64 },     { DT_UINT64, ccl::datatype::uint64 },
    { DT_FLOAT16, ccl::datatype::float16 }, { DT_FLOAT32, ccl::datatype::float32 },
    { DT_FLOAT64, ccl::datatype::float64 }, { DT_BFLOAT16, ccl::datatype::bfloat16 },
};

ccl_reduction_type first_ccl_reduction_type = RT_SUM;
#ifdef TEST_CCL_REDUCE
ccl_reduction_type last_ccl_reduction_type = RT_LAST;
#else
ccl_reduction_type last_ccl_reduction_type =
    static_cast<ccl_reduction_type>(first_ccl_reduction_type + 1);
#endif
std::map<int, const char*> ccl_reduction_type_str = {
    { RT_SUM, "RT_SUM" },       { RT_PROD, "RT_PROD" },
    { RT_MIN, "RT_MIN" },       { RT_MAX, "RT_MAX" },
#ifdef TEST_CCL_CUSTOM_REDUCE
    { RT_CUSTOM, "RT_CUSTOM" }, { RT_CUSTOM_NULL, "RT_CUSTOM_NULL" }
#endif
};
std::map<int, ccl::reduction> ccl_reduction_values = {
    { RT_SUM, ccl::reduction::sum },       { RT_PROD, ccl::reduction::prod },
    { RT_MIN, ccl::reduction::min },       { RT_MAX, ccl::reduction::max },
#ifdef TEST_CCL_CUSTOM_REDUCE
    { RT_CUSTOM, ccl::reduction::custom }, { RT_CUSTOM_NULL, ccl::reduction::custom }
#endif
};

ccl_cache_type first_ccl_cache_type = CT_CACHE_0;
ccl_cache_type last_ccl_cache_type = CT_LAST;
std::map<int, const char*> ccl_cache_type_str = { { CT_CACHE_0, "CT_CACHE_0" },
                                                  { CT_CACHE_1, "CT_CACHE_1" } };
std::map<int, int> ccl_cache_type_values = { { CT_CACHE_0, 0 }, { CT_CACHE_1, 1 } };

ccl_sync_type first_ccl_sync_type = SNCT_SYNC_0;
ccl_sync_type last_ccl_sync_type = SNCT_LAST;
std::map<int, const char*> ccl_sync_type_str = { { SNCT_SYNC_0, "SNCT_SYNC_0" },
                                                 { SNCT_SYNC_1, "SNCT_SYNC_1" } };
std::map<int, int> ccl_sync_type_values = { { SNCT_SYNC_0, 0 }, { SNCT_SYNC_1, 1 } };

ccl_order_type first_ccl_order_type = ORDER_DISABLE;
ccl_order_type last_ccl_order_type = ORDER_LAST;
std::map<int, const char*> ccl_order_type_str = { { ORDER_DISABLE, "ORDER_DISABLE" },
                                                  { ORDER_DIRECT, "ORDER_DIRECT" },
                                                  { ORDER_INDIRECT, "ORDER_INDIRECT" },
                                                  { ORDER_RANDOM, "ORDER_RANDOM" } };
std::map<int, int> ccl_order_type_values = { { ORDER_DISABLE, 0 },
                                             { ORDER_DIRECT, 1 },
                                             { ORDER_INDIRECT, 2 },
                                             { ORDER_RANDOM, 3 } };

POST_AND_PRE_INCREMENTS(ccl_place_type, PT_LAST);
POST_AND_PRE_INCREMENTS(ccl_size_type, ST_LAST);
POST_AND_PRE_INCREMENTS(ccl_completion_type, CMPT_LAST);
POST_AND_PRE_INCREMENTS(ccl_data_type, DT_LAST);
POST_AND_PRE_INCREMENTS(ccl_reduction_type, RT_LAST);
POST_AND_PRE_INCREMENTS(ccl_cache_type, CT_LAST);
POST_AND_PRE_INCREMENTS(ccl_sync_type, SNCT_LAST);
POST_AND_PRE_INCREMENTS(ccl_order_type, ORDER_LAST);
POST_AND_PRE_INCREMENTS(ccl_buffer_count, BC_LAST);
POST_AND_PRE_INCREMENTS(ccl_prolog_type, PTYPE_LAST);
POST_AND_PRE_INCREMENTS(ccl_epilog_type, ETYPE_LAST);

size_t get_ccl_elem_count(ccl_test_conf& test_conf) {
    return ccl_size_type_values[test_conf.size_type];
}

size_t get_ccl_buffer_count(ccl_test_conf& test_conf) {
    return ccl_buffer_count_values[test_conf.buffer_count];
}

ccl::datatype get_ccl_lib_datatype(const ccl_test_conf& test_conf) {
    return ccl_datatype_values[test_conf.datatype];
}

ccl::reduction get_ccl_lib_reduction(const ccl_test_conf& test_conf) {
    return ccl_reduction_values[test_conf.reduction];
}

bool should_skip_datatype(ccl_data_type dt) {
    if (dt == DT_BFLOAT16 && !is_bf16_enabled())
        return true;

    if (dt == DT_FLOAT16 && !is_fp16_enabled())
        return true;

    if (dt == DT_INT8 || dt == DT_UINT8 || dt == DT_INT16 || dt == DT_UINT16 || dt == DT_UINT32 ||
        dt == DT_INT64 || dt == DT_UINT64 || dt == DT_FLOAT64)
        return true;

    return false;
}

size_t calculate_test_count() {
    size_t test_count = max_test_count();

    // CCL_TEST_EPILOG_TYPE=0 CCL_TEST_PROLOG_TYPE=0 CCL_TEST_PLACE_TYPE=0 CCL_TEST_CACHE_TYPE=0 CCL_TEST_BUFFER_COUNT=0 CCL_TEST_SIZE_TYPE=0 CCL_TEST_PRIORITY_TYPE=1 CCL_TEST_COMPLETION_TYPE=0 CCL_TEST_SYNC_TYPE=0 CCL_TEST_REDUCTION_TYPE=0 CCL_TEST_DATA_TYPE=0
    char* test_data_type_enabled = getenv("CCL_TEST_DATA_TYPE");
    char* test_reduction_enabled = getenv("CCL_TEST_REDUCTION_TYPE");
    char* test_sync_enabled = getenv("CCL_TEST_SYNC_TYPE");
    char* test_completion_enabled = getenv("CCL_TEST_COMPLETION_TYPE");
    char* test_order_type_enabled = getenv("CCL_TEST_PRIORITY_TYPE");
    char* test_size_type_enabled = getenv("CCL_TEST_SIZE_TYPE");
    char* test_buffer_count_enabled = getenv("CCL_TEST_BUFFER_COUNT");
    char* test_cache_type_enabled = getenv("CCL_TEST_CACHE_TYPE");
    char* test_place_type_enabled = getenv("CCL_TEST_PLACE_TYPE");
    char* test_prolog_enabled = getenv("CCL_TEST_PROLOG_TYPE");
    char* test_epilog_enabled = getenv("CCL_TEST_EPILOG_TYPE");

    if (test_data_type_enabled && atoi(test_data_type_enabled) == 0) {
        test_count /= last_ccl_data_type;
        first_ccl_data_type = static_cast<ccl_data_type>(DT_FLOAT32);
        last_ccl_data_type = static_cast<ccl_data_type>(first_ccl_data_type + 1);
    }

    if (test_reduction_enabled && atoi(test_reduction_enabled) == 0) {
        test_count /= last_ccl_reduction_type;
        first_ccl_reduction_type = static_cast<ccl_reduction_type>(RT_SUM);
        last_ccl_reduction_type = static_cast<ccl_reduction_type>(first_ccl_reduction_type + 1);
    }

    if (test_sync_enabled && atoi(test_sync_enabled) == 0) {
        test_count /= last_ccl_sync_type;
        first_ccl_sync_type = static_cast<ccl_sync_type>(SNCT_SYNC_1);
        last_ccl_sync_type = static_cast<ccl_sync_type>(first_ccl_sync_type + 1);
    }

    if (test_completion_enabled && atoi(test_completion_enabled) == 0) {
        test_count /= last_ccl_completion_type;
        first_ccl_completion_type = static_cast<ccl_completion_type>(CMPT_WAIT);
        last_ccl_completion_type = static_cast<ccl_completion_type>(first_ccl_completion_type + 1);
    }

    if (test_order_type_enabled && atoi(test_order_type_enabled) == 0) {
        test_count /= (last_ccl_order_type * last_ccl_order_type);
        first_ccl_order_type = static_cast<ccl_order_type>(ORDER_DISABLE);
        last_ccl_order_type = static_cast<ccl_order_type>(first_ccl_order_type + 1);
    }

    if (test_size_type_enabled && atoi(test_size_type_enabled) == 0) {
        test_count /= last_ccl_size_type;
        first_ccl_size_type = static_cast<ccl_size_type>(ST_MEDIUM);
        last_ccl_size_type = static_cast<ccl_size_type>(first_ccl_size_type + 1);
    }

    if (test_buffer_count_enabled && atoi(test_buffer_count_enabled) == 0) {
        test_count /= last_ccl_buffer_count;
        first_ccl_buffer_count = static_cast<ccl_buffer_count>(BC_LARGE);
        last_ccl_buffer_count = static_cast<ccl_buffer_count>(first_ccl_buffer_count + 1);
    }

    if (test_cache_type_enabled && atoi(test_cache_type_enabled) == 0) {
        test_count /= last_ccl_cache_type;
        first_ccl_cache_type = static_cast<ccl_cache_type>(CT_CACHE_1);
        last_ccl_cache_type = static_cast<ccl_cache_type>(first_ccl_cache_type + 1);
    }

    if (test_place_type_enabled && atoi(test_place_type_enabled) == 0) {
        test_count /= last_ccl_place_type;
        first_ccl_place_type = static_cast<ccl_place_type>(PT_IN);
        last_ccl_place_type = static_cast<ccl_place_type>(first_ccl_place_type + 1);
    }

    if (test_prolog_enabled && atoi(test_prolog_enabled) == 0) {
        test_count /= last_ccl_prolog_type;
        first_ccl_prolog_type = static_cast<ccl_prolog_type>(PTYPE_NULL);
        last_ccl_prolog_type = static_cast<ccl_prolog_type>(first_ccl_prolog_type + 1);
    }

    if (test_epilog_enabled && atoi(test_epilog_enabled) == 0) {
        test_count /= last_ccl_epilog_type;
        first_ccl_epilog_type = static_cast<ccl_epilog_type>(ETYPE_NULL);
        last_ccl_epilog_type = static_cast<ccl_epilog_type>(first_ccl_epilog_type + 1);
    }

    return test_count;
}

void init_test_params() {
    test_params.resize(calculate_test_count());

    size_t idx = 0;
    for (ccl_prolog_type prolog_type = first_ccl_prolog_type; prolog_type < last_ccl_prolog_type;
         prolog_type++) {
        for (ccl_epilog_type epilog_type = first_ccl_epilog_type;
             epilog_type < last_ccl_epilog_type;
             epilog_type++) {
            // if ((epilog_type != ETYPE_CHAR_TO_T && prolog_type == PTYPE_T_TO_CHAR)||(epilog_type == ETYPE_CHAR_TO_T && prolog_type != PTYPE_T_TO_CHAR))
            //      // TODO: remove skipped data type
            //      continue;
            for (ccl_reduction_type reduction_type = first_ccl_reduction_type;
                 reduction_type < last_ccl_reduction_type;
                 reduction_type++) {
                for (ccl_sync_type sync_type = first_ccl_sync_type; sync_type < last_ccl_sync_type;
                     sync_type++) {
                    for (ccl_cache_type cache_type = first_ccl_cache_type;
                         cache_type < last_ccl_cache_type;
                         cache_type++) {
                        for (ccl_size_type size_type = first_ccl_size_type;
                             size_type < last_ccl_size_type;
                             size_type++) {
                            for (ccl_data_type data_type = first_ccl_data_type;
                                 data_type < last_ccl_data_type;
                                 data_type++) {
                                if (should_skip_datatype(data_type))
                                    continue;

                                for (ccl_completion_type completion_type =
                                         first_ccl_completion_type;
                                     completion_type < last_ccl_completion_type;
                                     completion_type++) {
                                    for (ccl_place_type place_type = first_ccl_place_type;
                                         place_type < last_ccl_place_type;
                                         place_type++) {
#ifdef TEST_CCL_BCAST
                                        if (place_type == PT_OOP)
                                            continue;
#endif
                                        for (ccl_order_type start_order_type = first_ccl_order_type;
                                             start_order_type < last_ccl_order_type;
                                             start_order_type++) {
                                            for (ccl_order_type complete_order_type =
                                                     first_ccl_order_type;
                                                 complete_order_type < last_ccl_order_type;
                                                 complete_order_type++) {
                                                for (ccl_buffer_count buffer_count =
                                                         first_ccl_buffer_count;
                                                     buffer_count < last_ccl_buffer_count;
                                                     buffer_count++) {
                                                    test_params[idx].place_type = place_type;
                                                    test_params[idx].size_type = size_type;
                                                    test_params[idx].datatype = data_type;
                                                    test_params[idx].cache_type = cache_type;
                                                    test_params[idx].sync_type = sync_type;
                                                    test_params[idx].completion_type =
                                                        completion_type;
                                                    test_params[idx].reduction = reduction_type;
                                                    test_params[idx].buffer_count = buffer_count;
                                                    test_params[idx].start_order_type =
                                                        start_order_type;
                                                    test_params[idx].complete_order_type =
                                                        complete_order_type;
                                                    test_params[idx].prolog_type = prolog_type;
                                                    test_params[idx].epilog_type = epilog_type;
                                                    idx++;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    test_params.resize(idx);
}

std::ostream& operator<<(std::ostream& stream, ccl_test_conf const& test_conf) {
    std::stringstream sstream;
    sstream << "["
            << " " << ccl_data_type_str[test_conf.datatype] << " "
            << ccl_place_type_str[test_conf.place_type] << " "
            << ccl_cache_type_str[test_conf.cache_type] << " "
            << ccl_size_type_str[test_conf.size_type] << " "
            << ccl_sync_type_str[test_conf.sync_type] << " "
            << ccl_reduction_type_str[test_conf.reduction] << " "
            << ccl_order_type_str[test_conf.start_order_type] << " "
            << ccl_order_type_str[test_conf.complete_order_type] << " "
            << ccl_completion_type_str[test_conf.completion_type] << " "
            << ccl_buffer_count_str[test_conf.buffer_count]
            // << " " << ccl_prolog_type_str[test_conf.prolog_type]
            // << " " << ccl_epilog_type_str[test_conf.epilog_type]
            << " ]";
    return stream << sstream.str();
}

void print_err_message(char* message, std::ostream& output) {
    ccl::communicator& comm = GlobalData::instance().comms[0];
    int comm_size = comm.size();
    int comm_rank = comm.rank();

    size_t message_len = strlen(message);
    std::vector<size_t> message_lens(comm_size, 0);
    std::vector<size_t> recv_counts(comm_size, 1);
    ccl::allgatherv(&message_len, 1, message_lens.data(), recv_counts, comm).wait();

    auto total_message_len = std::accumulate(message_lens.begin(), message_lens.end(), 0);

    if (total_message_len == 0) {
        return;
    }

    std::vector<char> messages(total_message_len);
    ccl::allgatherv(message, message_len, messages.data(), message_lens, ccl::datatype::int8, comm)
        .wait();

    if (comm_rank == 0) {
        output << messages.data();
    }
}
