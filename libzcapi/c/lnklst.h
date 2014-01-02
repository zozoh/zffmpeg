/*
 * linkedlist.h
 *
 * 提供一个双向链表的通用实现，构造诸如下列结构的内存：
 *
 *                         [z_lnklst]
 *                           size
 *               +--------- *first
 *               |          *last ---------+
 *               |                         |
 *               V                         V
 *            /-----\      /-----\      /-----\
 *            |*list|      |*list|      |*list|
 *  NULL <--- |*prev| <--- |*prev| <--- |*prev|
 *            |*next| ---> |*next| ---> |*next| ---> NULL
 *            |*data|      |*data|      |*data|
 *            | size|      | size|      | size|
 *            \-----/      \-----/      \-----/
 *
 *
 * 适用于数据大小不确定，基本顺序访问的场景。本对象提供的全部方法都是线程安全的。
 * 但是对于 z_linkedlist_item 结构，请自行考虑线程安全方面的事情
 *
 *  Created on: Dec 31, 2013
 *      Author: zozoh
 */

#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_
//----------------------------------------------------------
#include <pthread.h>
#include "../z.h"
//----------------------------------------------------------
typedef struct z_lnklst
{
    /**
     * 链表的大小，初始值为 0
     */
    int size;
    /*
     * 指向链表的第一个元素，初始值为 NULL。
     * 在链表长度为 1 的时候，与 last 指向的是同一个元素
     */
    struct z_lnklst_item *first;
    /**
     * 指向链表的最后一个元素，初始值为 NULL。
     * 在链表长度为 1 的时候，与 first 指向的是同一个元素
     */
    struct z_lnklst_item *last;

    /**
     * 用户定义的用来释放列表数据的回调函数，
     * !!! 如果没声明，则在释放列表的时候，不会释放每个列表项的 data
     */
    void (*free_li)(struct z_lnklst_item *li);
    /**
     * 线程同步锁，所有对于链表的顺序操作，都应该对这个锁进行同步
     */
    pthread_mutex_t mutex;
} z_lnklst;
//----------------------------------------------------------
typedef struct z_lnklst_item
{
    /**
     * 指向自己所属的链表
     */
    struct z_lnklst *list;
    /**
     * 指向前一个元素，初始值为 NULL。
     */
    struct z_lnklst_item *prev;
    /**
     * 指向后一个元素，初始值为 NULL。
     */
    struct z_lnklst_item *next;
    /**
     * 数据对象
     */
    void *data;
    /**
     * 数据对象所占内存的大小
     */
    int size;

} z_lnklst_item;
//----------------------------------------------------------
extern z_lnklst *z_lnklst_alloc(void (*free_li)(struct z_lnklst_item *li));
extern void z_lnklst_free(z_lnklst *list);

extern z_lnklst_item *z_lnklst_alloc_item(void *data, int size);
extern void z_lnklst_free_item(z_lnklst_item *li);

extern int z_lnklst_append_prev(z_lnklst_item *li, z_lnklst_item *new_li);
extern int z_lnklst_append_next(z_lnklst_item *li, z_lnklst_item *new_li);

extern int z_lnklst_add_first(z_lnklst *list, z_lnklst_item *new_li);
extern int z_lnklst_add_last(z_lnklst *list, z_lnklst_item *new_li);

extern BOOL z_lnklst_pop_first(z_lnklst *list, void *data, int *size);
extern BOOL z_lnklst_pop_last(z_lnklst *list, void *data, int *size);

extern BOOL z_lnklst_peek_first(z_lnklst *list, void *data, int *size);
extern BOOL z_lnklst_peek_last(z_lnklst *list, void *data, int *size);

extern int z_lnklst_clear(z_lnklst *list);
extern void z_lnklst_remove(z_lnklst_item *li);

extern BOOL z_lnklst_is_empty(z_lnklst *list);
//----------------------------------------------------------
#endif /* LINKEDLIST_H_ */
