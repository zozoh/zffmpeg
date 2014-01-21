/*
 * tld.h
 *
 *  Created on: Jan 3, 2014
 *      Author: zozoh
 */

#ifndef TLD_H_
#define TLD_H_
//------------------------------------------------------------
#include <stdint.h>
#include "z.h"
//------------------------------------------------------------
#define TLD_HEAD_SIZE 6
//------------------------------------------------------------
/**
 *  一个 TLD 的结构
 */
typedef struct
{
    /**
     * TLD 的 TAG 部分
     */
    uint8_t tag;
    /**
     * TLD 的数据部分应该的长度
     */
    uint32_t len;

    /**
     * TLD 头部的 6 个字节
     */
    uint8_t head[6];
    /**
     * 标识了一个 TLD 头部的真正被读取的大小，如果不等于 TLD_HEAD_SIZE
     * 则表示头部还不完整
     */
    int head_size;
    /**
     * 指向一个 TLD 的 'D' 部分的指针
     */
    uint8_t *data;
    /**
     * 一个 TLD 内部有效数据的长度，如果与 len 相等表示这个 TLD 圆满了
     */
    int data_size;

} TLD;
//------------------------------------------------------------
extern TLD *tld_alloc();
extern void tld_free(TLD *tld);
extern void tld_freep(TLD **tld);
//------------------------------------------------------------
extern BOOL tld_head_finished(TLD *tld);
extern BOOL tld_data_finished(TLD *tld);
//------------------------------------------------------------
extern void tld_brief_print(TLD *tld);
extern void tld_brief_print_data(uint8_t *data, int data_size);
//------------------------------------------------------------
/**
 * 从一块指向 TLD 内存的指针中 copy TLD 的头部 6 个字节到 head 段，
 * 返回真正读取的字节数
 */
extern int tld_copy_head(TLD *tld, uint8_t *pHead, int in_size);
/**
 * 如果 TLD 的头部完整，本函数将会根据 head 解析 tag 和 len 段的值，
 * 如果不完整，返回 FALSE
 */
extern BOOL tld_parse_head(TLD *tld);
/**
 * 从一块数据流读取数据到 tld 的 data 部分，会根据流的数据大小，TLD 内部状态，
 * 自动判断应该 copy 多少数据过来。 返回 copy 的字节数
 * 如果你非常自信，你给的 pData 肯定包括一个完整的 TLD，那么 in_size你可以传入<=0
 * 则会读取 tld.len - tld.data_size 个字节
 */
extern int tld_copy_data(TLD *tld, uint8_t *pData, int in_size);
//------------------------------------------------------------
#endif /* TLD_H_ */
