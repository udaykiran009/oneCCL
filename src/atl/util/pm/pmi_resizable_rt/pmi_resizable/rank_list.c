#include <stdio.h>
#include <stdlib.h>

#include "rank_list.h"

void rank_list_sort(rank_list_t* list) {
    rank_list_t* left = list;
    rank_list_t* right;

    while (left != NULL) {
        right = left->next;
        while (right != NULL) {
            if (left->rank > right->rank) {
                int tmp_i = left->rank;
                left->rank = right->rank;
                right->rank = tmp_i;
            }
            right = right->next;
        }
        left = left->next;
    }
}

void rank_list_clean(rank_list_t** list) {
    rank_list_t* cur_list = *list;
    rank_list_t* node_to_remove;

    while (cur_list != NULL) {
        node_to_remove = cur_list;
        cur_list = cur_list->next;
        free(node_to_remove);
    }
    *list = NULL;
}

size_t rank_list_contains(rank_list_t* list, int rank) {
    rank_list_t* cur_list = list;

    while (cur_list != NULL) {
        if (cur_list->rank == rank)
            return 1;
        cur_list = cur_list->next;
    }
    return 0;
}

void rank_list_keep_first_n(rank_list_t** origin_list, size_t n) {
    rank_list_t* cur_node = (*origin_list);
    rank_list_t* tmp_node = NULL;
    size_t i;

    for (i = 0; i < n; i++) {
        tmp_node = cur_node;
        cur_node = cur_node->next;
    }

    if (tmp_node != NULL)
        tmp_node->next = NULL;

    while (cur_node != NULL) {
        tmp_node = cur_node;
        cur_node = cur_node->next;
        free(tmp_node);
    }
    if (n == 0)
        (*origin_list) = NULL;
}

void rank_list_add(rank_list_t** origin_list, int rank) {
    if ((*origin_list) == NULL) {
        (*origin_list) = (rank_list_t*)malloc(sizeof(rank_list_t));
        (*origin_list)->next = NULL;
        (*origin_list)->rank = rank;
    }
    else {
        rank_list_t* cur_list;
        cur_list = (*origin_list);
        while (cur_list->next != NULL)
            cur_list = cur_list->next;
        cur_list->next = (rank_list_t*)malloc(sizeof(rank_list_t));
        cur_list = cur_list->next;
        cur_list->next = NULL;
        cur_list->rank = rank;
    }
}