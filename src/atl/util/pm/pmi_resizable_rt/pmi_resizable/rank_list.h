#ifndef INT_LIST_H_INCLUDED
#define INT_LIST_H_INCLUDED

typedef struct rank_list {
    size_t rank;
    struct rank_list* next;
} rank_list_t;

size_t rank_list_contains(rank_list_t* list, size_t rank);

void rank_list_clean(rank_list_t** list);

void rank_list_sort(rank_list_t* list);

void rank_list_keep_first_n(rank_list_t** origin_list, size_t n);

void rank_list_add(rank_list_t** origin_list, size_t rank);

#endif
