#include "helper.h"
#include <assert.h>

#define MAX_KVS_STR_LENGTH 1024
#define SHIFT_TO_VAL 3

char run_v2_template[RUN_TEMPLATE_SIZE];

size_t my_rank, count_pods;
size_t barrier_num = 0;
size_t up_idx;
size_t initialized = 0;

int_list_t* killed_ranks = NULL;
size_t killed_ranks_count = 0;

int_list_t* new_ranks = NULL;
size_t new_ranks_count = 0;

void replace_str(char* str, size_t old, size_t new)
{
    char old_str[INT_STR_SIZE];
    char new_str[INT_STR_SIZE];
    char* point_to_replace;
    size_t old_str_size;
    size_t new_str_size;

    SET_STR(old_str, INT_STR_SIZE, SIZE_T_TEMPLATE, old);

    SET_STR(new_str, INT_STR_SIZE, SIZE_T_TEMPLATE, new);

    point_to_replace = strstr(str, old_str);
    old_str_size     = strlen(old_str);
    new_str_size     = strlen(new_str);

    if(old_str_size != new_str_size)
    {
        size_t rest_len = strlen(point_to_replace);
        memmove(point_to_replace + new_str_size, point_to_replace + old_str_size, rest_len);
    }
    strncpy(point_to_replace, new_str, new_str_size);
}

void update_ranks(size_t* old_count, int_list_t** origin_list, const char* kvs_name)
{
    FILE *fp;
    char get_val[REQUEST_POSTFIX_SIZE];
    char run_str[RUN_REQUEST_SIZE];
    char grep_name_str[REQUEST_POSTFIX_SIZE];
    char rank_num[INT_STR_SIZE];
    size_t cur_count = *old_count;

    SET_STR(grep_name_str, REQUEST_POSTFIX_SIZE, GREP_TEMPLATE, kvs_name);

    SET_STR(get_val, REQUEST_POSTFIX_SIZE, CONCAT_TWO_COMMAND_TEMPLATE, grep_name_str, GET_VAL);

    /*get dead/new rank*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, get_val);

    fp = popen(run_str, READ_ONLY);
    while (fgets(rank_num, sizeof(rank_num)-1, fp) != NULL)
    {
        if (int_list_is_contained(*origin_list, atoi(rank_num)))
            continue;

        int_list_add(origin_list, atoi(rank_num));
        cur_count++;
    }
    pclose(fp);
    *old_count = cur_count;
}

void keep_first_n_up(size_t prev_new_ranks_count, size_t prev_killed_ranks_count)
{
    int_list_keep_first_n(&killed_ranks, prev_killed_ranks_count);
    int_list_keep_first_n(&new_ranks, prev_new_ranks_count);
}

void get_update_ranks(size_t id)
{
    char kvs_name[MAX_KVS_NAME_LENGTH];
    SET_STR(kvs_name, MAX_KVS_NAME_LENGTH, KVS_NAME_TEMPLATE_I, KVS_NEW_POD, id);

    update_ranks(&killed_ranks_count, &killed_ranks, KVS_DEAD_POD);
    update_ranks(&new_ranks_count, &new_ranks, kvs_name);
}

void get_shift(shift_list_t** list)
{
    size_t shift_pods_count = 0;
    size_t max_rank_survivor_pod = count_pods;
    int_list_t* cur_new = new_ranks;
    int_list_t* cur_killed = killed_ranks;

    if (killed_ranks != NULL)
        int_list_sort(killed_ranks);
    if (new_ranks != NULL)
        int_list_sort(new_ranks);

    while (cur_killed != NULL)
    {
        if (cur_new != NULL)
        {
            shift_list_add(list, cur_killed->i, cur_killed->i, CH_T_UPDATE);
            cur_new = cur_new->next;
        }
        else
        {
            while (int_list_is_contained(cur_killed, max_rank_survivor_pod - shift_pods_count - 1) == 1)
                max_rank_survivor_pod--;

            if (cur_killed->i < max_rank_survivor_pod - shift_pods_count)
            {
                    shift_list_add(list, max_rank_survivor_pod - shift_pods_count - 1, cur_killed->i, CH_T_SHIFT);
                    shift_pods_count++;
            }
            else
            {
                while (cur_killed != NULL)
                {
                    shift_list_add(list, cur_killed->i, cur_killed->i, CH_T_DEAD);
                    cur_killed = cur_killed->next;
                }
                break;
            }
        }
        cur_killed = cur_killed->next;
    }
    while (cur_new != NULL)
    {
        shift_list_add(list, cur_new->i, cur_new->i, CH_T_NEW);
        cur_new = cur_new->next;
    }
}

void up_pods_count(void)
{
    FILE *fp;
    char run_str[RUN_REQUEST_SIZE];
    char get_count_str[REQUEST_POSTFIX_SIZE];
    char count_pods_str[INT_STR_SIZE];

    SET_STR(get_count_str, REQUEST_POSTFIX_SIZE, GREP_COUNT_TEMPLATE, KVS_POD_NUM);

    /*get count accepted pods (like comm_world size)*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, get_count_str);

    fp = popen(run_str, READ_ONLY);
    if (fgets(count_pods_str, sizeof(count_pods_str) - 1, fp) == NULL) {
        assert(0);
    }
    pclose(fp);

    count_pods = atoi(count_pods_str);
}

void wait_accept()
{
    FILE *fp;
    char run_str[RUN_REQUEST_SIZE];
    char check_accept[INT_STR_SIZE];
    char check_accept_str[REQUEST_POSTFIX_SIZE];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];
    char kvs_key[MAX_KVS_KEY_LENGTH];

    send_notification(0);

    SET_STR(kvs_key, MAX_KVS_KEY_LENGTH, "%s\\\"", KVS_HOSTNAME);
    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_ACCEPT, kvs_key);
    SET_STR(check_accept_str, REQUEST_POSTFIX_SIZE, GREP_COUNT_TEMPLATE, kvs_name_key);

    /*check: is me accepted*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, check_accept_str);

    my_rank = 0;

    while (1)
    {
        fp = popen(run_str, READ_ONLY);
        if (fgets(check_accept, INT_STR_SIZE, fp) == NULL) {
            assert(0);
        }
        pclose(fp);
        if (atoi(check_accept) == 1)
        {
            char get_accept_str[REQUEST_POSTFIX_SIZE];
            char get_my_rank_str[REQUEST_POSTFIX_SIZE];
            char my_rank_str[INT_STR_SIZE];

            SET_STR(get_accept_str, REQUEST_POSTFIX_SIZE, GREP_TEMPLATE, kvs_name_key);
            SET_STR(get_my_rank_str, REQUEST_POSTFIX_SIZE, CONCAT_TWO_COMMAND_TEMPLATE, get_accept_str, GET_VAL);

            /*get my rank*/
            SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, get_my_rank_str);

            fp = popen(run_str, READ_ONLY);
            if (fgets(my_rank_str, INT_STR_SIZE, fp) == NULL) {
                assert(0);
            }
            pclose(fp);

            my_rank = atoi(my_rank_str);
            break;
        }
    }
}

