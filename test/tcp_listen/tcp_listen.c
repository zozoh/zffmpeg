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
    uint8_t * d = (uint8_t *) data;
    char cs[100];
    sprintf(cs,
            "%02x %02x %02x %02x %02x %02x",
            d[0],
            d[1],
            d[2],
            d[3],
            d[4],
            d[5]);
    _I(" - tcp_listen.on_recv : %d : %s ...", rsz, cs);
    return Z_TCP_CONTINUE;
}

char *abc = "zozoh is great!\n";

int on_send(int *size, void **data, z_tcp_context *ctx)
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
#define ABC(nm) \
    printf("%s", #nm);

    ABC(aaa);

    // 得到参数
    int port;
    z_args_m0_parse(argc, argv, parse_args, &port);

    // 调用主逻辑
    _I("hello : port is %d", port);
    z_tcp_context *ctx = z_tcp_alloc_context(1024);
    ctx->app_name = "test_tcp_listen";
    ctx->port = port;
    ctx->msg = 0xFFFFFFFF;
    ctx->on_recv = on_recv;
    //ctx->on_send = on_send;

    // 启动监听
    z_tcp_start_listen(ctx);

    // 释放
    z_tcp_free_context(&ctx);

    // 返回成功
    return EXIT_SUCCESS;
}
