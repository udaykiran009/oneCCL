#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "request_wrappers_k8s.h"
#include "def.h"

#define MAX_KVS_STR_LENGTH 1024
#define CCL_K8S_MANAGER_TYPE_ENV "CCL_K8S_MANAGER_TYPE"
#define CCL_K8S_API_ADDR_ENV "CCL_K8S_API_ADDR"


char run_get_template[RUN_TEMPLATE_SIZE];
char run_set_template[RUN_TEMPLATE_SIZE];

typedef enum manager_type
{
    MT_NONE = 0,
    MT_K8S  = 1,
} manager_type_t;

manager_type_t manager;

void json_get_val(FILE* fp, const char** keys, size_t keys_count, char* val)
{
    char cur_kvs_str[MAX_KVS_STR_LENGTH];
    char* res;
    char last_char;
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
            else if (strstr(cur_kvs_str, ": {") || strstr(cur_kvs_str, ": ["))
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
    res++;
    while (res[0] == ' ')
        res++;

    if (res[0] == '"' || res[0] == '\'')
        res++;

    last_char = res[strlen(res) - 1];
    while (last_char == '\n' ||
           last_char == ','  ||
           last_char == ' '  ||
           last_char == '"'  ||
           last_char == ' ')
    {
        res[strlen(res) - 1] = '\0';
        last_char = res[strlen(res) - 1];
    }
    STR_COPY(val, res, MAX_KVS_VAL_LENGTH);
    while (fgets(cur_kvs_str, MAX_KVS_STR_LENGTH, fp))
    {}
}

size_t k8s_init_with_manager()
{
    FILE* fp_name;
    FILE* fp_type;
    size_t i;
    size_t kind_type_size;
    char run_str[RUN_REQUEST_SIZE];
    char kind_type[MAX_KVS_NAME_LENGTH];
    char kind_name[MAX_KVS_NAME_LENGTH];
    char kind_path[MAX_KVS_NAME_KEY_LENGTH];
    char connect_api_template[RUN_TEMPLATE_SIZE];
    char* kube_api_addr = getenv(CCL_K8S_API_ADDR_ENV);
    const char* kind_type_key[] = {"metadata", "ownerReferences", "kind"};
    const char* kind_name_key[] = {"metadata", "ownerReferences", "name"};

    if (kube_api_addr == NULL)
    {
        printf("%s not set\n", CCL_K8S_API_ADDR_ENV);
        return 1;
    }

    SET_STR(connect_api_template, RUN_TEMPLATE_SIZE, ADDR_STR_V1_TEMPLATE, kube_api_addr);

    /*get full pod info*/
    SET_STR(run_str, RUN_REQUEST_SIZE, AUTHORIZATION_TEMPLATE, connect_api_template, my_hostname, "");

    memset(kind_type, NULL_CHAR, MAX_KVS_NAME_LENGTH);
    if ((fp_name = popen(run_str, READ_ONLY)) == NULL)
    {
        printf("Can't get kind_type\n");
        exit(1);
    }
    json_get_val(fp_name, kind_type_key, 3, kind_type);

    /*we must use the plural to access to statefulset/deployment KVS*/
    kind_type_size = strlen(kind_type);
    kind_type[kind_type_size] = 's';
    kind_type_size++;
    for (i = 0; i < kind_type_size; i++)
        kind_type[i] = (char)tolower(kind_type[i]);

    memset(kind_name, NULL_CHAR, MAX_KVS_NAME_LENGTH);
    if ((fp_type = popen(run_str, READ_ONLY)) == NULL)
    {
        printf("Can't get kind_name\n");
        exit(1);
    }
    json_get_val(fp_type, kind_name_key, 3, kind_name);

    SET_STR(kind_path, MAX_KVS_NAME_LENGTH, "%s/%s", kind_type, kind_name);
    SET_STR(connect_api_template, RUN_TEMPLATE_SIZE, ADDR_STR_V2_TEMPLATE, kube_api_addr);

    SET_STR(run_get_template, RUN_TEMPLATE_SIZE, AUTHORIZATION_TEMPLATE, connect_api_template, kind_path, "%s");
    SET_STR(run_set_template, RUN_TEMPLATE_SIZE, AUTHORIZATION_TEMPLATE, connect_api_template, kind_path, "%s");

    pclose(fp_name);
    pclose(fp_type);
    return 0;
}

