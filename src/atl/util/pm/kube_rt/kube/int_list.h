#ifndef INT_LIST_H_INCLUDED
#define INT_LIST_H_INCLUDED

typedef struct int_list {
    size_t i;
    struct int_list* next;
} int_list_t;

size_t int_list_is_contained(int_list_t* list, size_t i);

void int_list_clean(int_list_t** list);

void int_list_sort(int_list_t* list);

void int_list_keep_first_n(int_list_t** origin_list, size_t n);

void int_list_add(int_list_t** origin_list, size_t val);

#endif