void clean_dead_pods_info(int_list_t* dead_up_idx)
{
    FILE* fp;
    FILE* fp_null;
    char dead_name_key[MAX_KVS_NAME_KEY_LENGTH];
    char grep_dead[REQUEST_POSTFIX_SIZE];
    char get_dead_name_key[REQUEST_POSTFIX_SIZE];
    char run_str[RUN_REQUEST_SIZE];
    char patch[REQUEST_POSTFIX_SIZE];

    while(dead_up_idx != NULL)
    {
        SET_STR(grep_dead, REQUEST_POSTFIX_SIZE, GREP_TEMPLATE, KVS_DEAD_POD);
        SET_STR(get_dead_name_key, REQUEST_POSTFIX_SIZE, CONCAT_TWO_COMMAND_TEMPLATE, grep_dead, GET_NAME_KEY);

        /*get all "dead" records for cleaning*/
        SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, get_dead_name_key);

        fp = popen(run_str, READ_ONLY);
        while (fgets(dead_name_key, sizeof(dead_name_key)-1, fp) != NULL)
        {
            dead_name_key[strlen(dead_name_key) - 1] = NULL_CHAR;
            SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_NULL_TEMPLATE, dead_name_key);

            /*Cleaning dead record*/
            SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

            fp_null = popen(run_str, READ_ONLY);
            pclose(fp_null);
        }
        pclose(fp);
        dead_up_idx = dead_up_idx->next;
    }
}

