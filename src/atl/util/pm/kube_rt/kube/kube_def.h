#ifndef KUBE_DEF_INCLUDED
#define KUBE_DEF_INCLUDED


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
  } while(0);

#define BARRIER_NUM_MAX      1024
#define MAX_KVS_KEY_LENGTH   64
#define MAX_KVS_VAL_LENGTH   64
#define MAX_KVS_NAME_LENGTH  64
#define MAX_KVS_NAME_KEY_LENGTH  (MAX_KVS_NAME_LENGTH + MAX_KVS_KEY_LENGTH + 1)
#define MAX_KVS_NAME_KEY_VAL_LENGTH (MAX_KVS_NAME_KEY_LENGTH + MAX_KVS_VAL_LENGTH + 1)
#define RUN_TEMPLATE_SIZE    1024
#define INT_STR_SIZE         8
#define REQUEST_POSTFIX_SIZE 1024
#define RUN_REQUEST_SIZE     2048

#define ADDR_STR_V1_TEMPLATE "https://%s/api/v1/namespaces/default/pods/"
#define ADDR_STR_V2_TEMPLATE "https://%s/apis/apps/v1/namespaces/default/"

#define KVS_TEMPLATE "\\\"%s\\\":\\\"%s\\\""
#define KVS_NULL_TEMPLATE "\\\"%s\\\":null"


#define BASE_PATCH_TEMPLATE "-X PATCH -d {\\\"metadata\\\":{\\\"labels\\\":{%s}}} -H \"Content-Type: application/merge-patch+json\""
#define PATCH_TEMPLATE "-X PATCH -d {\\\"metadata\\\":{\\\"labels\\\":{\\\"%s\\\":\\\"%s\\\"}}} -H \"Content-Type: application/merge-patch+json\""
#define PATCH_NULL_TEMPLATE "-X PATCH -d {\\\"metadata\\\":{\\\"labels\\\":{\\\"%s\\\":null}}} -H \"Content-Type: application/merge-patch+json\""
#define PATCH_TEMPLATE_I "-X PATCH -d {\\\"metadata\\\":{\\\"labels\\\":{\\\"%s\\\":\\\"%zu\\\"}}} -H \"Content-Type: application/merge-patch+json\""
#define AUTORIZATION_TEMPLATE "curl -s -H \"Authorization: Bearer `cat /var/run/secrets/kubernetes.io/serviceaccount/token`\" --cacert /var/run/secrets/kubernetes.io/serviceaccount/ca.crt %s%s %s"

#define KVS_NAME_TEMPLATE_I "%s_%zu"
#define KVS_NAME_TEMPLATE_S "%s_%s"
#define KVS_NAME_KEY_TEMPLATE "%s-%s"
#define GREP_TEMPLATE "| grep \"%s\""
#define GREP_REG_TEMPLATE "| grep -E \"%s\""
#define GREP_COUNT_TEMPLATE "| grep -c \"%s\""
#define GREP_EXCEPT_TEMPLATE "| grep -v \"%s\""
#define SED_TEMPLATE "| sed -r 's/%s%s//g'"
#define CONCAT_TWO_COMMAND_TEMPLATE "%s %s"
#define CONCAT_THREE_COMMAND_TEMPLATE "%s %s %s"
#define GET_NUM_LINE_TEMPLATE "| sed -n \"/%s/=\""
#define SIZE_T_TEMPLATE "%zu"

#define KUBE_API_ADDR_ENV "MLSL_KUBE_API_ADDR"
#define RANK_COUNT_ENV "MLSL_RANK_COUNT"
#define ASK_ONLY_FRAMEWORK_ENV "MLSL_ASK_FRAMEWORK"

#define KVS_NAME "MLSL_POD_ADDR"
#define KVS_BARRIER "MLSL_BARRIER"
#define KVS_HOSTNAME "$HOSTNAME"
#define KVS_IDX "IDX"
#define KVS_UP "MLSL_UP"
#define KVS_POD_REQUEST "MLSL_POD_REQUEST"
#define KVS_POD_NUM "MLSL_POD_NUM"
#define KVS_NEW_POD "MLSL_NEW_POD"
#define KVS_DEAD_POD "MLSL_DEAD_POD"
#define KVS_LISTNER "MLSL_LISTNER"
#define KVS_ACCEPT "MLSL_ACCEPT"

#define CHECKER_IP "`hostname -i`"
#define SORT "| sort -n"

#define CLEANER "[a-zA-Z0-9_-]*\":|,|\"| |"
#define GET_NAME_KEY "| sed -r 's/: \"[a-zA-Z0-9_-]*|,|\"| |//g'"
#define GET_KEY "| sed -r 's/\"[a-zA-Z0-9_]*-|: \"[a-zA-Z0-9_-]*|,|\"| |//g'"
#define GET_VAL "| sed -r 's/[a-zA-Z0-9_-]*\":|,|\"| |//g'"
#define READ_ONLY "r"
#define NULL_CHAR '\0'
#define MAX_UP_IDX 2048
#define INITIAL_BARRIER_IDX "1"
#define INITIAL_UPDATE_IDX "0"
#define INITIAL_RANK_NUM "0"
#define MAX_CLEAN_CHECKS 2

#endif
