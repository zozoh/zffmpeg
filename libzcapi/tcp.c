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
void z_tcp_listen(z_tcp_listen_ctx *ctx)
{
    // 分配缓冲区
    char buf_recv[ctx->buf_in_size];

    // 设置一个socket地址结构server_addr,代表服务器internet地址, 端口
    int sfp, nfp; /* 定义两个描述符 */

    // 服务器地址
    struct sockaddr_in s_add;
    socklen_t sin_size = sizeof(struct sockaddr_in);

    // 客户端地址
    struct sockaddr_in c_add;

    memset(&c_add, 0, sizeof(struct sockaddr));
    _I("tcps:: Hello,welcome to access %s ^_^", ctx->server_name);

    // 建立 socket 句柄
    sfp = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sfp)
    {
        _F("tcps:: !!! socket fail : %d ", sfp);
        return;
    }
    _I("tcps:: socket OK %d", sfp);

    // 填充服务器端口地址信息，以便下面使用此地址和端口监听
    bzero(&s_add, sizeof(struct sockaddr_in));
    s_add.sin_family = AF_INET;
    s_add.sin_addr.s_addr = htonl(INADDR_ANY); /* 这里地址使用全0，即所有 */
    s_add.sin_port = htons(ctx->port);

    // 重用 socket
    int opt = TCP_NODELAY;
    if (setsockopt(sfp, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        _F("tcps:: !!! setsockopt fail");
        return;
    }
    _I("tcps:: setsockopt OK %d", sfp);

    // 使用bind进行绑定端口
    if (-1 == bind(sfp, (struct sockaddr *) (&s_add), sizeof(struct sockaddr)))
    {
        _F("tcps:: !!! bind @ %u fail, maybe be used.", ctx->port);
        close(sfp);
        return;
    }
    _I("tcps:: bind @ %u OK", ctx->port);

    // 开始监听相应的端口
    if (-1 == listen(sfp, 5))
    {
        _F("tcps:: !!! listen @ %u fail, maybe be used.", ctx->port);
        close(sfp);
        exit(0);
    }
    _I("tcps:: start listen");

    // 启动循环，持续监听
    char c_add_str[INET_ADDRSTRLEN];
    while (!ctx->quit)
    {
        // 接受客户端发来的请求
        nfp = accept(sfp, &c_add, &sin_size);
        if (nfp < 0)
        {
            _W("tcps:: !! fail to accept() : conn= %d", nfp);
            continue;
        }
        // 连接后日志
        _TCP_MSG(Z_TCP_SMSG_AF_ACCEPT,
                 "tcps:: accept from %s",
                 inet_ntop(AF_INET, &(c_add.sin_addr), c_add_str, INET_ADDRSTRLEN))

        // 不断的收取数据，并执行回调
        int callback_re;
        while (!ctx->quit)
        {
            // 接受数据
            _TCP_MSG(Z_TCP_SMSG_BF_RECV, "tcps:: before recv ...");
            socklen_t recv_len = recv(nfp, buf_recv, ctx->buf_in_size, 0);
            _TCP_MSG(Z_TCP_SMSG_AF_RECV, "tcps:: recv << %d bytes", recv_len);

            // 木有收到数据，退出，不过不知道这个逻辑有木有必要 ...
            if (recv_len == 0)
            {
                _I("tcps:: no more data, close it!");
                break;
            }

            // 调用回调
            _TCP_MSG(Z_TCP_SMSG_BF_RECV, "tcps:: ->callback(%ubytes)", recv_len);
            callback_re = ctx->callback(recv_len, buf_recv, ctx->userdata);
            if ( Z_TCP_QUIT == callback_re)
            {
                ctx->quit = TRUE;
                break;
            }
            else if (Z_TCP_CLOSE == callback_re)
            {
                break;
            }
            _TCP_MSG(Z_TCP_SMSG_BF_RECV, "tcps:: callback<-");
        }
        // 关闭前日志
        _TCP_MSG(Z_TCP_SMSG_BF_CLOSE, "tcps:: will close connection")
        // 执行关闭
        close(nfp);
        // 关闭后日志
        _TCP_MSG(Z_TCP_SMSG_AF_CLOSE, "tcps:: connection closed")
    }
    // 关闭 Socket
    close(sfp);
}
#undef _TCP_MSG
//----------------------------------------------------------------------