void accept_new_ranks(shift_list_t* cur_list)
{
    FILE* fp;
    char run_str[RUN_REQUEST_SIZE];
    char patch[REQUEST_POSTFIX_SIZE];
    char grep_name[REQUEST_POSTFIX_SIZE];
    char grep_rank[REQUEST_POSTFIX_SIZE];
    char grep_count_accept[REQUEST_POSTFIX_SIZE];
    char get_new_pod_name_str[REQUEST_POSTFIX_SIZE];
    char is_accept[INT_STR_SIZE];
    char kvs_name[MAX_KVS_NAME_LENGTH];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];
    char new_pod_name_str[MAX_KVS_KEY_LENGTH];

    SET_STR(kvs_name, MAX_KVS_NAME_LENGTH, KVS_NAME_TEMPLATE_S, KVS_NEW_POD, "");
    SET_STR(grep_name, REQUEST_POSTFIX_SIZE, GREP_TEMPLATE, kvs_name);

    while (cur_list != NULL)
    {
        if (cur_list->shift.type == CH_T_UPDATE || cur_list->shift.type == CH_T_NEW)
        {
            SET_STR(grep_rank, REQUEST_POSTFIX_SIZE, "| grep \": \\\"%zu\\\"\"", cur_list->shift.old);
            SET_STR(get_new_pod_name_str, REQUEST_POSTFIX_SIZE, CONCAT_THREE_COMMAND_TEMPLATE, grep_name, grep_rank, GET_KEY);

            /*get name new pod by rank cur_list->shift.old*/
            SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, get_new_pod_name_str);

            fp = popen(run_str, READ_ONLY);
            if (fgets(new_pod_name_str, sizeof(new_pod_name_str)-1, fp) == NULL) {
                assert(0);
            }
            pclose(fp);

            new_pod_name_str[strlen(new_pod_name_str) - 1] = NULL_CHAR;

            SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_ACCEPT, new_pod_name_str);
            SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_TEMPLATE_I, kvs_name_key, cur_list->shift.new);

            /*set accept new pod*/
            SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

            fp = popen(run_str, READ_ONLY);
            pclose(fp);
        }
        cur_list = cur_list->next;
    }
    SET_STR(grep_count_accept, REQUEST_POSTFIX_SIZE, GREP_COUNT_TEMPLATE, KVS_ACCEPT);

    /*waiting until all accepted pods will clear the entry "accept"*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, grep_count_accept);

    fp = popen(run_str, READ_ONLY);
    if (fgets(is_accept, sizeof(is_accept)-1, fp) == NULL) {
        assert(0);
    }
    while (atoi(is_accept) != 0)
    {
        pclose(fp);
        fp = popen(run_str, READ_ONLY);
        if (fgets(is_accept, sizeof(is_accept)-1, fp) == NULL) {
            assert(0);
        }
    }
    pclose(fp);
}

void update_kvs_info(size_t new_rank)
{
    FILE* fp;
    char run_str[RUN_REQUEST_SIZE];
    char patch[REQUEST_POSTFIX_SIZE];
    char kvsname[MAX_KVS_NAME_LENGTH];
    char kvs_key[MAX_KVS_KEY_LENGTH];
    char kvs_val[MAX_KVS_VAL_LENGTH];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];
    size_t kvs_list_size = get_kvs_list_size();
    size_t k;

    for (k = 0; k < kvs_list_size; k++)
    {
        cut_head(kvsname, kvs_key, kvs_val);

        SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, kvsname, kvs_key);
        SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_NULL_TEMPLATE, kvs_name_key);

        /*remove old entry*/
        SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

        fp = popen(run_str, READ_ONLY);
        pclose(fp);

        replace_str(kvs_key, my_rank, new_rank);

        SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, kvsname, kvs_key);
        SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_TEMPLATE, kvs_name_key, kvs_val);

        /*insert new entry*/
        SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

        fp = popen(run_str, READ_ONLY);
        pclose(fp);

        put_key(kvsname, kvs_key, kvs_val);
    }
}

