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
//------------------------------------------------------------------------
// 释放链表项的放方法
void _free_li(struct z_lnklst_item *li)
{
    if (NULL != li->data)
    {
        free(li->data);
        li->data = NULL;
    }
}
//------------------------------------------------------------------------
void parse_args(int i, const char *argnm, const char *argval, void *userdata)
{
    ZPlayArgs *args = (ZPlayArgs *) userdata;
    if (0 == strcmp(argnm, "p"))
    {
        args->port = atoi(argval);
    }
}
//------------------------------------------------------------------------
int on_recv(int rsz, void *data, z_tcp_context *ctx)
{
    TLDReceving *ring = (TLDReceving *) ctx->userdata;
    uint8_t *d = (uint8_t *) data;

    int n_used = 0;
    while (n_used < rsz)
    {
        // 重新开启一个 TLD
        if (NULL == ring->tld)
        {
            ring->tld = tld_alloc();
            n_used += tld_copy_head(ring->tld, d + n_used, rsz - n_used);
            if (tld_head_finished(ring->tld)) tld_parse_head(ring->tld);
        }
        // 头部还不完整
        else if (!tld_head_finished(ring->tld))
        {
            n_used += tld_copy_head(ring->tld, d + n_used, rsz - n_used);
            if (tld_head_finished(ring->tld)) tld_parse_head(ring->tld);
        }
        // 数据还不完整
        else if (!tld_data_finished(ring->tld))
        {
            n_used += tld_copy_data(ring->tld, d + n_used, rsz - n_used);
            if (tld_data_finished(ring->tld))
            {
                tld_brief_print(ring->tld);
                tld_freep(&ring->tld);
            }
        }
        // 见鬼了
        else
        {
            _F("!!!tld receving impossible!!!");
            exit(0);
        }

    }
    return Z_TCP_CONTINUE;
}
//------------------------------------------------------------------------
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

    // 创建主上下文
    TLDReceving ring;
    ring.tld = NULL;
    ring.tlds = z_lnklst_alloc(_free_li);

    // 调用主逻辑
    _I("hello : port is %d", args.port);
    z_tcp_context *ctx = z_tcp_alloc_context(1024);
    ctx->app_name = "ZPlay";
    ctx->port = args.port;
    ctx->msg = 0x00;
    ctx->on_recv = on_recv;
    ctx->userdata = &ring;

    // 启动监听
    z_tcp_start_listen(ctx);

    // 释放
    z_tcp_free_context(&ctx);

    // 返回成功
    return EXIT_SUCCESS;
}
//------------------------------------------------------------------------
