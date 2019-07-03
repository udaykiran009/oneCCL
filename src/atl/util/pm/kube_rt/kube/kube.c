#include "kube.h"
#include "helper.h"

typedef enum framework_answers
{
    FA_WAIT     = 0,
    FA_USE      = 1,
    FA_FINALIZE = 2,
}framework_answers_t;

static size_t root_rank = 0;
static size_t is_new_root = 0;
static size_t is_need_additional_update = 0;
static size_t ask_only_framework = 0;
static size_t finalized = 0;
static size_t extreme_finalize = 0;

framework_answers_t default_checker(size_t comm_size)
{
    char* comm_size_to_start_env;
    size_t comm_size_to_start;

    comm_size_to_start_env = getenv(RANK_COUNT_ENV);

    if (comm_size_to_start_env != NULL)
        comm_size_to_start = atoi(comm_size_to_start_env);
    else
        comm_size_to_start = get_replica_size();
    if (comm_size >= comm_size_to_start)
        return FA_USE;

    return FA_WAIT;
}

static update_checker__f ask_framework = &default_checker;

void use_additional_update(int sig)
{
    is_need_additional_update = 1;
}

int KUBE_API KUBE_Update()
{
    FILE *fp;
    char get_up_idx[REQUEST_POSTFIX_SIZE];
    char grep_name_key[REQUEST_POSTFIX_SIZE];
    char sed_name_key[REQUEST_POSTFIX_SIZE];
    char run_get_up_str[RUN_REQUEST_SIZE];
    char run_set_up_str[RUN_REQUEST_SIZE];
    char patch[REQUEST_POSTFIX_SIZE];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];
    char up_idx_str[INT_STR_SIZE];
    size_t prev_new_ranks_count = 0;
    size_t prev_killed_ranks_count = 0;
    int prev_idx = -1;
    framework_answers_t answer;
    int_list_t* dead_up_idx = NULL;
    shift_list_t* list = NULL;

    new_ranks_count = 0;
    killed_ranks_count = 0;

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_UP, KVS_IDX);
    SET_STR(grep_name_key, REQUEST_POSTFIX_SIZE, GREP_TEMPLATE, kvs_name_key);
    SET_STR(sed_name_key, REQUEST_POSTFIX_SIZE, SED_TEMPLATE, kvs_name_key, CLEANER);
    SET_STR(get_up_idx, REQUEST_POSTFIX_SIZE, CONCAT_TWO_COMMAND_TEMPLATE, grep_name_key, sed_name_key);

    SET_STR(run_get_up_str, RUN_REQUEST_SIZE, run_v2_template, get_up_idx);

    if (initialized == 1)
    {
        size_t is_wait = 1;
        size_t is_first_collect = 0;

        fp = popen(run_get_up_str, READ_ONLY);
        fgets(up_idx_str, sizeof(up_idx_str)-1, fp);
        pclose(fp);

        up_idx = atoi(up_idx_str);
        if (up_idx == 0)
            is_first_collect = 1;

        do
        {
            size_t count_clean_checks = 0;
            size_t count_applyed_changes = 0;
            is_need_additional_update = 0;
            do
            {
                fp = popen(run_get_up_str, READ_ONLY);
                fgets(up_idx_str, sizeof(up_idx_str)-1, fp);
                pclose(fp);

                up_idx = atoi(up_idx_str);
                if (prev_idx == up_idx)
                {
                    keep_first_n_up(prev_new_ranks_count, prev_killed_ranks_count);
                    new_ranks_count    = prev_new_ranks_count;
                    killed_ranks_count = prev_killed_ranks_count;
                }

                prev_idx                = up_idx;
                prev_new_ranks_count    = new_ranks_count;
                prev_killed_ranks_count = killed_ranks_count;

                get_update_ranks(up_idx);
                if (killed_ranks_count != prev_killed_ranks_count)
                    int_list_add(&dead_up_idx, up_idx);

                while (int_list_is_contained(killed_ranks, root_rank) == 1)
                {
                    root_rank++;
                    if (my_rank == root_rank)
                        is_new_root = 1;
                    else
                        is_new_root = 0;
                }

                KUBE_Barrier();
                if (my_rank == root_rank && is_new_root == 0)
                {
                    up_idx++;
                    if (up_idx > MAX_UP_IDX)
                        up_idx = 1;

                    SET_STR(up_idx_str, INT_STR_SIZE, SIZE_T_TEMPLATE, up_idx);
                    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_UP, KVS_IDX);
                    SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_TEMPLATE, kvs_name_key, up_idx_str);

                    SET_STR(run_set_up_str, RUN_REQUEST_SIZE, run_v2_template, patch);

                    fp = popen(run_set_up_str, READ_ONLY);
                    pclose(fp);
                    up_kvs_new_and_dead(up_idx);
                }
                KUBE_Barrier();

                if (finalized == 1)
                {
                    int_list_clean(&killed_ranks);
                    int_list_clean(&new_ranks);
                    int_list_clean(&dead_up_idx);
                    return 1;
                }

                is_new_root = 0;
                count_applyed_changes += new_ranks_count - prev_new_ranks_count + killed_ranks_count - prev_killed_ranks_count;

                if ((prev_new_ranks_count != new_ranks_count) || (prev_killed_ranks_count != killed_ranks_count))
                    count_clean_checks = 0;
                else
                    count_clean_checks++;
            } while(count_clean_checks != MAX_CLEAN_CHECKS);

            if (!is_first_collect || ask_only_framework == 1)
                answer = ask_framework(count_pods - killed_ranks_count + new_ranks_count);
            else
            {
                if (get_replica_size() != count_pods - killed_ranks_count + new_ranks_count)
                    answer = FA_WAIT;
                else
                    answer = FA_USE;
            }

            switch (answer) 
            {
            case FA_WAIT:
                break;
            case FA_USE:
                is_wait = 0;
                break;
            case FA_FINALIZE:
                KUBE_Finalize();
                return 1;
            }
            is_need_additional_update = is_new_up(count_applyed_changes);
        } while (is_wait == 1 ||
                 is_need_additional_update == 1);
    }
    else
    {
        wait_accept();
        is_new_up(-1);
    }

    get_shift(&list);
    count_pods = count_pods - killed_ranks_count + new_ranks_count;
    update(&list, &dead_up_idx, root_rank);

    is_need_additional_update = 0;
    root_rank = 0;

    KUBE_Barrier();
    up_pods_count();

    int_list_clean(&killed_ranks);
    int_list_clean(&new_ranks);
    int_list_clean(&dead_up_idx);
    shift_list_clean(&list);
    return 0;
}