void move_to_new_rank(size_t new_rank)
{
    FILE* fp;
    char run_str[RUN_REQUEST_SIZE];
    char patch[REQUEST_POSTFIX_SIZE];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];
    char rank_str[INT_STR_SIZE];

    update_kvs_info(new_rank);
    my_rank = new_rank;

    SET_STR(rank_str, INT_STR_SIZE, SIZE_T_TEMPLATE, my_rank);
    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_POD_REQUEST, KVS_HOSTNAME);
    SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_TEMPLATE, kvs_name_key, rank_str);

    /*set new rank in request entry*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

    fp = popen(run_str, READ_ONLY);
    pclose(fp);

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_POD_NUM, rank_str);
    SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_TEMPLATE, kvs_name_key, KVS_HOSTNAME);

    /*set new rank in pod num entry*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

    fp = popen(run_str, READ_ONLY);
    pclose(fp);
}

void update_my_info(shift_list_t* list)
{
    FILE* fp;
    char run_str[RUN_REQUEST_SIZE];
    char patch[REQUEST_POSTFIX_SIZE];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];
    char rank_str[INT_STR_SIZE];

    while (list != NULL)
    {
        if (list->shift.old == my_rank)
        {
            size_t old_rank = my_rank;
            move_to_new_rank(list->shift.new);

            SET_STR(rank_str, INT_STR_SIZE, SIZE_T_TEMPLATE, old_rank);
            SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_POD_NUM, rank_str);
            SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_NULL_TEMPLATE, kvs_name_key);

            /*Remove old entry with my num*/
            SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

            fp = popen(run_str, READ_ONLY);
            pclose(fp);

            break;
        }
        list = list->next;
    }
}

