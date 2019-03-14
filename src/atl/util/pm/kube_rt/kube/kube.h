#ifndef KUBE_H_INCLUDED
#define KUBE_H_INCLUDED

#include <stdlib.h>

#define KUBE_SUCCESS                  0
#define KUBE_FAIL                    -1
#define KUBE_ERR_INIT                 1
#define KUBE_ERR_NOMEM                2
#define KUBE_ERR_INVALID_ARG          3
#define KUBE_ERR_INVALID_KEY          4
#define KUBE_ERR_INVALID_KEY_LENGTH   5
#define KUBE_ERR_INVALID_VAL          6
#define KUBE_ERR_INVALID_VAL_LENGTH   7
#define KUBE_ERR_INVALID_LENGTH       8
#define KUBE_ERR_INVALID_NUM_ARGS     9
#define KUBE_ERR_INVALID_ARGS        10
#define KUBE_ERR_INVALID_NUM_PARSED  11
#define KUBE_ERR_INVALID_KEYVALP     12
#define KUBE_ERR_INVALID_SIZE        13

enum framework_answers;
typedef enum framework_answers (*update_checker__f)(size_t comm_size);

int KUBE_Init(void);

int KUBE_Finalize(void);

int KUBE_Extreme_finalize(void);

int KUBE_Get_size(size_t *size);

int KUBE_Get_rank(size_t *rank);

int KUBE_KVS_Get_my_name(char kvsname[], size_t length);

int KUBE_KVS_Get_name_length_max(size_t *length);

int KUBE_Barrier( void );

int KUBE_Update( void );

int KUBE_Get_shift_count(size_t* count);

int KUBE_KVS_Get_key_length_max(size_t *length);

int KUBE_KVS_Get_value_length_max(size_t *length);

int KUBE_KVS_Put(const char kvsname[], const char key[], const char value[]);

int KUBE_KVS_Commit(const char kvsname[]);

int KUBE_KVS_Get(const char kvsname[], const char key[], char value[], size_t length);

int KUBE_set_framework_function(update_checker__f user_checker);

#endif
