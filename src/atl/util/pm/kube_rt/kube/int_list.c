#include <stdio.h>
#include <stdlib.h>

#include "int_list.h"

void int_list_sort(int_list_t* list)
{
    int_list_t* left = list;
    int_list_t* right;

    while(left != NULL)
    {
        right = left->next;
        while(right != NULL)
        {
            if (left->i > right->i)
            {
                size_t tmp_i = left->i;
                left->i = right->i;
                right->i = tmp_i;
            }
            right = right->next;
        }
        left = left->next;
    }
}

void int_list_clean(int_list_t** list)
{
    int_list_t* cur_list = *list;
    int_list_t* node_to_remove;

    while (cur_list != NULL)
    {
        node_to_remove = cur_list;
        cur_list = cur_list->next;
        free(node_to_remove);
    }
    *list = NULL;
}

size_t int_list_is_contained(int_list_t* list, size_t i)
{
    int_list_t* cur_list = list;

    while (cur_list != NULL)
    {
        if (cur_list->i == i)
            return 1;
        cur_list = cur_list->next;
    }
    return 0;
}

void int_list_keep_first_n(int_list_t** origin_list, size_t n)
{
    int_list_t* cur_node = (*origin_list);
    int_list_t* tmp_node = NULL;
    size_t i;

    for (i = 0; i < n; i++)
    {
        tmp_node = cur_node;
        cur_node = cur_node->next;
    }

    if (tmp_node != NULL)
        tmp_node->next = NULL;

    while (cur_node != NULL)
    {
        tmp_node = cur_node;
        cur_node = cur_node->next;
        free(tmp_node);
    }
    if (n == 0)
        (*origin_list) = NULL;
}

void int_list_add(int_list_t** origin_list, size_t val)
{
    if ((*origin_list) == NULL)
    {
        (*origin_list) = (int_list_t*)malloc(sizeof(int_list_t));
        (*origin_list)->next = NULL;
        (*origin_list)->i = val;
    }
    else
    {
        int_list_t* cur_list;
        cur_list = (*origin_list);
        while (cur_list->next != NULL)
            cur_list = cur_list->next;
        cur_list->next = (int_list_t*)malloc(sizeof(int_list_t));
        cur_list = cur_list->next;
        cur_list->next = NULL;
        cur_list->i = val;
    }
}