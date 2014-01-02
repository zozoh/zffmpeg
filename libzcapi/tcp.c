/*
 * tcp.c
 *
 *  Created on: Dec 26, 2013
 *      Author: zozoh
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "log.h"
#include "tcp.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

//----------------------------------------------------------------------
#define _TCP_MSG(key,args...)\
    if (Z_BON(ctx->msg,key)) \
    { \
        _I(args); \
    }
//----------------------------------------------------------------------
#define _TCP_CHECK_CALLBACK\
    if ( Z_TCP_QUIT == callback_re)\
    {\
        ctx->quit = TRUE;\
        return FALSE;\
    }\
    else if (Z_TCP_CLOSE == callback_re)\
    {\
        return FALSE;\
    }
//----------------------------------------------------------------------
z_tcp_context *z_tcp_alloc_context(uint32_t buf_recv_size)
{
    z_tcp_context *ctx = malloc(sizeof(z_tcp_context));
    memset(ctx, 0, sizeof(z_tcp_context));
    ctx->buf_recv = malloc(buf_recv_size);
    ctx->buf_recv_size = buf_recv_size;
    memset(ctx->buf_recv, 0, buf_recv_size);
    return ctx;
}
//----------------------------------------------------------------------
void z_tcp_free_context(z_tcp_context **ctx)
{
    uint8_t *buf_recv = (*ctx)->buf_recv;
    if (NULL != buf_recv)
    {
        free(buf_recv);
    }
    free(*ctx);
    *ctx = NULL;
}
//----------------------------------------------------------------------
BOOL _exec(z_tcp_context *ctx)
{
    int callback_re;
    // 发送数据
    if (NULL != ctx->on_send)
    {
        uint32_t bytes_sent = 0;
        int data_size = 0;
        void *data = NULL;
        _TCP_MSG(Z_TCP_MSG_BF_ONSEND, "tcps:: <on_send>");
        callback_re = ctx->on_send(&data_size, &data, ctx);
        _TCP_MSG(Z_TCP_MSG_AF_ONSEND,
                 "tcps:: </on_send re=%d data_size=%d>",
                 callback_re,
                 data_size);
        // 发送全部数据
        while (bytes_sent < data_size)
        {
            _TCP_MSG(Z_TCP_MSG_BF_SEND,
                     "tcps:: send >>> %u/%u bytes",
                     bytes_sent,
                     data_size);
            bytes_sent += send(ctx->sid, data, data_size, 0);
            _TCP_MSG(Z_TCP_MSG_AF_SEND,
                     "tcps:: <<< send %u/%u bytes",
                     bytes_sent,
                     data_size);

        }
        // 处理返回值
        _TCP_CHECK_CALLBACK;
    }
    // 接受数据
    if (NULL != ctx->on_recv)
    {
        _TCP_MSG(Z_TCP_MSG_BF_RECV, "tcps:: recv <<< ");
        socklen_t recv_len = recv(ctx->sid,
                                  ctx->buf_recv,
                                  ctx->buf_recv_size,
                                  0);
        _TCP_MSG(Z_TCP_MSG_AF_RECV, "tcps:: >>> recv %d bytes", recv_len);

        // 收到数据则调用回调
        if (recv_len > 0)
        {
            _TCP_MSG(Z_TCP_MSG_BF_ONRECV,
                     "tcps:: <on_recv recv_len=%dbytes>",
                     recv_len);
            callback_re = ctx->on_recv(recv_len, ctx->buf_recv, ctx);
            _TCP_MSG(Z_TCP_MSG_AF_ONRECV,
                     "tcps:: </on_recv re=%d>",
                     callback_re);
            _TCP_CHECK_CALLBACK;
        }else{
            return FALSE;
        }
    }
    return TRUE;
}
//----------------------------------------------------------------------
void z_tcp_start_connect(z_tcp_context *ctx)
{
    // 服务器地址
    struct sockaddr_in s_add;
    socklen_t sin_size = sizeof(struct sockaddr_in);

    _I("tcps:: connect to %s:%d", ctx->host_ipv4, ctx->port);

    // 建立 socket 句柄
    ctx->sid = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == ctx->sid)
    {
        _F("tcps:: !!! socket fail : %d ", ctx->sid);
        return;
    }
    _I("tcps:: socket OK %d", ctx->sid);

    // 填充服务器端口地址信息，以便下面使用此地址和端口监听
    bzero(&s_add, sizeof(struct sockaddr_in));
    s_add.sin_family = AF_INET;
    inet_pton(AF_INET, ctx->host_ipv4, &s_add.sin_addr);
    s_add.sin_port = htons(ctx->port);

    // 启动循环，持续监听
    while (!ctx->quit)
    {
        // 接受客户端发来的请求
        if (connect(ctx->sid, &s_add, sin_size) == -1)
        {
            _W("tcps:: !! fail to connnect to %s : sid=%d",
               ctx->host_ipv4,
               ctx->sid);
            continue;
        }
        // 连接后日志
        _TCP_MSG(Z_TCP_MSG_AF_CONNECT, "tcps:: connection built ^_^")

        // 不断的收取数据，并执行回调
        while (!ctx->quit)
        {
            if (!_exec(ctx)) break;
        }
        // 关闭前日志
        _TCP_MSG(Z_TCP_MSG_BF_CLOSE, "tcps:: will close connection")
        // 执行关闭
        shutdown(ctx->sid, SHUT_WR);
        // 关闭后日志
        _TCP_MSG(Z_TCP_MSG_AF_CLOSE, "tcps:: connection closed")
    }
    // 关闭 Socket
    close(ctx->sid);
}
//----------------------------------------------------------------------
void z_tcp_start_listen(z_tcp_context *ctx)
{
    // socket 的描述符
    int so;

    // 服务器地址
    struct sockaddr_in s_add;
    socklen_t sin_size = sizeof(struct sockaddr_in);

    // 客户端地址
    struct sockaddr_in c_add;

    memset(&c_add, 0, sizeof(struct sockaddr));
    _I("tcps:: Hello,welcome to access %s ^_^", ctx->app_name);

    // 建立 socket 句柄
    so = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == so)
    {
        _F("tcps:: !!! socket fail : %d ", so);
        return;
    }
    _I("tcps:: socket OK %d", so);

    // 填充服务器端口地址信息，以便下面使用此地址和端口监听
    bzero(&s_add, sizeof(struct sockaddr_in));
    s_add.sin_family = AF_INET;
    s_add.sin_addr.s_addr = htonl(INADDR_ANY); /* 这里地址使用全0，即所有 */
    s_add.sin_port = htons(ctx->port);

    // 重用 socket
    int opt = TCP_NODELAY;
    if (setsockopt(so, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        _F("tcps:: !!! setsockopt fail");
        return;
    }
    _I("tcps:: setsockopt OK %d", so);

    // 使用bind进行绑定端口
    if (-1 == bind(so, (struct sockaddr *) (&s_add), sizeof(struct sockaddr)))
    {
        _F("tcps:: !!! bind @ %u fail, maybe be used.", ctx->port);
        close(so);
        return;
    }
    _I("tcps:: bind @ %u OK", ctx->port);

    // 开始监听相应的端口
    if (-1 == listen(so, 5))
    {
        _F("tcps:: !!! listen @ %u fail, maybe be used.", ctx->port);
        close(so);
        exit(0);
    }
    _I("tcps:: start listen");

    // 启动循环，持续监听
    char c_add_str[INET_ADDRSTRLEN];
    while (!ctx->quit)
    {
        // 接受客户端发来的请求
        ctx->sid = accept(so, &c_add, &sin_size);
        if (ctx->sid < 0)
        {
            _W("tcps:: !! fail to accept() : conn= %d", ctx->sid);
            continue;
        }
        // 连接后日志
        _TCP_MSG(Z_TCP_MSG_AF_ACCEPT,
                 "tcps:: accept from %s",
                 inet_ntop(AF_INET, &(c_add.sin_addr), c_add_str, INET_ADDRSTRLEN))

        // 不断的收取数据，并执行回调
        while (!ctx->quit)
        {
            if (!_exec(ctx)) break;
        }
        // 关闭前日志
        _TCP_MSG(Z_TCP_MSG_BF_CLOSE, "tcps:: will close connection")
        // 执行关闭
        close(ctx->sid);
        // 关闭后日志
        _TCP_MSG(Z_TCP_MSG_AF_CLOSE, "tcps:: connection closed")
    }
    // 关闭 Socket
    close(so);
}
//----------------------------------------------------------------------
#undef _TCP_CHECK_CALLBACK
#undef _TCP_MSG
//----------------------------------------------------------------------
