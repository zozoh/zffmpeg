/*
 * zplay.h
 *
 *  Created on: Dec 26, 2013
 *      Author: zozoh
 */

#ifndef ZPLAY_H_
#define ZPLAY_H_

#include <stdint.h>
#include <libzcapi/tld.h>
#include <libzcapi/c/lnklst.h>

/**
 * 定义了  zPlay 接受的参数
 */
typedef struct
{
    int port;
} ZPlayArgs;

/**
 *  接受 TLD 的上下文
 */
typedef struct
{
    // 当前正在操作的 TLD，如果接受满了，就推入列表
    TLD *tld;

    // 存放一个个完整的 TLD
    z_lnklst *tlds;

} TLDReceving;

/**
 * 一个 TLD 消费线程的上下文
 */
typedef struct
{
    z_lnklst *tlds;
    struct SwsContext *swsc;
    struct AVCodecContext *cc;
} TLDDecoding;

typedef int (action_func)(AVCodecContext *c, void *arg);
typedef int (action_func2)(AVCodecContext *c, void *arg, int jobnr, int threadnr);

typedef struct ThreadContext {
    pthread_t *workers;
    action_func *func;
    action_func2 *func2;
    void *args;
    int *rets;
    int rets_count;
    int job_count;
    int job_size;

    pthread_cond_t last_job_cond;
    pthread_cond_t current_job_cond;
    pthread_mutex_t current_job_lock;
    int current_job;
    unsigned int current_execute;
    int done;
} ThreadContext;

#endif /* ZPLAY_H_ */
