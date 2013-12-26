/*
 * tcp.h
 *
 * 定义了一组 TCP 相关的监听服务函数，以及请求发起函数
 *
 *  Created on: Dec 26, 2013
 *      Author: zozoh
 */

#ifndef TCP_H_
#define TCP_H_

#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>

#include "z.h"

/**
 * 描述了启动监听循环的上下文
 */
typedef struct z_tcp_listen_ctx
{
    // 指定一个监听的名称，这个变量仅仅是为了日志等显示的友好
    const char *server_name;

    // 要监听的端口
    uint16_t port;

    // 分配的缓冲区大小
    uint32_t buf_in_size;

    // 标识当前监听是否要退出，如果为 0 表示继续监听，其他值表示要立即退出监听函数
    int quit;

    // 定义一个32位码，表示服务器运行期间日志的显示开关
    uint32_t msg;
#define Z_TCP_SMSG_AF_ACCEPT  1
#define Z_TCP_SMSG_BF_RECV    2
#define Z_TCP_SMSG_AF_RECV    4
#define Z_TCP_SMSG_BF_CLOSE   8
#define Z_TCP_SMSG_AF_CLOSE   16
#define Z_TCP_SMSG_BF_CALLBACK   32
#define Z_TCP_SMSG_AF_CALLBACK   64

    // 当收到数据后的回调
    // 本函数返回参加下面的宏定义
    int (*callback)(uint32_t rsz, void *data, void *userdata);
#define Z_TCP_QUIT -1
#define Z_TCP_CLOSE 0
#define Z_TCP_CONTINUE 1


    // 一个上下文指针，传回给定的回调函数之用
    void *userdata;
} z_tcp_listen_ctx;

/**
 * 在当前线程启动一个监听循环
 */
extern void z_tcp_listen(z_tcp_listen_ctx *ctx);

#endif /* TCP_H_ */
