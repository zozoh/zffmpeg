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
                //tld_brief_print(ring->tld);
                //tld_freep(&ring->tld);
                z_lnklst_add_last(ring->tlds, z_lnklst_item_alloc(ring->tld));
                ring->tld = NULL;
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
void _free_TLDDecoding(TLDDecoding *dec)
{
    if (NULL != dec->cc)
    {
        avcodec_close(dec->cc);
        av_freep(&dec->cc);
    }
    if (NULL != dec->swsc)
    {
        sws_freeContext(dec->swsc);
        dec->swsc = NULL;
    }
}
//------------------------------------------------------------------------
void *pthread_decoding(void *arg)
{
    TLDDecoding *dec = (TLDDecoding *) arg;
    AVFrame *pFrame = (AVFrame *) avcodec_alloc_frame();
    AVFrame *pRGB = (AVFrame *) avcodec_alloc_frame();
    int frameIndex = 0;

    while (TRUE)
    {
        if (z_lnklst_is_empty(dec->tlds))
        {
            usleep(1000 * 1000);
            _I("dec->tlds is empty");
        }
        // 开始消费
        else
        {
            // 读取一个 TLD
            TLD *tld;
            int tld_size;
            int re;
            int finished = 0;
            z_lnklst_pop_first(dec->tlds, &tld, &tld_size);

            // 如果是 AVCodecContext，则初始化
            if (0xE0 == tld->tag)
            {
                _free_TLDDecoding(dec);

                // 得到解码上下文
                AVCodecContext *remoteCC;
                AVCodec *c;
                re = zff_avcodec_context_rdata(tld->data, tld->len, &remoteCC);
                if (0 != re)
                {
                    _F("fail to recover AVCodecContext from remote! re=%d", re);
                    exit(re);
                }

                // 找到解码器
                c = avcodec_find_decoder(remoteCC->codec_id);
                if (NULL == c)
                {
                    _F("fail to found decoder : %d(%s)",
                       remoteCC->codec_id,
                       remoteCC->codec_name);
                    exit(0);
                }

                //dec->cc = cc2;
                dec->cc = avcodec_alloc_context3(c);
                //dec->cc->thread_count = remoteCC->thread_count;
                //dec->cc->thread_type = remoteCC->thread_type;
                // dec->cc->active_thread_type = remoteCC->active_thread_type;
                mp_copy_lav_codec_headers(dec->cc, remoteCC);
                //zff_avcodec_context_copy(cc3,dec->cc);
                re = avcodec_open2(dec->cc, c, NULL);
                if (0 != re)
                {
                    _F("fail to avcodec_open2 %p by  %d(%s)",
                       dec->cc,
                       c->id,
                       c->name);
                    exit(re);
                }

            }
            // 如果是 AVPacket 则开始解码
            else if (0xF0 == tld->tag)
            {
                AVPacket *pkt = NULL;
                re = zff_avcodec_packet_rdata(tld->data, tld->data_size, &pkt);
                // 读取包
                if (0 != re)
                {
                    printf("fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck \n");
                    exit(0);
                }
                if (pkt->stream_index == 0)
                {
                    // 进行解码
                    // dec->cc->active_thread_type = 0;
                    avcodec_decode_video2(dec->cc, pFrame, &finished, pkt);
                    if (finished)
                    {
                        printf("----------------------> [%5d] : %lld\n",
                               frameIndex++,
                               pFrame->best_effort_timestamp);

                    }
                }
                // 释放包
                if (NULL != pkt)
                {
                    FREEP(pkt->data);
                    FREEP(pkt->side_data);
                    FREEP(pkt);
                }
            }
            // 见鬼了
            else
            {
                _F("zplay:: get TLD error tag : %02X", tld->tag);
                exit(0);
            }
            // 释放 TLD
            tld_freep(&tld);
        }
    }
    return NULL;
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

    // 创建接受逻辑上下文
    TLDReceving ring;
    ring.tld = NULL;
    ring.tlds = z_lnklst_alloc(_free_li);

    // 创建消费逻辑上下文
    TLDDecoding dec;
    dec.cc = NULL;
    dec.swsc = NULL;
    dec.tlds = ring.tlds;

    // 启动消费线程
    pthread_t tId;
    pthread_attr_t tAttr;
    pthread_attr_init(&tAttr);
    if (0 != pthread_create(&tId, &tAttr, pthread_decoding, &dec))
    {
        _F("!!! fail to pthread_create");
        exit(0);
    }

    // 调用主逻辑
    _I("hello : port is %d", args.port);
    z_tcp_context *ctx = z_tcp_alloc_context(1024);
    ctx->app_name = "ZPlay";
    ctx->port = args.port;
    ctx->msg = 0x00; //Z_TCP_MSG_DUMP_DATA_RECV | Z_TCP_MSG_DUMP_DATA_SEND;
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
