#include <stdio.h>
#include <stdlib.h>

#include "shift_list.hpp"

void shift_list_clean(shift_list_t** list) {
    shift_list_t* cur_list = (*list);
    shift_list_t* node_to_remove;
    while (cur_list != NULL) {
        node_to_remove = cur_list;
        cur_list = cur_list->next;
        free(node_to_remove);
    }
    (*list) = NULL;
}

void shift_list_add(shift_list_t** list, int old_rank, int new_rank, change_type_t type) {
    shift_list_t* cur_list;
    if ((*list) == NULL) {
        (*list) = (shift_list_t*)malloc(sizeof(shift_list_t));
        if((*list) == NULL) {
            printf("Memory allocation failed\n");
            return;
        }
        cur_list = (*list);
    }
    else {
        cur_list = (*list);
        while (cur_list->next != NULL)
            cur_list = cur_list->next;
        cur_list->next = (shift_list_t*)malloc(sizeof(shift_list_t));
        cur_list = cur_list->next;
    }
    cur_list->shift.old_rank = old_rank;
    cur_list->shift.new_rank = new_rank;
    cur_list->shift.type = type;
    cur_list->next = NULL;
}