void Extreme_finalize(int sig)
{
    FILE *fp;
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];
    char run_str[RUN_REQUEST_SIZE];
    char patch[REQUEST_POSTFIX_SIZE];
    char rank_str[INT_STR_SIZE];

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_DEAD_POD, KVS_HOSTNAME);

    SET_STR(rank_str, INT_STR_SIZE, SIZE_T_TEMPLATE, my_rank);
    SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_TEMPLATE, kvs_name_key, rank_str);

    /*set: ICCL_DEAD_POD-$HOSTNAME=rank*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

    fp = popen(run_str, READ_ONLY);
    pclose(fp);
    send_notification(sig);

    extreme_finalize = 1;
    KUBE_Finalize();
    finalized = 1;
}

int KUBE_API KUBE_Init(void)
{
    FILE *fp_name;
    FILE *fp_type;
    sigset_t   set;
    size_t i;
    size_t kind_type_size;
    struct sigaction act, act1;
    char run_str[RUN_REQUEST_SIZE];
    char kind_type[MAX_KVS_NAME_LENGTH];
    char kind_name[MAX_KVS_NAME_LENGTH];
    char kind_path[MAX_KVS_NAME_KEY_LENGTH];
    char connect_api_template[RUN_TEMPLATE_SIZE];
    char* kube_api_addr = getenv(KUBE_API_ADDR_ENV);
    char* ask_framework_env = getenv(ASK_ONLY_FRAMEWORK_ENV);
    const char* kind_type_key[] = {"metadata", "ownerReferences", "kind"};
    const char* kind_name_key[] = {"metadata", "ownerReferences", "name"};

    if (kube_api_addr == NULL)
    {
        printf("%s not set\n", KUBE_API_ADDR_ENV);
        return 1;
    }

    if (ask_framework_env != NULL)
        ask_only_framework = atoi(ask_framework_env);

//    ask_framework = &default_checker;

    SET_STR(connect_api_template, RUN_TEMPLATE_SIZE, ADDR_STR_V1_TEMPLATE, kube_api_addr);

    /*get full pod info*/
    SET_STR(run_str, RUN_REQUEST_SIZE, AUTORIZATION_TEMPLATE, connect_api_template, KVS_HOSTNAME, "");

    memset(kind_type, NULL_CHAR, MAX_KVS_NAME_LENGTH);
    fp_name = popen(run_str, READ_ONLY);
    json_get_val(fp_name, kind_type_key, 3, kind_type);

    /*we must use the plural to access to statefulset/deployment KVS*/
    kind_type_size = strlen(kind_type);
    kind_type[kind_type_size] = 's';
    kind_type_size++;
    for (i = 0; i < kind_type_size; i++)
        kind_type[i] = tolower(kind_type[i]);

    memset(kind_name, NULL_CHAR, MAX_KVS_NAME_LENGTH);
    fp_type = popen(run_str, READ_ONLY);
    json_get_val(fp_type, kind_name_key, 3, kind_name);

    SET_STR(kind_path, MAX_KVS_NAME_LENGTH, "%s/%s", kind_type, kind_name);
    SET_STR(connect_api_template, RUN_TEMPLATE_SIZE, ADDR_STR_V2_TEMPLATE, kube_api_addr);

    SET_STR(run_v2_template, RUN_TEMPLATE_SIZE, AUTORIZATION_TEMPLATE, connect_api_template, kind_path, "%s");

    pclose(fp_name);
    pclose(fp_type);
    reg_rank();

    up_pods_count();

    memset(&act1, 0, sizeof(act1));
    act1.sa_handler = &Extreme_finalize;
    act1.sa_flags = 0;
    sigaction(SIGTERM, &act1, 0);

    memset(&act, 0, sizeof(act));
    act.sa_handler = &use_additional_update;
    act.sa_flags = 0;
    sigemptyset(&set);
    sigaction(SIGUSR1, &act, 0);
    create_listner(run_v2_template, RUN_TEMPLATE_SIZE);
    collect_sock_addr();
    return 0;
}