void post_my_info()
{
    FILE* fp;
    char run_str[RUN_REQUEST_SIZE];
    char patch[REQUEST_POSTFIX_SIZE];
    char barrier_num_str[INT_STR_SIZE];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];
    char kvs_key[MAX_KVS_KEY_LENGTH];
    char get_barriers[REQUEST_POSTFIX_SIZE];
    char get_barriers_num[REQUEST_POSTFIX_SIZE];
    char get_min_barrier_num[REQUEST_POSTFIX_SIZE];

    char my_rank_str[INT_STR_SIZE];
    char grep_new_pod[REQUEST_POSTFIX_SIZE];
    char grep_my_name[REQUEST_POSTFIX_SIZE];
    char get_name_key[REQUEST_POSTFIX_SIZE];

    initialized = 1;

    SET_STR(my_rank_str, INT_STR_SIZE, SIZE_T_TEMPLATE, my_rank);

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_POD_NUM, my_rank_str);
    SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_TEMPLATE, kvs_name_key, KVS_HOSTNAME);

    /*set my rank as pod num*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

    fp = popen(run_str, READ_ONLY);
    pclose(fp);

    SET_STR(get_barriers, REQUEST_POSTFIX_SIZE, GREP_TEMPLATE, KVS_BARRIER);
    SET_STR(get_barriers_num, REQUEST_POSTFIX_SIZE, SED_TEMPLATE, KVS_BARRIER, CLEANER);
    SET_STR(get_min_barrier_num, REQUEST_POSTFIX_SIZE, CONCAT_THREE_COMMAND_TEMPLATE, get_barriers, get_barriers_num, SORT);

    /*get actual min barrier num*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, get_min_barrier_num);

    fp = popen(run_str, READ_ONLY);
    if (fgets(barrier_num_str, INT_STR_SIZE, fp) == NULL) {
        assert(0);
    }
    pclose(fp);

    barrier_num_str[strlen(barrier_num_str) - 1] = NULL_CHAR;
    barrier_num = atoi(barrier_num_str);

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_BARRIER, KVS_HOSTNAME);
    SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_TEMPLATE, kvs_name_key, barrier_num_str);

    /*set my barrier num*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

    fp = popen(run_str, READ_ONLY);
    pclose(fp);

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_ACCEPT, KVS_HOSTNAME);
    SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_NULL_TEMPLATE, kvs_name_key);

    /*remove accept entry*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

    fp = popen(run_str, READ_ONLY);
    pclose(fp);

    SET_STR(grep_new_pod, REQUEST_POSTFIX_SIZE, GREP_TEMPLATE, KVS_NEW_POD);
    SET_STR(kvs_key, MAX_KVS_KEY_LENGTH, "%s\\\"", KVS_HOSTNAME);
    SET_STR(grep_my_name, REQUEST_POSTFIX_SIZE, GREP_TEMPLATE, kvs_key);
    SET_STR(get_name_key, REQUEST_POSTFIX_SIZE, CONCAT_THREE_COMMAND_TEMPLATE, grep_new_pod, grep_my_name, GET_NAME_KEY);

    /*get my "new_pod" entry*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, get_name_key);

    fp = popen(run_str, READ_ONLY);
    if (fgets(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, fp) == NULL) {
        assert(0);
    }
    pclose(fp);
    kvs_name_key[strlen(kvs_name_key) - 1] = NULL_CHAR;

    SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_NULL_TEMPLATE, kvs_name_key);

    /*remove my "new_pod" entry*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

    fp = popen(run_str, READ_ONLY);
    pclose(fp);

    barrier_num++;
    if (barrier_num > BARRIER_NUM_MAX) barrier_num = 0;
}

size_t update(shift_list_t** list, int_list_t** dead_up_idx, size_t root_rank)
{
    if (initialized == 1)
    {
        if ((*list) != NULL)
        {
            if (my_rank == root_rank)
            {
                if ((*dead_up_idx) != NULL)
                    clean_dead_pods_info(*dead_up_idx);

                accept_new_ranks(*list);
            }
            update_my_info(*list);
        }
    }
    else
        post_my_info();

    collect_sock_addr();

    return 0;
}

size_t get_count_occupied_ranks(char* rank)
{
    FILE* fp;
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];
    char count_pod_str[INT_STR_SIZE];
    char count_new_pod_str[INT_STR_SIZE];
    char check_my_rank[REQUEST_POSTFIX_SIZE];
    char grep_new_pod[REQUEST_POSTFIX_SIZE];
    char grep_count_ranks[REQUEST_POSTFIX_SIZE];
    char get_count_new_pods_by_rank[REQUEST_POSTFIX_SIZE];
    char run_str[RUN_REQUEST_SIZE];

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_POD_NUM, rank);
    SET_STR(check_my_rank, REQUEST_POSTFIX_SIZE, GREP_COUNT_TEMPLATE, kvs_name_key);

    /*Get count pod_nums with my rank*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, check_my_rank);

    fp = popen(run_str, READ_ONLY);
    if (fgets(count_pod_str, sizeof(count_pod_str) - 1, fp) == NULL) {
        assert(0);
    }
    pclose(fp);

    SET_STR(grep_new_pod, MAX_KVS_NAME_KEY_LENGTH, GREP_TEMPLATE, KVS_NEW_POD);
    SET_STR(grep_count_ranks, MAX_KVS_NAME_KEY_LENGTH, "| grep -c '\"%s'", rank);
    SET_STR(get_count_new_pods_by_rank, REQUEST_POSTFIX_SIZE, CONCAT_TWO_COMMAND_TEMPLATE, grep_new_pod, grep_count_ranks);

    /*Get count new_pods with my rank*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, get_count_new_pods_by_rank);

    fp = popen(run_str, READ_ONLY);
    if (fgets(count_new_pod_str, sizeof(count_new_pod_str) - 1, fp) == NULL) {
        assert(0);
    }
    pclose(fp);

    return atoi(count_new_pod_str) + atoi(count_pod_str);
}

size_t get_count_requested_ranks(char* rank)
{
    FILE* fp;
    char grep_pod_request[REQUEST_POSTFIX_SIZE];
    char grep_count_ranks[REQUEST_POSTFIX_SIZE];
    char get_count_requested_pods_by_rank[REQUEST_POSTFIX_SIZE];
    char run_str[RUN_REQUEST_SIZE];
    char count_pods_with_my_rank[INT_STR_SIZE];

    SET_STR(grep_pod_request, MAX_KVS_NAME_KEY_LENGTH, GREP_TEMPLATE, KVS_POD_REQUEST);
    SET_STR(grep_count_ranks, MAX_KVS_NAME_KEY_LENGTH, "| grep -c '\"%s'", rank);
    SET_STR(get_count_requested_pods_by_rank, REQUEST_POSTFIX_SIZE, CONCAT_TWO_COMMAND_TEMPLATE, grep_pod_request, grep_count_ranks);

    /*Get count requested_pods with my rank*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, get_count_requested_pods_by_rank);

    fp = popen(run_str, READ_ONLY);
    if (fgets(count_pods_with_my_rank, sizeof(count_pods_with_my_rank) - 1, fp) == NULL) {
        assert(0);
    }
    pclose(fp);

    return atoi(count_pods_with_my_rank);
}

void occupied_rank(char* rank)
{
    FILE *fp;
    char run_str[RUN_REQUEST_SIZE];
    char patch[REQUEST_POSTFIX_SIZE];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];
    char is_inited[INT_STR_SIZE];
    char check_up_idx[REQUEST_POSTFIX_SIZE];

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_UP, KVS_IDX);
    SET_STR(check_up_idx, REQUEST_POSTFIX_SIZE, GREP_COUNT_TEMPLATE, kvs_name_key);

    /*check up_idx, if up_idx don't exist - this is initial phase*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, check_up_idx);

    fp = popen(run_str, READ_ONLY);
    if (fgets(is_inited, sizeof(is_inited) - 1, fp) == NULL) {
        assert(0);
    }
    pclose(fp);

    if ((atoi(is_inited) == 0) && (my_rank == 0))
    {
        SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_UP, KVS_IDX);
        SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_TEMPLATE, kvs_name_key, INITIAL_UPDATE_IDX);

        /*set initial val up_idx*/
        SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

        fp = popen(run_str, READ_ONLY);
        pclose(fp);

        count_pods = 1;

        update(NULL, NULL, 0);
    }
    else
    {
        SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_NEW_POD, KVS_HOSTNAME);
        SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_TEMPLATE, kvs_name_key, rank);

        /*set new_pod as rank*/
        SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

        fp = popen(run_str, READ_ONLY);
        pclose(fp);
    }
}

