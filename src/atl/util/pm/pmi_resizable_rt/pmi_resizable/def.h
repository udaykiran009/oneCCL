#ifndef DEF_INCLUDED
#define DEF_INCLUDED


//TODO: change exit to something more useful
#define SET_STR(dst, size, ...)                         \
  do                                                    \
  {                                                     \
     if (snprintf(dst, size, __VA_ARGS__) > size)       \
     {                                                  \
        printf("Line so big (must be low %d)\n", size); \
        printf(__VA_ARGS__);                            \
        exit(1);                                        \
     }                                                  \
  } while(0)

#define BARRIER_NUM_MAX      1024
#define MAX_KVS_KEY_LENGTH   64
#define MAX_KVS_VAL_LENGTH   64
#define MAX_KVS_NAME_LENGTH  64
#define MAX_KVS_NAME_KEY_LENGTH  (MAX_KVS_NAME_LENGTH + MAX_KVS_KEY_LENGTH + 1)
#define RUN_TEMPLATE_SIZE    1024
#define INT_STR_SIZE         8
#define REQUEST_POSTFIX_SIZE 1024
#define RUN_REQUEST_SIZE     2048

#define CCL_KVS_IP_PORT_ENV "CCL_KVS_IP_PORT"
#define CCL_KVS_IP_EXCHANGE_ENV "CCL_KVS_IP_EXCHANGE"
#define CCL_WORLD_SIZE_ENV "CCL_WORLD_SIZE"

#define CCL_KVS_IP_EXCHANGE_VAL_ENV "env"
#define CCL_KVS_IP_EXCHANGE_VAL_K8S "k8s"

#define ADDR_STR_V1_TEMPLATE "https://%s/api/v1/namespaces/default/pods/"
#define ADDR_STR_V2_TEMPLATE "https://%s/apis/apps/v1/namespaces/default/"

#define PATCH_TEMPLATE "-X PATCH -d {\\\"metadata\\\":{\\\"labels\\\":{\\\"%s\\\":\\\"%s\\\"}}} -H \"Content-Type: application/merge-patch+json\""
#define PATCH_NULL_TEMPLATE "-X PATCH -d {\\\"metadata\\\":{\\\"labels\\\":{\\\"%s\\\":null}}} -H \"Content-Type: application/merge-patch+json\""
#define AUTHORIZATION_TEMPLATE "curl -s -H \"Authorization: Bearer `cat /var/run/secrets/kubernetes.io/serviceaccount/token`\" --cacert /var/run/secrets/kubernetes.io/serviceaccount/ca.crt %s%s %s"

#define KVS_NAME_TEMPLATE_I "%s_%zu"
#define KVS_NAME_KEY_TEMPLATE "%s-%s"
#define GREP_TEMPLATE "| grep \"%s\""
#define GREP_COUNT_TEMPLATE "| grep -c \"%s\""
#define CONCAT_TWO_COMMAND_TEMPLATE "%s %s"
#define SIZE_T_TEMPLATE "%zu"


#define KVS_NAME "CCL_POD_ADDR"
#define KVS_BARRIER "CCL_BARRIER"

#define KVS_IDX "IDX"
#define KVS_UP "CCL_UP"
#define KVS_POD_REQUEST "CCL_POD_REQUEST"
#define KVS_POD_NUM "CCL_POD_NUM"
#define KVS_NEW_POD "CCL_NEW_POD"
#define KVS_DEAD_POD "CCL_DEAD_POD"
#define KVS_APPROVED_NEW_POD "CCL_APPROVED_NEW_POD"
#define KVS_APPROVED_DEAD_POD "CCL_APPROVED_DEAD_POD"
#define KVS_LISTENER "CCL_LISTENER"
#define KVS_ACCEPT "CCL_ACCEPT"

#define CCL_KVS_IP "CCL_KVS_IP"
#define CCL_KVS_PORT "CCL_KVS_PORT"
#define MASTER_ADDR "CCL_MASTER"
#define REQ_KVS_IP "CCL_REQ_KVS_IP"
#define KVS_IP "KVS_IP"
#define KVS_PORT "KVS_PORT"

#define CCL_IP_LEN 128

#define CHECKER_IP "hostname -I"
#define GET_KEY "| sed -r 's/\"[a-zA-Z0-9_]*-|: \"[a-zA-Z0-9_-]*|,|\"| |//g'"
#define GET_VAL "| sed -r 's/[a-zA-Z0-9_-]*\":|,|\"| |//g'"
#define READ_ONLY "r"
#define NULL_CHAR '\0'
#define MAX_UP_IDX 2048
#define INITIAL_UPDATE_IDX "0"
#define INITIAL_RANK_NUM "0"
#define MAX_CLEAN_CHECKS 3

extern char my_hostname[MAX_KVS_VAL_LENGTH];

#endif
