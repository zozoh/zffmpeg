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
 * 描述了 TCP 连接的上下文，可以用在监听或者发起函数
 */
typedef struct z_tcp_context
{
    /**
     * 指定一个监听的名称，这个变量仅仅是为了日志等显示的友好
     * - client: unused
     * - server: Set by user.
     */
    const char *app_name;

    /**
     * 要连接的目标 IPv4 地址
     * - client: Set by user
     * - server: unused
     */
    const char *host_ipv4;

    /**
     * 要监听/连接的端口
     * - client: Set by user.
     * - server: Set by user.
     */
    int port;

    /**
     * 打开的 Socket 资源 ID，默认为 -1 表示未打开
     * - client: Set by z_tcp :: connect
     * - server: Set by z_tcp :: accept
     */
    int sid;

    /**
     * 一个接受数据的缓冲，由 z_tcp 根据 buf_in_size 来分配内存。
     * 如果你自己手工 malloc 了这块内存，请指定正确的 buf_in_size
     *
     * 在函数 z_tcp_free_context 会释放这块内存,所以如果你不打算让
     * z_tcp_free_context 的释放 buf_recv，那么调用之前，将其置 NULL即可
     *
     * - client: Set by z_tcp
     * - server: Set by z_tcp
     */
    uint8_t *buf_recv;

    /**
     * 输入缓冲区大小
     * - client: Set by user.
     * - server: Set by user.
     */
    uint32_t buf_recv_size;

    /**
     * 标识当前监听是否要退出，如果为 0 表示继续监听，其他值表示要立即退出监听函数
     * - client: Set by user.
     * - server: Set by user.
     */
    int quit;

    /**
     * 标识当前监听是否要退出，如果为 0 表示继续监听，其他值表示要立即退出监听函数
     * - client: Set by user.
     * - server: Set by user.
     */
    uint32_t msg;
#define Z_TCP_MSG_AF_ACCEPT  1
#define Z_TCP_MSG_BF_RECV    2
#define Z_TCP_MSG_AF_RECV    4
#define Z_TCP_MSG_BF_CLOSE   8
#define Z_TCP_MSG_AF_CLOSE   16
#define Z_TCP_MSG_BF_ONRECV  32
#define Z_TCP_MSG_AF_ONRECV  64
#define Z_TCP_MSG_BF_SEND    128
#define Z_TCP_MSG_AF_SEND    256
#define Z_TCP_MSG_BF_ONSEND  512
#define Z_TCP_MSG_AF_ONSEND  1024
#define Z_TCP_MSG_AF_CONNECT 2048
#define Z_TCP_MSG_DUMP_DATA_SEND 4096
#define Z_TCP_MSG_DUMP_DATA_RECV 8192

    // 当收到数据， z_tcp 会调用你给的这个回调函数，告诉你收到了多少字节，以及一个数据的起始指针。
    // 你需要做的就是，尽快处理，比如将这块内存 copy 出来，返回迅速返回。
    // [in] rsz  : 收到的字节数
    // [in] data : 数据指针（你不可以释放它，z_tcp 会自行管理这块内存区域）
    // [in] ctx  : 这个上下文对象本身
    // 本函数返回参见下面的宏定义
    int (*on_recv)(int rsz, void *data, struct z_tcp_context *ctx);
#define Z_TCP_QUIT -1
#define Z_TCP_CLOSE 0
#define Z_TCP_CONTINUE 1
    // z_tcp 会定期询问你是否需要发送数据。
    // 询问的方式就是调用这个回调，你在回调函数里返回一块数据的指针
    // [in] size : 要发送的字节数
    // [in] data : 数据指针你传递给 z_tcp 的一块数据，你自己负责分配内存和释放，z_tcp 只管传输
    // [in] ctx  : 这个上下文对象本身
    // 本函数返回参见上面的宏定义
    int (*on_send)(int *size, void **data, struct z_tcp_context *ctx);

    // 一个上下文指针，传回给定的回调函数之用
    void *userdata;
} z_tcp_context;

/**
 * 初始化一块内存给 z_tcp_context
 */
extern z_tcp_context *z_tcp_alloc_context(uint32_t buf_recv_size);
/**
 * 释放 z_tcp_context 的内存，也包括其内部分配的缓存等，并把传入的指针置成 NULL
 */
extern void z_tcp_free_context(z_tcp_context **ctx);

/**
 * 在当前线程启动一个监听循环
 */
extern void z_tcp_start_listen(z_tcp_context *ctx);

/**
 * 在当前线程启动一个发送循环
 */
extern void z_tcp_start_connect(z_tcp_context *ctx);

#endif /* TCP_H_ */