void reg_rank(void)
{
    FILE *fp;
    char run_str[RUN_REQUEST_SIZE];
    char patch[REQUEST_POSTFIX_SIZE];
    char grep_pod_request[REQUEST_POSTFIX_SIZE];
    char grep_rank[REQUEST_POSTFIX_SIZE];
    char get_my_num_line[REQUEST_POSTFIX_SIZE];
    char check_my_rank_str[REQUEST_POSTFIX_SIZE];
    char rank_str[INT_STR_SIZE];
    char my_num_str[INT_STR_SIZE];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];
    size_t wait_shift = 0;

    my_rank = 0;

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_POD_REQUEST, KVS_HOSTNAME);
    SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_TEMPLATE, kvs_name_key, INITIAL_RANK_NUM);

    /*set pod_reques as initial rank*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

    fp = popen(run_str, READ_ONLY);
    pclose(fp);

    SET_STR(rank_str, INT_STR_SIZE, SIZE_T_TEMPLATE, my_rank);

    SET_STR(grep_pod_request, REQUEST_POSTFIX_SIZE, GREP_TEMPLATE, KVS_POD_REQUEST);
    SET_STR(get_my_num_line, REQUEST_POSTFIX_SIZE, GET_NUM_LINE_TEMPLATE, KVS_HOSTNAME);

    while (1)
    {
        wait_shift = 0;

        SET_STR(grep_rank, REQUEST_POSTFIX_SIZE, "| grep '\"%zu\"'", my_rank);
        SET_STR(check_my_rank_str, REQUEST_POSTFIX_SIZE, CONCAT_THREE_COMMAND_TEMPLATE, grep_pod_request, grep_rank, get_my_num_line);

        /*check: am I first for this rank*/
        SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, check_my_rank_str);

        fp = popen(run_str, READ_ONLY);
        if (fgets(my_num_str, sizeof(my_num_str) - 1, fp) == NULL) {
            assert(0);
        }
        pclose(fp);

        if (atoi(my_num_str) == 1)
        {
            if (get_count_occupied_ranks(rank_str) != 0)
            {
                wait_shift = 0;
            }
            else
            {
                wait_shift = 1;
                if (get_count_requested_ranks(rank_str) == 1)
                {
                    occupied_rank(rank_str);
                    break;
                }
            }
        }
        if (!wait_shift)
        {
            my_rank++;

            SET_STR(rank_str, INT_STR_SIZE, SIZE_T_TEMPLATE, my_rank);
            SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_TEMPLATE, kvs_name_key, rank_str);

            /*set pod_request as rank*/
            SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

            fp = popen(run_str, READ_ONLY);
            pclose(fp);
        }
    }
}

void json_get_val(FILE *fp, const char** keys, size_t keys_count, char* val)
{
    char cur_kvs_str[MAX_KVS_STR_LENGTH];
    char* res;
    size_t i = 0;
    size_t wrong_namespace_depth = 0;
    while (fgets(cur_kvs_str, MAX_KVS_STR_LENGTH, fp))
    {
        if (wrong_namespace_depth == 0)
        {
            if (strstr(cur_kvs_str, keys[i]))
            {
                i++;
                if (i == keys_count)
                    break;
            }
            else
                if (strstr(cur_kvs_str, ": {") || strstr(cur_kvs_str, ": ["))
                    wrong_namespace_depth++;
        }
        else
        {
            if (strstr(cur_kvs_str, "{") || strstr(cur_kvs_str, "["))
                wrong_namespace_depth++;
            if (strstr(cur_kvs_str, "}") || strstr(cur_kvs_str, "]"))
                wrong_namespace_depth--;
        }
    }
    res = strstr(cur_kvs_str, ":");
    res += SHIFT_TO_VAL;
    if (strstr(res, ","))
        res[strlen(res) - 1] = '\0';
    res[strlen(res) - 2] = '\0';
    strncpy(val, res, MAX_KVS_VAL_LENGTH);
}

