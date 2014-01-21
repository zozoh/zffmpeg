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
    int port;        // -p 监听的端口
    char *ppmPath;   // -ppm 解码输出 ppm 到哪个文件夹

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
    ZPlayArgs *args;

    z_lnklst *tlds;

    struct SwsContext *swsc;
    struct AVCodecContext *cc;

    uint8_t stream_index;

    int W;
    int H;

    AVFrame *pFrame;
    AVFrame *pRGB;

} TLDDecoding;

#endif /* ZPLAY_H_ */
