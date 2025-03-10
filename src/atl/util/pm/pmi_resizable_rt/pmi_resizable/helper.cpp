#include <string.h>
#include <errno.h>

#include "util/pm/pmi_resizable_rt/pmi_resizable/helper.hpp"
#include "util/pm/pmi_resizable_rt/pmi_resizable/kvs/internal_kvs.h"

int my_rank;
size_t count_pods;
size_t barrier_num = 0;
size_t up_idx;
size_t applied = 0;

std::list<int> killed_ranks;
int killed_ranks_count = 0;

std::list<int> new_ranks;
int new_ranks_count = 0;

kvs_status_t helper::replace_str(char* str, int old_rank, int new_rank) {
    //    throw std::runtime_error("unexpected path");
    LOG_ERROR("unexpected path");
    return KVS_STATUS_FAILURE;

    char old_str[INT_STR_SIZE];
    char new_str[INT_STR_SIZE];
    char* point_to_replace;
    int old_str_size;
    int new_str_size;

    SET_STR(old_str, INT_STR_SIZE, RANK_TEMPLATE, old_rank);
    SET_STR(new_str, INT_STR_SIZE, RANK_TEMPLATE, new_rank);

    point_to_replace = strstr(str, old_str);
    if (point_to_replace == NULL) {
        LOG_ERROR("not found old rank(%d) in str(%s)", old_rank, str);
        return KVS_STATUS_FAILURE;
    }

    old_str_size = strlen(old_str);
    new_str_size = strlen(new_str);

    if (old_str_size != new_str_size) {
        size_t rest_len = strlen(point_to_replace) - old_str_size;
        memmove(point_to_replace + new_str_size, point_to_replace + old_str_size, rest_len);
    }
    memcpy(point_to_replace, new_str, new_str_size);
    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::update_ranks(int* old_count,
                                  std::list<int>& origin_list,
                                  const char* kvs_name) {
    std::vector<std::string> rank_nums(1);
    std::vector<std::string> clear_buf;
    size_t rank_count;
    KVS_CHECK_STATUS(get_keys_values_by_name(kvs_name, clear_buf, rank_nums, rank_count),
                     "failed to get values by name");
    size_t i;
    size_t cur_count = 0;

    if (rank_count == 0) {
        // *old_count = 0;
        return KVS_STATUS_SUCCESS;
    }
    int rank_num;
    for (i = 0; i < rank_count; i++) {
        KVS_CHECK_STATUS(safe_strtol(rank_nums[i].c_str(), rank_num),
                         "failed to to convert rank_num");

        if (std::find(origin_list.begin(), origin_list.end(), rank_num) != origin_list.end())
            continue;

        origin_list.push_back(rank_num);
        cur_count++;
    }

    *old_count += cur_count;
    return KVS_STATUS_SUCCESS;
}

void helper::keep_first_n_up(int prev_new_ranks_count, int prev_killed_ranks_count) {
    killed_ranks.resize(prev_killed_ranks_count);
    new_ranks.resize(prev_new_ranks_count);
}

kvs_status_t helper::get_update_ranks(void) {
    KVS_CHECK_STATUS(update_ranks(&killed_ranks_count, killed_ranks, KVS_APPROVED_DEAD_POD),
                     "failed to update killed ranks");
    KVS_CHECK_STATUS(update_ranks(&new_ranks_count, new_ranks, KVS_APPROVED_NEW_POD),
                     "failed to update new ranks");
    return KVS_STATUS_SUCCESS;
}

void helper::get_shift(std::list<shift_rank_t>& list) {
    int shift_pods_count = 0;
    int max_rank_survivor_pod = count_pods;
    new_ranks.sort();
    killed_ranks.sort();
    auto cur_new = new_ranks.begin();
    auto cur_killed = killed_ranks.begin();

    while (cur_killed != killed_ranks.end()) {
        if (cur_new != new_ranks.end()) {
            list.push_back({ *cur_killed, *cur_killed, CH_T_UPDATE });
            cur_new++;
        }
        else {
            while (std::find(cur_killed,
                             killed_ranks.end(),
                             max_rank_survivor_pod - shift_pods_count - 1) != killed_ranks.end()) {
                max_rank_survivor_pod--;
            }

            if (*cur_killed < max_rank_survivor_pod - shift_pods_count) {
                list.push_back(
                    { max_rank_survivor_pod - shift_pods_count - 1, *cur_killed, CH_T_SHIFT });
                shift_pods_count++;
            }
            else {
                while (cur_killed != killed_ranks.end()) {
                    list.push_back({ *cur_killed, *cur_killed, CH_T_DEAD });
                    cur_killed++;
                }
                break;
            }
        }
        cur_killed++;
    }
    while (cur_new != new_ranks.end()) {
        list.push_back({ *cur_new, *cur_new, CH_T_NEW });
        cur_new++;
    }
}

kvs_status_t helper::up_pods_count(void) {
    KVS_CHECK_STATUS(get_count_names(KVS_POD_NUM, count_pods), "failed to get count names");
    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::wait_accept(void) {
    std::string my_rank_str;

    my_rank = 0;

    while (1) {
        KVS_CHECK_STATUS(get_value_by_name_key(KVS_ACCEPT, pmi_hostname, my_rank_str),
                         "failed to get value");
        if (my_rank_str.empty())
            continue;
        KVS_CHECK_STATUS(safe_strtol(my_rank_str.c_str(), my_rank), "failed to convert my_rank");
        break;
    }
    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::clean_dead_pods_info(std::list<int>& dead_up_idx) {
    size_t i;
    size_t count_death;
    std::vector<std::string> kvs_keys(1);
    std::vector<std::string> clear_buf;
    auto it = dead_up_idx.begin();

    while (it != dead_up_idx.end()) {
        KVS_CHECK_STATUS(
            get_keys_values_by_name(KVS_APPROVED_DEAD_POD, kvs_keys, clear_buf, count_death),
            "failed to get keys and values");

        for (i = 0; i < count_death; i++) {
            KVS_CHECK_STATUS(remove_name_key(KVS_APPROVED_DEAD_POD, kvs_keys[i]),
                             "failed to remove name and key");
            it++;
            if (it == dead_up_idx.end()) {
                break;
            }
        }
    }
    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::accept_new_ranks(const std::list<shift_rank_t>& list) {
    char new_rank_str[INT_STR_SIZE];
    char old_rank_str[INT_STR_SIZE];
    std::vector<std::string> kvs_values(1);
    std::vector<std::string> kvs_keys(1);
    std::vector<std::string> clear_buf;
    size_t count_values;
    size_t i = 0;

    for (const auto& cur_list : list) {
        if (cur_list.type == CH_T_UPDATE || cur_list.type == CH_T_NEW) {
            SET_STR(old_rank_str, INT_STR_SIZE, RANK_TEMPLATE, cur_list.old_rank);
            SET_STR(new_rank_str, INT_STR_SIZE, RANK_TEMPLATE, cur_list.new_rank);

            KVS_CHECK_STATUS(
                get_keys_values_by_name(KVS_APPROVED_NEW_POD, kvs_keys, kvs_values, count_values),
                "failed to get keys and values");

            for (i = 0; i < count_values; i++) {
                if (!strcmp(kvs_values[i].c_str(), old_rank_str)) {
                    KVS_CHECK_STATUS(set_value(KVS_ACCEPT, kvs_keys[i], new_rank_str),
                                     "failed to set value");
                    break;
                }
            }
        }
    }

    do {
        KVS_CHECK_STATUS(get_keys_values_by_name(KVS_ACCEPT, clear_buf, kvs_values, count_values),
                         "failed to get keys and values");
    } while (count_values != 0);

    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::update_kvs_info(int new_rank) {
    char kvs_name[MAX_KVS_NAME_LENGTH];
    char kvs_key[MAX_KVS_KEY_LENGTH];
    char kvs_val[MAX_KVS_VAL_LENGTH];
    size_t kvs_list_size = get_kvs_list_size(ST_CLIENT);

    for (size_t kvs_idx = 0; kvs_idx < kvs_list_size; kvs_idx++) {
        cut_head(kvs_name, kvs_key, kvs_val, ST_CLIENT);

        KVS_CHECK_STATUS(remove_name_key(kvs_name, kvs_key), "failed to remove name and key");

        KVS_CHECK_STATUS(replace_str(kvs_key, my_rank, new_rank), "failed to replace str");

        KVS_CHECK_STATUS(set_value(kvs_name, kvs_key, kvs_val), "failed to set value");

        put_key(kvs_name, kvs_key, kvs_val, ST_CLIENT);
    }
    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::move_to_new_rank(int new_rank) {
    char rank_str[INT_STR_SIZE];

    KVS_CHECK_STATUS(update_kvs_info(new_rank), "failed to update kvs info");
    my_rank = new_rank;

    SET_STR(rank_str, INT_STR_SIZE, RANK_TEMPLATE, new_rank);

    //    request_set_val(KVS_POD_REQUEST, pmi_hostname, rank_str);

    KVS_CHECK_STATUS(set_value(KVS_POD_NUM, rank_str, pmi_hostname), "failed to update kvs info");
    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::update_my_info(const std::list<shift_rank_t>& list) {
    char rank_str[INT_STR_SIZE];

    for (const auto& it : list) {
        if (it.old_rank == static_cast<int>(my_rank)) {
            int old_rank = my_rank;
            KVS_CHECK_STATUS(move_to_new_rank(it.new_rank), "failed to move to new rank");

            SET_STR(rank_str, INT_STR_SIZE, RANK_TEMPLATE, old_rank);

            KVS_CHECK_STATUS(remove_name_key(KVS_POD_NUM, rank_str),
                             "failed to remove name and key");

            break;
        }
    }
    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::get_barrier_idx(size_t& barrier_num_out) {
    std::vector<std::string> kvs_values(1);
    std::vector<std::string> clear_buf;

    size_t count_kvs_values = 0;
    size_t tmp_barrier_num;
    size_t min_barrier_num;
    size_t i = 0;

    KVS_CHECK_STATUS(get_keys_values_by_name(KVS_BARRIER, clear_buf, kvs_values, count_kvs_values),
                     "failed to get keys and values");
    if (count_kvs_values == 0)
        return KVS_STATUS_SUCCESS;

    KVS_CHECK_STATUS(safe_strtol(kvs_values[0].c_str(), min_barrier_num),
                     "failed to convert barrier num");
    for (i = 1; i < count_kvs_values; i++) {
        KVS_CHECK_STATUS(safe_strtol(kvs_values[i].c_str(), tmp_barrier_num),
                         "failed to convert tmp barrier num");
        if (min_barrier_num > tmp_barrier_num)
            min_barrier_num = tmp_barrier_num;
    }

    barrier_num_out = min_barrier_num;
    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::post_my_info(void) {
    char barrier_num_str[INT_STR_SIZE];
    char my_rank_str[INT_STR_SIZE];

    applied = 1;

    SET_STR(my_rank_str, INT_STR_SIZE, RANK_TEMPLATE, my_rank);

    KVS_CHECK_STATUS(set_value(KVS_POD_NUM, my_rank_str, pmi_hostname), "failed to set rank");

    KVS_CHECK_STATUS(get_barrier_idx(barrier_num), "failed to get barrier idx");

    SET_STR(barrier_num_str, INT_STR_SIZE, SIZE_T_TEMPLATE, barrier_num);

    KVS_CHECK_STATUS(set_value(KVS_BARRIER, pmi_hostname, barrier_num_str),
                     "failed to set barrier idx");

    KVS_CHECK_STATUS(remove_name_key(KVS_ACCEPT, pmi_hostname),
                     "failed to remove accepted hostname");

    KVS_CHECK_STATUS(remove_name_key(KVS_APPROVED_NEW_POD, pmi_hostname),
                     "failed to remove approved hostname");

    barrier_num++;
    if (barrier_num > BARRIER_NUM_MAX)
        barrier_num = 0;
    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::update(const std::list<shift_rank_t>& list,
                            std::list<int>& dead_up_idx,
                            int root_rank) {
    if (applied == 1) {
        if (!list.empty()) {
            if (static_cast<int>(my_rank) == root_rank) {
                if (!dead_up_idx.empty()) {
                    KVS_CHECK_STATUS(clean_dead_pods_info(dead_up_idx), "failed to clean dead pod");
                }
                KVS_CHECK_STATUS(accept_new_ranks(list), "failed to accept new ranks");
            }
            KVS_CHECK_STATUS(update_my_info(list), "failed to update info");
        }
    }
    else {
        KVS_CHECK_STATUS(post_my_info(), "failed to post info");
    }
    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::get_val_count(const char* name, const char* val, size_t& res) {
    res = 0;
    std::vector<std::string> kvs_values(1);
    std::vector<std::string> clear_buf;
    size_t count_values;
    size_t i;

    KVS_CHECK_STATUS(get_keys_values_by_name(name, clear_buf, kvs_values, count_values),
                     "failed to get keys and values");

    if (count_values != 0) {
        for (i = 0; i < count_values; i++) {
            if (val == kvs_values[i]) {
                res++;
            }
        }
    }

    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::get_occupied_ranks_count(char* rank, size_t& res) {
    std::string occupied_rank_val_str;
    size_t is_occupied_rank;
    size_t count_new_pod = 0;
    size_t count_seen_new_pod = 0;

    KVS_CHECK_STATUS(get_value_by_name_key(KVS_POD_NUM, rank, occupied_rank_val_str),
                     "failed to get occupied rank");

    is_occupied_rank = (occupied_rank_val_str.empty()) ? 0 : 1;

    KVS_CHECK_STATUS(get_val_count(KVS_NEW_POD, rank, count_new_pod), "failed to get mew rank");

    KVS_CHECK_STATUS(get_val_count(KVS_APPROVED_NEW_POD, rank, count_seen_new_pod),
                     "failed to get new approved rank");

    res = is_occupied_rank + count_new_pod + count_seen_new_pod;
    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::get_count_requested_ranks(char* rank, size_t& count_pods_with_my_rank) {
    count_pods_with_my_rank = 0;

    KVS_CHECK_STATUS(get_val_count(KVS_POD_REQUEST, rank, count_pods_with_my_rank),
                     "failed tp get requested ranks");

    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::occupied_rank(char* rank) {
    std::string idx_val;

    KVS_CHECK_STATUS(get_value_by_name_key(KVS_UP, KVS_IDX, idx_val), "failed to get ID");

    if ((idx_val.empty()) && (my_rank == 0)) {
        KVS_CHECK_STATUS(set_value(KVS_UP, KVS_IDX, INITIAL_UPDATE_IDX),
                         "failed to set initial ID");

        count_pods = 1;

        std::list<int> clear_list{};
        std::list<shift_rank_t> clear_shift_list{};
        KVS_CHECK_STATUS(update(clear_shift_list, clear_list, 0), "failed to initial update");
    }
    else {
        KVS_CHECK_STATUS(set_value(KVS_NEW_POD, pmi_hostname, rank), "failed to set rank");
    }
    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::reg_rank(void) {
    char rank_str[INT_STR_SIZE];
    size_t wait_shift = 0;
    std::vector<std::string> kvs_values(1);
    std::vector<std::string> kvs_keys(1);
    size_t count_values = 0;
    size_t my_num_in_pod_request_line = 0;
    size_t i;

    my_rank = 0;
    KVS_CHECK_STATUS(set_value(KVS_POD_REQUEST, pmi_hostname, INITIAL_RANK_NUM),
                     "failed to set initial rank");

    SET_STR(rank_str, INT_STR_SIZE, RANK_TEMPLATE, my_rank);

    while (1) {
        wait_shift = 0;
        my_num_in_pod_request_line = 0;

        KVS_CHECK_STATUS(
            get_keys_values_by_name(KVS_POD_REQUEST, kvs_keys, kvs_values, count_values),
            "failed to get requested pods");

        for (i = 0; i < count_values; i++) {
            if (!strcmp(kvs_values[i].c_str(), rank_str)) {
                my_num_in_pod_request_line++;
                if (!strcmp(kvs_keys[i].c_str(), pmi_hostname))
                    break;
            }
        }

        if (my_num_in_pod_request_line == 1) {
            size_t rank_count;
            KVS_CHECK_STATUS(get_occupied_ranks_count(rank_str, rank_count),
                             "failed to get occupied ranks count");
            if (rank_count != 0) {
                wait_shift = 0;
            }
            else {
                wait_shift = 1;
                KVS_CHECK_STATUS(get_count_requested_ranks(rank_str, rank_count),
                                 "failed to get requested ranks count");
                if (rank_count == 1) {
                    KVS_CHECK_STATUS(occupied_rank(rank_str), "failed to get occupied rank");
                    break;
                }
            }
        }

        if (!wait_shift) {
            my_rank++;
            SET_STR(rank_str, INT_STR_SIZE, RANK_TEMPLATE, my_rank);
            KVS_CHECK_STATUS(set_value(KVS_POD_REQUEST, pmi_hostname, rank_str),
                             "failed to set rank");
        }
    }

    KVS_CHECK_STATUS(remove_name_key(KVS_POD_REQUEST, pmi_hostname), "failed to remove host info");

    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::get_replica_size(size_t& replica_size) {
    return kvs->kvs_get_replica_size(replica_size);
}

kvs_status_t helper::up_kvs(const char* new_kvs_name, const char* old_kvs_name) {
    std::vector<std::string> kvs_values(1);
    std::vector<std::string> kvs_keys(1);
    size_t i = 0;
    size_t count_values;

    KVS_CHECK_STATUS(get_keys_values_by_name(old_kvs_name, kvs_keys, kvs_values, count_values),
                     "failed to get keys and values");
    for (i = 0; i < count_values; i++) {
        KVS_CHECK_STATUS(remove_name_key(old_kvs_name, kvs_keys[i]),
                         "failed to remove old kvs info");

        KVS_CHECK_STATUS(set_value(new_kvs_name, kvs_keys[i], kvs_values[i]),
                         "failed to set new kvs info");
    }
    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::up_kvs_new_and_dead(void) {
    KVS_CHECK_STATUS(up_kvs(KVS_APPROVED_NEW_POD, KVS_NEW_POD), "failed to update new");
    KVS_CHECK_STATUS(up_kvs(KVS_APPROVED_DEAD_POD, KVS_DEAD_POD), "failed to update dead");
    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::get_new_root(int* old_root) {
    size_t i;
    std::vector<std::string> rank_nums(1);
    std::vector<std::string> clear_buf;
    size_t rank_count;
    int rank_num;
    KVS_CHECK_STATUS(get_keys_values_by_name(KVS_DEAD_POD, clear_buf, rank_nums, rank_count),
                     "failed to update new");

    for (i = 0; i < rank_count; i++) {
        KVS_CHECK_STATUS(safe_strtol(rank_nums[i].c_str(), rank_num), "failed to update new");
        if (*old_root == rank_num) {
            (*old_root)++;
        }
    }
    return KVS_STATUS_SUCCESS;
}

kvs_status_t helper::get_keys_values_by_name(const std::string& kvs_name,
                                             std::vector<std::string>& kvs_keys,
                                             std::vector<std::string>& kvs_values,
                                             size_t& count) {
    return kvs->kvs_get_keys_values_by_name(kvs_name, kvs_keys, kvs_values, count);
}
kvs_status_t helper::set_value(const std::string& kvs_name,
                               const std::string& kvs_key,
                               const std::string& kvs_val) {
    return kvs->kvs_set_value(kvs_name, kvs_key, kvs_val);
}
kvs_status_t helper::remove_name_key(const std::string& kvs_name, const std::string& kvs_key) {
    return kvs->kvs_remove_name_key(kvs_name, kvs_key);
}
kvs_status_t helper::get_value_by_name_key(const std::string& kvs_name,
                                           const std::string& kvs_key,
                                           std::string& kvs_val) {
    return kvs->kvs_get_value_by_name_key(kvs_name, kvs_key, kvs_val);
}
size_t helper::init(const char* main_addr) {
    return kvs->kvs_init(main_addr);
}
kvs_status_t helper::main_server_address_reserve(char* main_addr) {
    return kvs->kvs_main_server_address_reserve(main_addr);
}
kvs_status_t helper::get_count_names(const std::string& kvs_name, size_t& count_names) {
    return kvs->kvs_get_count_names(kvs_name, count_names);
}
kvs_status_t helper::finalize(void) {
    return kvs->kvs_finalize();
}