void add_str(char* dst, const char* src)
{
    if (strlen(dst) == 0)
    {
        SET_STR(dst, REQUEST_POSTFIX_SIZE, "%s", src);
    }
    else
    {
        char tmp[REQUEST_POSTFIX_SIZE];

        SET_STR(tmp, REQUEST_POSTFIX_SIZE, "%s", dst);
        SET_STR(dst, REQUEST_POSTFIX_SIZE, "%s,%s", tmp, src);
    }
}


int KUBE_API KUBE_Finalize(void)
{
    FILE *fp;
    char patch[REQUEST_POSTFIX_SIZE];
    char patch_res[REQUEST_POSTFIX_SIZE];
    char patch_null[REQUEST_POSTFIX_SIZE];
    char run_str[RUN_REQUEST_SIZE];
    char kvs_name[MAX_KVS_NAME_LENGTH];
    char kvs_key[MAX_KVS_KEY_LENGTH];
    char kvs_val[MAX_KVS_VAL_LENGTH];
    char rank_str[INT_STR_SIZE];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];

    if (finalized) return 0;

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_POD_REQUEST, KVS_HOSTNAME);
    SET_STR(patch_res, REQUEST_POSTFIX_SIZE, KVS_NULL_TEMPLATE, kvs_name_key);

    SET_STR(rank_str, INT_STR_SIZE, SIZE_T_TEMPLATE, my_rank);
    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_POD_NUM, rank_str);
    SET_STR(patch_null, REQUEST_POSTFIX_SIZE, KVS_NULL_TEMPLATE, kvs_name_key);
    SET_STR(&(patch_res[strlen(patch_res)]), REQUEST_POSTFIX_SIZE, ",%s", patch_null);

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_BARRIER, KVS_HOSTNAME);
    SET_STR(patch_null, REQUEST_POSTFIX_SIZE, KVS_NULL_TEMPLATE, kvs_name_key);
    SET_STR(&(patch_res[strlen(patch_res)]), REQUEST_POSTFIX_SIZE, ",%s", patch_null);

    SET_STR(patch, REQUEST_POSTFIX_SIZE, BASE_PATCH_TEMPLATE, patch_res);

    /*remove request, num and barrier entries*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

    fp = popen(run_str, READ_ONLY);
    pclose(fp);

    while (cut_head(kvs_name, kvs_key, kvs_val))
    {
        SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, kvs_name, kvs_key);
        SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_NULL_TEMPLATE, kvs_name_key);

        /*remove posted entries(basically - addresses)*/
        SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);
    
        fp = popen(run_str, READ_ONLY);
        pclose(fp);
    }

    if (my_rank == 0 && extreme_finalize != 1)
    {
        SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_UP, KVS_IDX);
        SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_NULL_TEMPLATE, kvs_name_key);

        /*Remove global up_idx*/
        SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

        fp = popen(run_str, READ_ONLY);
        pclose(fp);
    }

    delete_listner();

    return 0;
}

