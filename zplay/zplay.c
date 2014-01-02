#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/avstring.h>
#include <libavutil/pixfmt.h>
#include <libavutil/log.h>
#include <libavutil/common.h>
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
#include <libavutil/imgutils.h>
#include <libavutil/avassert.h>
#include <libavformat/avio.h>

#include <libzcapi/tcp.h>
#include <libzcapi/log.h>
#include <libzcapi/datatime.h>
#include <libzcapi/args.h>
#include <libffsplit/zffsplit.h>

#include "zplay.h"

void parse_args(int i, const char *argnm, const char *argval, void *userdata)
{
    ZPlayArgs *args = (ZPlayArgs *) userdata;
    if (0 == strcmp(argnm, "p"))
    {
        args->port = atoi(argval);
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
        printf("zplay useage: \n\n    %s\n\n", "zplay [-p=8722]");
        return EXIT_FAILURE;
    }

    // 初始化 ffmpeg
    av_register_all();

    // 得到参数
    ZPlayArgs args;
    z_args_m0_parse(argc, argv, parse_args, &args);

    // 调用主逻辑
    _I("hello : port is %d", args.port);
    z_tcp_context *ctx = z_tcp_alloc_context(1024);
    ctx->app_name = "ZPlay";
    ctx->port = args.port;
    ctx->msg = 0xFFFFFFFF;
    ctx->on_recv = on_recv;
    ctx->on_send = on_send;

    // 启动监听
    z_tcp_start_listen(ctx);

    // 释放
    z_tcp_free_context(&ctx);

    // 返回成功
    return EXIT_SUCCESS;
}
