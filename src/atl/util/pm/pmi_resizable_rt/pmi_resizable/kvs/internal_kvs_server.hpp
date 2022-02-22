#pragma once
#include "util/pm/pmi_resizable_rt/pmi_resizable/def.h"
#include "internal_kvs.h"

typedef enum kvs_access_mode {
    AM_PUT = 2,
    AM_REMOVE = 3,
    AM_GET_COUNT = 4,
    AM_GET_VAL = 5,
    AM_GET_KEYS_VALUES = 6,
    AM_GET_REPLICA = 7,
    AM_FINALIZE = 8,
    AM_BARRIER = 9,
    AM_BARRIER_REGISTER = 10,
    AM_INTERNAL_REGISTER = 11,
    AM_SET_SIZE = 12,
} kvs_access_mode_t;

typedef struct kvs_request {
    kvs_access_mode_t mode{ AM_PUT };
    char name[MAX_KVS_NAME_LENGTH]{};
    char key[MAX_KVS_KEY_LENGTH]{};
    char val[MAX_KVS_VAL_LENGTH]{};
    kvs_request() {
        memset((void*)this, 0, sizeof(kvs_request));
    }
} kvs_request_t;

typedef struct server_args {
    int sock_listener;
    std::shared_ptr<isockaddr> args;
} server_args_t;

void* kvs_server_init(void* args);