size_t k8s_init_without_manager()
{
    char* kube_api_addr = getenv(CCL_K8S_API_ADDR_ENV);
    char connect_api_template[RUN_TEMPLATE_SIZE];

    if (kube_api_addr == NULL)
    {
        printf("%s not set\n", CCL_K8S_API_ADDR_ENV);
        return 1;
    }

    SET_STR(connect_api_template, RUN_TEMPLATE_SIZE, ADDR_STR_V1_TEMPLATE, kube_api_addr);
    SET_STR(run_get_template, RUN_TEMPLATE_SIZE, AUTHORIZATION_TEMPLATE, connect_api_template, " | sort ", "%s");
    SET_STR(run_set_template, RUN_TEMPLATE_SIZE, AUTHORIZATION_TEMPLATE, connect_api_template, my_hostname, "%s");
    return 0;
}

size_t request_k8s_kvs_init()
{
    char* manager_type_env = getenv(CCL_K8S_MANAGER_TYPE_ENV);

    if (!manager_type_env || strstr(manager_type_env, "none"))
    {
        manager = MT_NONE;
    }
    else if (strstr(manager_type_env, "k8s"))
    {
        manager = MT_K8S;
    }
    else
    {
        printf("Unknown %s = %s, running with \"none\"\n",
               CCL_K8S_MANAGER_TYPE_ENV, manager_type_env);
        manager = MT_NONE;
    }

    switch (manager)
    {
    case MT_NONE:
        return k8s_init_without_manager();
    case MT_K8S:
        return k8s_init_with_manager();
    }
    return 1;
}

size_t request_k8s_kvs_finalize(void)
{

    request_k8s_remove_name_key(CCL_KVS_IP, my_hostname);
    request_k8s_remove_name_key(CCL_KVS_PORT, my_hostname);
    return 0;
}

size_t get_by_template(char*** kvs_entry, const char* request, const char* template, int count, int max_count)
{
    FILE* fp;
    char get_val[REQUEST_POSTFIX_SIZE];
    char run_str[RUN_REQUEST_SIZE];
    size_t i;

    if (*kvs_entry != NULL)
        free(*kvs_entry);

    *kvs_entry = (char**) malloc(sizeof(char*) * count);
    for (i = 0; i < count; i++)
        (*kvs_entry)[i] = (char*) malloc(sizeof(char) * max_count);

    i = 0;

    SET_STR(get_val, REQUEST_POSTFIX_SIZE, CONCAT_TWO_COMMAND_TEMPLATE, request, template);
    SET_STR(run_str, RUN_REQUEST_SIZE, run_get_template, get_val);
    if ((fp = popen(run_str, READ_ONLY)) == NULL)
    {
        printf("Can't get by template\n");
        exit(1);
    }
    while ((fgets((*kvs_entry)[i], max_count, fp) != NULL) &&
           (i < count))
    {
        while ((*kvs_entry)[i][strlen((*kvs_entry)[i]) - 1] == '\n' ||
               (*kvs_entry)[i][strlen((*kvs_entry)[i]) - 1] == ' ')
            (*kvs_entry)[i][strlen((*kvs_entry)[i]) - 1] = NULL_CHAR;
        i++;
    }
    pclose(fp);
    return 0;
}