size_t get_replica_size(void)
{
    FILE *fp;
    size_t replica_size;
    char run_str[RUN_REQUEST_SIZE];
    char replica_size_str[INT_STR_SIZE];
    const char* replica_keys[] = {"spec", "replicas"};

    /*get full output*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, "");

    fp = popen(run_str, READ_ONLY);
    json_get_val(fp, replica_keys, 2, replica_size_str);
    pclose(fp);

    replica_size = atoi(replica_size_str);

    return replica_size;
}

void split_kvs_name_key_val(char* full_str, char* kvs_name, char* kvs_key, char* kvs_val)
{
    char *tmp_kvs_name,
         *tmp_kvs_key,
         *tmp_kvs_val;

    tmp_kvs_name = strstr(full_str, "\"");
    tmp_kvs_name[0] = '\0';
    tmp_kvs_name++;

    tmp_kvs_key = strstr(tmp_kvs_name, "-");
    tmp_kvs_key[0] = '\0';
    tmp_kvs_key++;

    tmp_kvs_val = strstr(tmp_kvs_key, "\"");
    tmp_kvs_val[0] = '\0';
    tmp_kvs_val++;

    tmp_kvs_val += SHIFT_TO_VAL;
    if (strstr(tmp_kvs_val, ","))
        tmp_kvs_val[strlen(tmp_kvs_val) - 1] = '\0';
    tmp_kvs_val[strlen(tmp_kvs_val) - 2] = '\0';

    strncpy(kvs_name, tmp_kvs_name, MAX_KVS_NAME_LENGTH);
    strncpy(kvs_key, tmp_kvs_key, MAX_KVS_KEY_LENGTH);
    strncpy(kvs_val, tmp_kvs_val, MAX_KVS_VAL_LENGTH);
}

void up_kvs(size_t up_idx, const char* request)
{
    FILE *fp,
         *patch_fp;
    char patch[REQUEST_POSTFIX_SIZE];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];
    char run_str[RUN_REQUEST_SIZE];
    char grep_name_key[REQUEST_POSTFIX_SIZE];
    char kvs_name_key_val[MAX_KVS_NAME_KEY_VAL_LENGTH];
    char kvs_name_up[MAX_KVS_NAME_LENGTH];
    char kvs_name[MAX_KVS_NAME_LENGTH],
         kvs_key[MAX_KVS_KEY_LENGTH],
         kvs_val[MAX_KVS_VAL_LENGTH];

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, request, "");
    SET_STR(grep_name_key, REQUEST_POSTFIX_SIZE, GREP_TEMPLATE, kvs_name_key);

    /*get "request" entry (new or dead)*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, grep_name_key);

    fp = popen(run_str, READ_ONLY);
    while (fgets(kvs_name_key_val, MAX_KVS_NAME_KEY_VAL_LENGTH, fp))
    {
        split_kvs_name_key_val(kvs_name_key_val, kvs_name, kvs_key, kvs_val);

        SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, kvs_name, kvs_key);
        SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_NULL_TEMPLATE, kvs_name_key);

        /*remove "request" entry(new or dead)*/
        SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

        patch_fp = popen(run_str, READ_ONLY);
        pclose(patch_fp);

        SET_STR(kvs_name_up, MAX_KVS_NAME_LENGTH, KVS_NAME_TEMPLATE_I, kvs_name, up_idx);
        SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, kvs_name_up, kvs_key);
        SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_TEMPLATE, kvs_name_key, kvs_val);

        /*set up_idx for "request"(NEW_POD-test1:1 -> NEW_POD_2-test1:1)*/
        SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

        patch_fp = popen(run_str, READ_ONLY);
        pclose(patch_fp);
    }
    pclose(fp);
}
void up_kvs_new_and_dead(size_t up_idx)
{
    up_kvs(up_idx, KVS_NEW_POD);
    up_kvs(up_idx, KVS_DEAD_POD);
}
