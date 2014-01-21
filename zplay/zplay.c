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
#include <libzcapi/alg/md5.h>
#include <libffsplit/zffsplit.h>

#include "zplay.h"
//------------------------------------------------------------------------
void _save_rgb_frame_to_file(TLDDecoding *dec, int iFrame)
{
    // 计算文件名
    char ph[100];
    sprintf(ph, "%s/fr_%08d.ppm", dec->args->ppmPath, iFrame);

    // 写入到文件
    zff_save_rgb_frame_to_file(ph, dec->pRGB);

    // show md5
    char md5[33];
    md5_context md5_ctx;
    md5_file(&md5_ctx, ph);
    md5_sprint(&md5_ctx, md5);

    printf("  >>> %s (%s)\n", ph, md5);
}
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
    else if (0 == strcmp(argnm, "ppm"))
    {
        args->ppmPath = malloc(strlen(argval) + 1);
        strcpy(args->ppmPath, argval);
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
                ZFFStream *pStream;
                re = zff_avcodec_context_rdata(tld->data,
                                               tld->len,
                                               &remoteCC,
                                               &pStream);
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

                dec->cc = avcodec_alloc_context3(c);
                dec->stream_index = pStream->index;
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

                // 这里初始化一下转换上下文
                dec->W = dec->cc->width;
                dec->H = dec->cc->height;
                dec->swsc = sws_alloc_context();
                sws_getCachedContext(dec->swsc,
                                     dec->W,
                                     dec->H,
                                     dec->cc->pix_fmt,
                                     dec->W,
                                     dec->H,
                                     AV_PIX_FMT_RGB24,
                                     SWS_BICUBIC,
                                     NULL,
                                     NULL,
                                     NULL);
                if (dec->swsc == NULL)
                {
                    printf("Cannot initialize the conversion context!\n");
                    exit(0);
                }
                // 准备输出的 RGB 镇的镇缓冲
                uint8_t *buffer;
                int numBytes;
                // Determine required buffer size and allocate buffer
                numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, dec->W, dec->H);
                buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

                avpicture_fill((AVPicture *) dec->pRGB,
                               buffer,
                               AV_PIX_FMT_RGB24,
                               dec->W,
                               dec->H);
                dec->pRGB->width = dec->W;
                dec->pRGB->height = dec->H;

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
                if (pkt->stream_index == dec->stream_index)
                {
                    // 进行解码
                    // dec->cc->active_thread_type = 0;
                    avcodec_decode_video2(dec->cc, dec->pFrame, &finished, pkt);
                    if (finished)
                    {
                        printf("----------------------> [%5d] : %lld\n",
                               frameIndex,
                               dec->pFrame->best_effort_timestamp);
                        frameIndex++;
                        sws_scale(dec->swsc,
                                  dec->pFrame->data,
                                  dec->pFrame->linesize,
                                  0,
                                  dec->H,
                                  dec->pRGB->data,
                                  dec->pRGB->linesize);
                        _save_rgb_frame_to_file(dec, frameIndex);
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
    if (argc != 3)
    {
        printf("zplay useage: \n\n    %s\n\n",
               "zplay [-p=8722] [-ppm=/path/to/ppm_output]");
        return EXIT_FAILURE;
    }

    // 初始化 ffmpeg
    av_register_all();

    // 得到参数
    ZPlayArgs args;
    z_args_m0_parse(argc, argv, parse_args, &args);

    printf("ZPlay listen:%d >> %s\n\n", args.port, args.ppmPath);

    // 创建接受逻辑上下文
    TLDReceving ring;
    ring.tld = NULL;
    ring.tlds = z_lnklst_alloc(_free_li);

    // 创建消费逻辑上下文
    TLDDecoding dec;
    dec.args = &args;
    dec.cc = NULL;
    dec.stream_index = 99;
    dec.swsc = NULL;
    dec.tlds = ring.tlds;
    dec.pFrame = (AVFrame *) avcodec_alloc_frame();
    dec.pRGB = (AVFrame *) avcodec_alloc_frame();

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
