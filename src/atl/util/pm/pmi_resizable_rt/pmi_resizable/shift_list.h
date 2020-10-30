#ifndef SHIFT_LIST_H_INCLUDED
#define SHIFT_LIST_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif
typedef enum change_type {
    CH_T_SHIFT = 0,
    CH_T_DEAD = 1,
    CH_T_NEW = 2,
    CH_T_UPDATE = 3,
} change_type_t;

typedef struct shift_rank {
    int old_rank;
    int new_rank;
    change_type_t type;
} shift_rank_t;

typedef struct shift_list {
    shift_rank_t shift;
    struct shift_list* next;
} shift_list_t;

void shift_list_clean(shift_list_t** list);

void shift_list_add(shift_list_t** list, int old_rank, int new_rank, change_type_t type);

#ifdef __cplusplus
}
#endif
#endif
