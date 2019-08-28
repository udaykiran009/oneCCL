#ifndef PMIR_H_INCLUDED
#define PMIR_H_INCLUDED

#include <stdlib.h>

#define PMIR_API __attribute__ ((visibility ("default")))

#define PMIR_SUCCESS                  0
#define PMIR_FAIL                    -1
#define PMIR_ERR_INIT                 1
#define PMIR_ERR_NOMEM                2
#define PMIR_ERR_INVALID_ARG          3
#define PMIR_ERR_INVALID_KEY          4
#define PMIR_ERR_INVALID_KEY_LENGTH   5
#define PMIR_ERR_INVALID_VAL          6
#define PMIR_ERR_INVALID_VAL_LENGTH   7
#define PMIR_ERR_INVALID_LENGTH       8
#define PMIR_ERR_INVALID_NUM_ARGS     9
#define PMIR_ERR_INVALID_ARGS        10
#define PMIR_ERR_INVALID_NUM_PARSED  11
#define PMIR_ERR_INVALID_KEYVALP     12
#define PMIR_ERR_INVALID_SIZE        13

enum pmir_resize_action;
typedef enum pmir_resize_action (*pmir_resize_fn_t)(size_t comm_size);

int PMIR_API PMIR_Init(void);

int PMIR_API PMIR_Finalize(void);

int PMIR_API PMIR_Get_size(size_t *size);

int PMIR_API PMIR_Get_rank(size_t *rank);

int PMIR_API PMIR_KVS_Get_my_name(char * kvs_name, size_t length);

int PMIR_API PMIR_KVS_Get_name_length_max(size_t *length);

int PMIR_API PMIR_Barrier(void );

int PMIR_API PMIR_Update(void );

int PMIR_API PMIR_KVS_Get_key_length_max(size_t *length);

int PMIR_API PMIR_KVS_Get_value_length_max(size_t *length);

int PMIR_API PMIR_KVS_Put(const char* kvs_name, const char* key, const char* value);

int PMIR_API PMIR_KVS_Commit(const char* kvs_name);

int PMIR_API PMIR_KVS_Get(const char* kvs_name, const char* key, char * value, size_t length);

int PMIR_API PMIR_set_resize_function(pmir_resize_fn_t resize_fn);

int PMIR_API PMIR_Wait_notification(void );

#endif