size_t request_k8s_get_keys_values_by_name(const char* kvs_name, char*** kvs_keys, char*** kvs_values)
{
    FILE* fp;
    char run_str[RUN_REQUEST_SIZE];
    char grep_name_str[REQUEST_POSTFIX_SIZE];
    char get_name_count[REQUEST_POSTFIX_SIZE];
    char values_count_str[INT_STR_SIZE];
    size_t values_count;

    SET_STR(get_name_count, REQUEST_POSTFIX_SIZE, GREP_COUNT_TEMPLATE, kvs_name);

    /*get dead/new rank*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_get_template, get_name_count);

    if ((fp = popen(run_str, READ_ONLY)) == NULL)
    {
        printf("Can't get keys-values by name: %s\n", kvs_name);
        exit(1);
    }
    fgets(values_count_str, INT_STR_SIZE, fp);
    pclose(fp);

    values_count = strtol(values_count_str, NULL, 10);
    if (values_count == 0)
        return 0;

    SET_STR(grep_name_str, REQUEST_POSTFIX_SIZE, GREP_TEMPLATE, kvs_name);
    if (kvs_values != NULL)
    {
        get_by_template(kvs_values, grep_name_str, GET_VAL, values_count, MAX_KVS_VAL_LENGTH);
    }
    if (kvs_keys != NULL)
    {
        get_by_template(kvs_keys, grep_name_str, GET_KEY, values_count, MAX_KVS_KEY_LENGTH);
    }
    return values_count;
}

size_t request_k8s_get_count_names(const char* kvs_name)
{
    FILE* fp;
    char run_str[RUN_REQUEST_SIZE];
    char get_count_str[REQUEST_POSTFIX_SIZE];
    char count_names[INT_STR_SIZE];

    SET_STR(get_count_str, REQUEST_POSTFIX_SIZE, GREP_COUNT_TEMPLATE, kvs_name);

    /*get count accepted pods (like comm_world size)*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_get_template, get_count_str);

    if ((fp = popen(run_str, READ_ONLY)) == NULL)
    {
        printf("Can't get names count: %s\n", kvs_name);
        exit(1);
    }
    fgets(count_names, INT_STR_SIZE, fp);
    pclose(fp);

    return strtol(count_names, NULL, 10);
}

size_t request_k8s_get_val_by_name_key(const char* kvs_name, const char* kvs_key, char* kvs_val)
{

    FILE* fp;
    char run_str[RUN_REQUEST_SIZE];
    char grep_kvs_name_key[REQUEST_POSTFIX_SIZE];
    char get_kvs_val[REQUEST_POSTFIX_SIZE];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, kvs_name, kvs_key);
    SET_STR(grep_kvs_name_key, REQUEST_POSTFIX_SIZE, GREP_TEMPLATE, kvs_name_key);
    SET_STR(get_kvs_val, REQUEST_POSTFIX_SIZE, CONCAT_TWO_COMMAND_TEMPLATE, grep_kvs_name_key, GET_VAL);

    /*check: is me accepted*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_get_template, get_kvs_val);

    if ((fp = popen(run_str, READ_ONLY)) == NULL)
    {
        printf("Can't get value by name-key: %s\n", kvs_name_key);
        exit(1);
    }
    fgets(kvs_val, MAX_KVS_VAL_LENGTH, fp);
    pclose(fp);
    kvs_val[strlen(kvs_val) - 1] = NULL_CHAR;
    return strlen(kvs_val);
}

size_t request_k8s_remove_name_key(const char* kvs_name, const char* kvs_key)
{
    FILE* fp;
    char run_str[RUN_REQUEST_SIZE];
    char patch[REQUEST_POSTFIX_SIZE];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, kvs_name, kvs_key);
    SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_NULL_TEMPLATE, kvs_name_key);

    /*remove old entry*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_set_template, patch);

    if ((fp = popen(run_str, READ_ONLY)) == NULL)
    {
        printf("Can't remove name-key: %s\n", kvs_name_key);
        exit(1);
    }
    pclose(fp);
    return 0;
}

size_t request_k8s_set_val(const char* kvs_name, const char* kvs_key, const char* kvs_val)
{
    FILE* fp;
    char run_str[RUN_REQUEST_SIZE];
    char patch[REQUEST_POSTFIX_SIZE];
    char kvs_name_key[MAX_KVS_NAME_KEY_LENGTH];

    SET_STR(kvs_name_key, MAX_KVS_NAME_KEY_LENGTH, KVS_NAME_KEY_TEMPLATE, kvs_name, kvs_key);
    SET_STR(patch, REQUEST_POSTFIX_SIZE, PATCH_TEMPLATE, kvs_name_key, kvs_val);

    /*insert new entry*/
    SET_STR(run_str, RUN_REQUEST_SIZE, run_set_template, patch);

    if ((fp = popen(run_str, READ_ONLY)) == NULL)
    {
        printf("Can't set name-key-val: %s-%s\n", kvs_name_key, kvs_val);
        exit(1);
    }
    pclose(fp);
    return 0;
}

size_t request_k8s_get_replica_size(void)
{
    FILE* fp;
    char run_str[RUN_REQUEST_SIZE];
    char replica_size_str[MAX_KVS_VAL_LENGTH];
    const char* replica_keys[] = {"spec", "replicas"};

    switch (manager)
    {
    case MT_NONE:
        return request_k8s_get_count_names(CCL_KVS_IP);
    case MT_K8S:
        /*get full output*/
        SET_STR(run_str, RUN_REQUEST_SIZE, run_get_template, "");

        if ((fp = popen(run_str, READ_ONLY)) == NULL)
        {
            printf("Can't get replica size\n");
            exit(1);
        }
        json_get_val(fp, replica_keys, 2, replica_size_str);
        pclose(fp);
        return strtol(replica_size_str, NULL, 10);
    }
    return 0;
}