int KUBE_API KUBE_Barrier(void)
{
    FILE *fp;
    char patch[REQUEST_POSTFIX_SIZE];
    char run_str[RUN_REQUEST_SIZE];
    char get_barriers[REQUEST_POSTFIX_SIZE];
    char get_barriers_num[REQUEST_POSTFIX_SIZE];
    char get_min_barrier_num[REQUEST_POSTFIX_SIZE];
    char min_barrier_num[INT_STR_SIZE];
    char barrier_num_str[INT_STR_SIZE];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];

    if (finalized) return 0;

    SET_STR(barrier_num_str, INT_STR_SIZE, SIZE_T_TEMPLATE, barrier_num);

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, KVS_BARRIER, KVS_HOSTNAME);

    SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_TEMPLATE, kvs_name_key, barrier_num_str);

    /*set my barrier idx*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);
    
    fp = popen(run_str, READ_ONLY);
    pclose(fp);

    SET_STR(get_barriers, REQUEST_POSTFIX_SIZE, GREP_TEMPLATE, KVS_BARRIER);
    SET_STR(get_barriers_num, REQUEST_POSTFIX_SIZE, SED_TEMPLATE, KVS_BARRIER, CLEANER);
    SET_STR(get_min_barrier_num, REQUEST_POSTFIX_SIZE, CONCAT_THREE_COMMAND_TEMPLATE, get_barriers, get_barriers_num, SORT);

    /*get min barrier_idx*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, get_min_barrier_num);

    fp = popen(run_str, READ_ONLY);
    fgets(min_barrier_num, INT_STR_SIZE, fp);
    while (atoi(min_barrier_num) != barrier_num && finalized != 1)
    {
        pclose(fp);
        fp = popen(run_str, READ_ONLY);
        fgets(min_barrier_num, INT_STR_SIZE, fp);
    }
    pclose(fp);

    barrier_num++;
    if (barrier_num > BARRIER_NUM_MAX) barrier_num = 0;

    return 0;
}

int KUBE_API KUBE_Get_size(size_t *size)
{
    *size = count_pods;
    return 0;
}

int KUBE_API KUBE_Get_rank(size_t *rank)
{
    *rank = my_rank;
    return 0;
}

int KUBE_API KUBE_KVS_Get_my_name(char kvsname[], size_t length)
{
    strncpy(kvsname, KVS_NAME, length);
    return 0;
}

int KUBE_API KUBE_KVS_Get_name_length_max(size_t *length)
{
    *length =  MAX_KVS_NAME_LENGTH;
    return 0;
}

int KUBE_API KUBE_KVS_Get_key_length_max(size_t *length)
{
    *length = MAX_KVS_KEY_LENGTH;
    return 0;
}

int KUBE_API KUBE_KVS_Get_value_length_max(size_t *length)
{
    *length = MAX_KVS_VAL_LENGTH;
    return 0;
}

int KUBE_API KUBE_KVS_Commit(const char kvsname[])
{
    return 0;
}

int KUBE_API KUBE_KVS_Put(const char kvsname[], const char key[], const char value[])
{
    FILE* fp;
    char patch[REQUEST_POSTFIX_SIZE];
    char run_str[RUN_REQUEST_SIZE];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];

    put_key(kvsname, key, value);

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, kvsname, key);
    SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_TEMPLATE, kvs_name_key, value);

    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, patch);

    fp = popen(run_str, READ_ONLY);
    pclose(fp);
    return 0;
}

int KUBE_API KUBE_KVS_Get(const char kvsname[], const char key[], char value[], size_t length)
{
    FILE *fp;

    char run_str[RUN_REQUEST_SIZE];
    char get_val[REQUEST_POSTFIX_SIZE];
    char get_name_key[REQUEST_POSTFIX_SIZE];
    char sed_name_key[REQUEST_POSTFIX_SIZE];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];
    char kvs_val[MAX_KVS_VAL_LENGTH];

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, kvsname, key);

    SET_STR(get_name_key, REQUEST_POSTFIX_SIZE, GREP_TEMPLATE, kvs_name_key);
    SET_STR(sed_name_key, REQUEST_POSTFIX_SIZE, SED_TEMPLATE, kvs_name_key, CLEANER);
    SET_STR(get_val, REQUEST_POSTFIX_SIZE, CONCAT_TWO_COMMAND_TEMPLATE, get_name_key, sed_name_key);

    SET_STR(run_str, RUN_REQUEST_SIZE, run_v2_template, get_val);

    fp = popen(run_str, READ_ONLY);
    fgets(kvs_val, MAX_KVS_VAL_LENGTH, fp);
    while (kvs_val[0] ==  NULL_CHAR)
    {
        pclose(fp);
        fp = popen(run_str, READ_ONLY);
        fgets(kvs_val, MAX_KVS_VAL_LENGTH, fp);
    }
    pclose(fp);

    kvs_val[strlen(kvs_val) - 1] = NULL_CHAR;

    memset(value, NULL_CHAR, length);
    memcpy(value, kvs_val, length);
    return 0;
}

int KUBE_API KUBE_set_framework_function(update_checker__f user_checker)
{
    ask_framework = user_checker;
    return 0;
}
