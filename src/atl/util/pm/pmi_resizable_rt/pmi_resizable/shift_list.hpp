#pragma once
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
