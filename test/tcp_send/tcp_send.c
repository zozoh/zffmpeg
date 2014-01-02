/*
 * tcpsend.c
 *
 *  Created on: Dec 27, 2013
 *      Author: zozoh
 */

/*
 * tcp_listen.c
 *
 *  Created on: Dec 30, 2013
 *      Author: zozoh
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>

#include <libzcapi/tcp.h>
#include <libzcapi/log.h>
#include <libzcapi/datatime.h>
#include <libzcapi/args.h>

void parse_args(int i, const char *argnm, const char *argval, void *userdata)
{
    int *port = (int *) userdata;
    if (0 == strcmp(argnm, "p"))
    {
        *port = atoi(argval);
    }
}

int on_recv(int rsz, void *data, z_tcp_context *ctx)
{
    _I(" - zplay.on_recv : %d", rsz);
    if (rsz == -1) return Z_TCP_CONTINUE;
    if (rsz < 4) return Z_TCP_QUIT;
    if (rsz < 5) return Z_TCP_CLOSE;
    return Z_TCP_CONTINUE;
}

char *abc = "zozoh is great!\n";

int on_send(int *size, void **data, struct z_tcp_context *ctx)
{
    *size = strlen(abc);
    _I(" - zplay.on_send : %d", *size);
    *data = abc;
    return Z_TCP_CONTINUE;
}

/**
 * 主程序入口：主要检查参数等
 */
int main(int argc, char *argv[])
{

    // 防止错误
    if (argc != 2)
    {
        printf("useage: \n\n    %s [-p=8722]\n\n", argv[0]);
        return EXIT_FAILURE;
    }

    // 得到参数
    int port;
    z_args_m0_parse(argc, argv, parse_args, &port);

    // 调用主逻辑
    _I("hello : port is %d", port);
    z_tcp_context *ctx = z_tcp_alloc_context(1024);
    ctx->host_ipv4 = "127.0.0.1";
    ctx->port = port;
    ctx->msg = 0xFFFFFFFF;
    ctx->on_recv = on_recv;
    ctx->on_send = on_send;

    // 启动发送
    z_tcp_start_connect(ctx);

    // 释放
    z_tcp_free_context(&ctx);

    // 返回成功
    return EXIT_SUCCESS;
}
