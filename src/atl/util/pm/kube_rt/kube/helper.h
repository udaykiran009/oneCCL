#ifndef HELPER_H_INCLUDED
#define HELPER_H_INCLUDED

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>

#include "kube_def.h"
#include "int_list.h"
#include "shift_list.h"
#include "kvs_keeper.h"
#include "listener.h"


extern char run_v2_template[RUN_TEMPLATE_SIZE];

extern size_t my_rank, count_pods;
extern size_t barrier_num;
extern size_t up_idx;
extern size_t initialized;

extern int_list_t* killed_ranks;
extern size_t killed_ranks_count;

extern int_list_t* new_ranks;
extern size_t new_ranks_count;

void get_update_ranks(size_t id);

size_t get_replica_size(void);

void wait_accept(void);

size_t update(shift_list_t** list, int_list_t** dead_up_idx, size_t root_rank);

void up_pods_count(void);

void get_shift(shift_list_t** list);

void send_notification(int sig);

void reg_rank(void);

void put_key(const char kvsname[], const char kvs_key[], const char kvs_val[]);

void up_kvs_new_and_dead(size_t up_idx);

void json_get_val(FILE *fp, const char** keys, size_t keys_count, char* val);

void keep_first_n_up(size_t prev_new_ranks_count, size_t prev_killed_ranks_count);

#endif
