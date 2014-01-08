/*
 * lnklst.c
 *
 *  Created on: Dec 31, 2013
 *      Author: zozoh
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "lnklst.h"
//------------------------------------------------------------------
z_lnklst *z_lnklst_alloc(void (*free_li)(struct z_lnklst_item *li))
{
    z_lnklst *list = malloc(sizeof(z_lnklst));
    memset(list, 0, sizeof(z_lnklst));
    list->free_li = free_li;
    pthread_mutex_init(&list->mutex, NULL);
    return list;
}
//------------------------------------------------------------------
void z_lnklst_free(z_lnklst *list)
{
    z_lnklst_clear(list);
    pthread_mutex_destroy(&list->mutex);
    free(list);
}
//------------------------------------------------------------------
z_lnklst_item *z_lnklst_item_alloc(void *data)
{
    return z_lnklst_item_alloc2(data, sizeof(data));
}
//------------------------------------------------------------------
z_lnklst_item *z_lnklst_item_alloc2(void *data, int size)
{
    z_lnklst_item *li = malloc(sizeof(z_lnklst_item));
    memset(li, 0, sizeof(z_lnklst_item));
    li->data = data;
    li->size = size;
    return li;
}
//------------------------------------------------------------------
void z_lnklst_item_free(z_lnklst_item *li)
{
    if (NULL != li->data && NULL != li->list->free_li)
    {
        li->list->free_li(li);
    }
    free(li);
}
//------------------------------------------------------------------
int _append_prev(z_lnklst_item *li, z_lnklst_item *new_li)
{
    new_li->list = li->list;
    new_li->prev = li->prev;

    if (NULL != li->prev) li->prev->next = new_li;
    li->prev = new_li;
    new_li->next = li;

    if (li->list->first == li) li->list->first = new_li;
    li->list->size++;

    return li->list->size;
}
//------------------------------------------------------------------
int _append_next(z_lnklst_item *li, z_lnklst_item *new_li)
{
    new_li->list = li->list;
    new_li->next = li->next;

    if (NULL != li->next) li->next->prev = new_li;

    li->next = new_li;
    new_li->prev = li;

    if (li->list->last == li) li->list->last = new_li;
    li->list->size++;

    return li->list->size;
}
//------------------------------------------------------------------
int z_lnklst_append_prev(z_lnklst_item *li, z_lnklst_item *new_li)
{
    int re = 0;
    pthread_mutex_lock(&li->list->mutex);
    {
        re = _append_prev(li, new_li);
    }
    pthread_mutex_unlock(&li->list->mutex);
    return re;
}
//------------------------------------------------------------------
int z_lnklst_append_next(z_lnklst_item *li, z_lnklst_item *new_li)
{
    int re;
    pthread_mutex_lock(&li->list->mutex);
    {
        re = _append_next(li, new_li);
    }
    pthread_mutex_unlock(&li->list->mutex);
    return re;
}
//------------------------------------------------------------------
int z_lnklst_add_first(z_lnklst *list, z_lnklst_item *new_li)
{
    int re;
    pthread_mutex_lock(&list->mutex);
    {
        if (NULL != list->first)
        {
            re = _append_prev(list->first, new_li);
        }
        else
        {
            new_li->list = list;
            list->first = new_li;
            list->last = new_li;
            list->size = 1;
        }
    }
    pthread_mutex_unlock(&list->mutex);
    return re;
}
//------------------------------------------------------------------
int z_lnklst_add_last(z_lnklst *list, z_lnklst_item *new_li)
{
    int re;
    pthread_mutex_lock(&list->mutex);
    {
        if (NULL != list->last)
        {
            re = _append_next(list->last, new_li);
        }
        else
        {
            new_li->list = list;
            list->first = new_li;
            list->last = new_li;
            list->size = 1;
        }
    }
    pthread_mutex_unlock(&list->mutex);
    return re;
}
//------------------------------------------------------------------
BOOL z_lnklst_pop_first(z_lnklst *list, void *data, int *size)
{
    BOOL re = FALSE;
    *((void **) data) = NULL;
    *size = 0;
    pthread_mutex_lock(&list->mutex);
    {
        z_lnklst_item *li = list->first;
        if (li != NULL)
        {
            *((void **) data) = li->data;
            *size = li->size;

            if (NULL != li->next) li->next->prev = NULL;
            list->first = li->next;
            if (NULL == li->next) list->last = NULL;
            list->size--;
        }
    }
    pthread_mutex_unlock(&list->mutex);
    return re;
}
//------------------------------------------------------------------
BOOL z_lnklst_pop_last(z_lnklst *list, void *data, int *size)
{
    BOOL re = FALSE;
    *((void **) data) = NULL;
    *size = 0;
    pthread_mutex_lock(&list->mutex);
    {
        z_lnklst_item *li = list->last;
        if (li != NULL)
        {
            *((void **) data) = li->data;
            *size = li->size;

            if (NULL != li->prev) li->prev->next = NULL;
            list->last = li->prev;
            if (NULL == li->prev) list->first = NULL;
            list->size--;
        }
    }
    pthread_mutex_unlock(&list->mutex);
    return re;
}
//------------------------------------------------------------------
BOOL z_lnklst_peek_first(z_lnklst *list, void *data, int *size)
{
    BOOL re = FALSE;
    *((void **) data) = NULL;
    *size = 0;
    pthread_mutex_lock(&list->mutex);
    {
        if (list->first != NULL)
        {
            *((void **) data) = list->first->data;
            *size = list->first->size;
        }
    }
    pthread_mutex_unlock(&list->mutex);
    return re;
}
//------------------------------------------------------------------
BOOL z_lnklst_peek_last(z_lnklst *list, void *data, int *size)
{
    BOOL re = FALSE;
    *((void **) data) = NULL;
    *size = 0;
    pthread_mutex_lock(&list->mutex);
    {
        if (list->last != NULL)
        {
            *((void **) data) = list->last->data;
            *size = list->last->size;
        }
    }
    pthread_mutex_unlock(&list->mutex);
    return re;
}
//------------------------------------------------------------------
int z_lnklst_clear(z_lnklst *list)
{
    int re = list->size;
    pthread_mutex_lock(&list->mutex);
    {
        z_lnklst_item *li = list->first;
        while (NULL != li)
        {
            z_lnklst_item *next = li->next;
            z_lnklst_item_free(li);
            li = next;
        }
        list->first = NULL;
        list->last = NULL;
        list->size = 0;
    }
    pthread_mutex_unlock(&list->mutex);
    return re;
}

//------------------------------------------------------------------
void z_lnklst_remove(z_lnklst_item *li)
{
    pthread_mutex_lock(&li->list->mutex);
    {
        z_lnklst_item *prev = li->prev;
        z_lnklst_item *next = li->next;
        z_lnklst_item_free(li);
        if (NULL != prev) prev->next = next;
        if (NULL != next) next->prev = prev;
        li->list->size--;
    }
    pthread_mutex_unlock(&li->list->mutex);
}
//------------------------------------------------------------------
BOOL z_lnklst_is_empty(z_lnklst *list)
{
    BOOL re = TRUE;
    pthread_mutex_lock(&list->mutex);
    {
        re = (list->size == 0);
    }
    pthread_mutex_unlock(&list->mutex);
    return re;
}
//------------------------------------------------------------------
