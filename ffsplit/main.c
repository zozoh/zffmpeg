#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>    // for usleep

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

#include <libzcapi/log.h>
#include <libzcapi/datatime.h>
#include <libzcapi/tcp.h>
#include <libzcapi/tld.h>
#include <libzcapi/files.h>
#include <libzcapi/c/lnklst.h>
#include <libzcapi/alg/md5.h>
#include <libffsplit/zffsplit.h>

//------------------------------------------------------------------------
typedef struct FFStream
{
    // 当前流的下标
    int sIndex;

    // 流的解码上下文
    AVCodecContext *cc;

    // 流的解码器
    AVCodec *codec;

    // 解码器上下文的内存 copy，以便发送给远端
    uint8_t *cc_data;
    int cc_data_size;

} FFStream;
//------------------------------------------------------------------------
void _init_codec(FFStream *s, AVStream *avs)
{
    s->cc = avs->codec;
    s->codec = avcodec_find_decoder(s->cc->codec_id);
    if (NULL == s->codec)
    {
        _F("Fail to find codec !!!");
        exit(0);
    }
}
//------------------------------------------------------------------------
typedef struct FFSplitContext
{
    char *vph;
    char *dph;
    AVFormatContext *fmtctx;

    // 保存当前视频流的相关信息
    FFStream v;

    // 这个是一个双向链表，线程共享的。
    // 它记录了要发送的 TLD 数据结构
    z_lnklst *tlds;

    // 发送线程的 ID
    pthread_t Tsender;
    pthread_attr_t Tsender_attr;

    // 记录视频的宽高
    int W;
    int H;

    // 记录临时解码和转换的指针，程序启动的时候分配一次即可
    AVFrame *pFrame;
    AVFrame *pRGB;

    // 转换的上下文
    struct SwsContext *swsc;

} FFSplitContext;
//------------------------------------------------------------------------
int tcp_on_send(int *size, void **data, struct z_tcp_context *ctx)
{
    FFSplitContext *sc = (FFSplitContext *) ctx->userdata;

    // 木有内容了，先休息 100 ms
    while (z_lnklst_is_empty(sc->tlds))
    {
        //_I("------------------------------- z_lnklst_is_empty");
        usleep(100 * 1000);
    }

    // 弹出一块进行发送
    z_lnklst_pop_first(sc->tlds, data, size);

    // 打印一下 MD5
    // uint8_t *d = (uint8_t *) *data;
    // tld_brief_print_data(d + TLD_HEAD_SIZE, (*size) - TLD_HEAD_SIZE);

    return Z_TCP_CONTINUE;
}
//------------------------------------------------------------------------
void *pthread_tld_sender(void *arg)
{
    FFSplitContext *sc = (FFSplitContext *) arg;

    int port = 8750;
    _I("hello : port is %d", port);
    z_tcp_context *ctx = z_tcp_alloc_context(1024 * 4);
    ctx->host_ipv4 = "127.0.0.1";
    ctx->port = port;
    ctx->msg = 0x00; //Z_TCP_MSG_DUMP_DATA_RECV | Z_TCP_MSG_DUMP_DATA_SEND;
    ctx->on_send = tcp_on_send;
    ctx->userdata = sc;

    // 启动发送
    z_tcp_start_connect(ctx);

    return NULL;
}
//------------------------------------------------------------------------
void _init_threads(FFSplitContext *sc)
{
    pthread_attr_init(&sc->Tsender_attr);
    int re = pthread_create(&sc->Tsender,
                            &sc->Tsender_attr,
                            pthread_tld_sender,
                            sc);
    if (0 != re)
    {
        _F("!!! fail to pthread_create : %d", re);
        exit(0);
    }
}
//------------------------------------------------------------------------
// 打开视频，初始化 AVFormatContext
void _open_video(FFSplitContext *sc)
{
    // 打开格式上下文
    sc->fmtctx = NULL;
    AVDictionary *opts = NULL;
    avformat_open_input(&sc->fmtctx, sc->vph, NULL, &opts);

    // 初始化 ffmpeg 流格式上下文
    if (avformat_find_stream_info(sc->fmtctx, NULL) < 0)
    {
        _W("floader:: !!! fail to create format contex");
        exit(0);
    }
}
//------------------------------------------------------------------------
// 打开流媒体
void _open_stream(FFSplitContext *sc)
{

    sc->vph = "http://localhost:8090/test.flv";

    // 打开格式上下文
    sc->fmtctx = NULL;

    AVDictionary *avdic = NULL;
    char option_key[] = "rtsp_transport";
    char option_value[] = "tcp";
    av_dict_set(&avdic, option_key, option_value, 0);
    char option_key2[] = "max_delay";
    char option_value2[] = "5000000";
    av_dict_set(&avdic, option_key2, option_value2, 0);

    avformat_open_input(&sc->fmtctx, sc->vph, NULL, &avdic);

    // 初始化 ffmpeg 流格式上下文
    if (avformat_find_stream_info(sc->fmtctx, NULL) < 0)
    {
        _W("floader:: !!! fail to create format contex");
        exit(0);
    }
}
//------------------------------------------------------------------------
// 打印流信息，并且找到视频流和音频流的编号
void _find_streams(FFSplitContext *sc)
{
    printf("AVFormatContext : \n");
    printf("  > AVMEDIA_TYPE_VIDEO : %d\n", AVMEDIA_TYPE_VIDEO);
    printf("  > AVMEDIA_TYPE_AUDIO : %d\n", AVMEDIA_TYPE_AUDIO);
    for (int i = 0; i < sc->fmtctx->nb_streams; i++)
    {
        AVStream *s = sc->fmtctx->streams[i];
        printf(" - STREAM %d : media-type = %d\n", i, s->codec->codec_type);
        switch ((int) s->codec->codec_type)
        {
        case AVMEDIA_TYPE_VIDEO:
            sc->v.sIndex = i;
            _init_codec(&sc->v, s);
            break;
        }
    }
    printf("\n");
}
//------------------------------------------------------------------------
void _re_init(FFSplitContext *sc)
{
    // 看到了吧，如果直接 clone 一个，也是可以正常解码的
    // AVCodecContext *cc = avcodec_alloc_context3(NULL);
    // zff_avcodec_context_copy(cc, sc->v.cc);
    // 不 clone 一个，直接写到一块内存里，然后再读回来，依然能解码
    _I("sc->v.cc->active_thread_type : %d", sc->v.cc->active_thread_type);
    ZFFStream stream;
    AVStream *avs = sc->fmtctx->streams[sc->v.sIndex];
    stream.index = avs->index;
    stream.type = 0;
    stream.time_base.num = avs->time_base.num;
    stream.time_base.den = avs->time_base.den;
    stream.frame_rate.num = avs->r_frame_rate.num;
    stream.frame_rate.den = avs->r_frame_rate.den;
    stream.start_time = avs->start_time;
    stream.duration = avs->duration;
    sc->v.cc_data = zff_avcodec_context_w(&sc->v.cc_data_size,
                                          sc->v.cc,
                                          &stream);

    // 向发送队列里发送一份
    uint8_t *data = malloc(sc->v.cc_data_size);
    memcpy(data, sc->v.cc_data, sc->v.cc_data_size);
    z_lnklst_add_last(sc->tlds, z_lnklst_item_alloc2(data, sc->v.cc_data_size));

    AVCodecContext *cc = NULL;
    ZFFStream *pStream;
    if (0 != zff_avcodec_context_r(data, sc->v.cc_data_size, &cc, &pStream))
    {
        printf("kao kao kao kao kao kao kao kao kao !!!");
        exit(0);
    }

    avcodec_close(sc->v.cc);
    av_freep(&sc->v.cc);

    sc->v.cc = cc;
    sc->v.codec = avcodec_find_decoder(cc->codec_id);
    sc->v.sIndex = pStream->index;

    free(pStream);

    // 打开视频解码器
    if (avcodec_open2(sc->v.cc, sc->v.codec, NULL) < 0)
    {
        _F("!!! fail to avcodec_open2 !!!");
        exit(0);
    }

    // 提出宽高
    sc->W = sc->v.cc->width;
    sc->H = sc->v.cc->height;
}
//------------------------------------------------------------------------
// 打开解码器
void _open_codec(FFSplitContext *sc)
{
    // 打开视频解码器
    if (avcodec_open2(sc->v.cc, sc->v.codec, NULL) < 0)
    {
        _F("!!! fail to avcodec_open2 !!!");
        exit(0);
    }

    // 提出宽高
    sc->W = sc->v.cc->width;
    sc->H = sc->v.cc->height;

    // 尝试一下，重新自己分配视频的解码器等
    _re_init(sc);
}
//------------------------------------------------------------------------
// 初始化 sws 以便对每帧进行图像转换
void _init_sws(FFSplitContext *sc)
{
    sc->swsc = sws_alloc_context();
    //sws_init_context(sc->swsc, NULL, NULL);
    sws_getCachedContext(sc->swsc,
                         sc->W,
                         sc->H,
                         sc->v.cc->pix_fmt,
                         sc->W,
                         sc->H,
                         AV_PIX_FMT_RGB24,
                         SWS_BICUBIC,
                         NULL,
                         NULL,
                         NULL);
    if (sc->swsc == NULL)
    {
        printf("Cannot initialize the conversion context!\n");
        exit(0);
    }
}
//------------------------------------------------------------------------
void _save_rgb_frame_to_file(FFSplitContext *sc, int iFrame)
{
    // 计算文件名
    char ph[100];
    sprintf(ph, "%s/ppm/fr_%08d.ppm", sc->dph, iFrame);

    // 写入到文件
    zff_save_rgb_frame_to_file(ph, sc->pRGB);

    // show md5
    char md5[33];
    md5_context md5_ctx;
    md5_file(&md5_ctx, ph);
    md5_sprint(&md5_ctx, md5);

    printf("  >>> %s (%s)\n", ph, md5);
}
//------------------------------------------------------------------------
void _free_cloned_packet(AVPacket **pkt)
{
    av_free_packet(*pkt);
    av_freep(pkt);
    *pkt = NULL;
}
//------------------------------------------------------------------------
// 这个函数可以看到， copy 一个 AVPacket 是很容易的。
void _clone_packet(FFSplitContext *sc, AVPacket *src, AVPacket **pkt)
{
    int size = -1;
    uint8_t *data = zff_avcodec_packet_w(&size, src);

    uint8_t *data2 = malloc(size);
    memcpy(data2, data, size);
    z_lnklst_add_last(sc->tlds, z_lnklst_item_alloc2(data2, size));

    char md5[33];
    md5_context md5_ctx;
    md5_uint8(&md5_ctx, data2, size);
    md5_sprint(&md5_ctx, md5);

    // 保存到文件
    char ph[100];
    sprintf(ph, "%s/pkt/%08lld.pkt", sc->dph, src->pos);
    z_fwrite(ph, data2 + 6, size - 6);

    sprintf(ph, "%s/pkt/%08lld.info", sc->dph, src->pos);
    char txt[200];
    sprintf(txt,
            "AVPacket: %d \nSI:%d du:%d pts:%lld dts:%lld data:%p(%d bytes)\n",
            sizeof(AVPacket),
            src->stream_index,
            src->duration,
            src->pts,
            src->dts,
            src->data,
            src->size);
    z_fwrite_str(ph, txt);

    //_I("z_lnklst_add_last : %d : %s", sc->tlds->size, md5);

    if (0 != zff_avcodec_packet_rdata(data + 6, size - 6, pkt))
    {
        printf("fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck \n");
        exit(0);
    }
    av_free(data);
}
//------------------------------------------------------------------------
// 循环读取视频的数据包
void _read_packets_from_video(FFSplitContext *sc)
{
    AVPacket *pkt = (AVPacket *) av_malloc(sizeof(AVPacket));
    int re = 0;

    /*
     int64_t ms = 3000;
     int64_t timestamp = (ms * sc->v.cc->time_base.den)
     / (1000 * sc->v.cc->time_base.num);
     re = av_seek_frame(sc->fmtctx, sc->v.sIndex, timestamp,
     AVSEEK_FLAG_BACKWARD);
     if (re < 0)
     {
     _F("fail to seek : re=%d", re);
     exit(re);
     }
     */

    printf("reading AVPackets ...\n");
    int i = 0;
    int finished = 0;
    int frameIndex = 0;
    while (1)
    {
        memset(pkt, 0, sizeof(AVPacket));
        re = av_read_frame(sc->fmtctx, pkt);
        if (re == AVERROR_EOF || re < 0) break;

        // 这个是我们读出来的包，先打印一下先
        i++;

        printf("AVPacket:: %4d si:%d: pos:%-9lld, sz:%-6u, data:%p(%d) \n",
               i,
               pkt->stream_index,
               pkt->pos,
               sizeof(AVPacket),
               pkt->data,
               pkt->size);

        // 试图解压一下，如果解压成功，把图片存出到目标路径
        if (pkt->stream_index == sc->v.sIndex)
        {
            AVPacket *p2 = NULL;
            // usleep(1000 * 1000);
            _clone_packet(sc, pkt, &p2);
            avcodec_decode_video2(sc->v.cc, sc->pFrame, &finished, p2);
            _free_cloned_packet(&p2);
            if (finished)
            {
                frameIndex++;

                printf("----------------------> [%5d] data:%p(%dx%d) : %lld\n",
                       frameIndex,
                       sc->pFrame->data,
                       sc->pFrame->width,
                       sc->pFrame->height,
                       sc->pFrame->best_effort_timestamp);

                // 保存到 PPM 文件中
                sws_scale(sc->swsc,
                          sc->pFrame->data,
                          sc->pFrame->linesize,
                          0,
                          sc->H,
                          sc->pRGB->data,
                          sc->pRGB->linesize);
                _save_rgb_frame_to_file(sc, frameIndex);
            }
        }
    }

    if (NULL != pkt)
    {
        av_free_packet(pkt);
        av_freep(&pkt);
    }
    printf(" %d AVPackets readed\n", i);

}
//------------------------------------------------------------------------
// 分配解码和输出帧
// 这里给上下文分配两个帧对象，一个用来解压，一个用来转换成输出图像
void _alloc_frames(FFSplitContext *sc)
{
    sc->pFrame = (AVFrame *) avcodec_alloc_frame();
    sc->pRGB = (AVFrame *) avcodec_alloc_frame();

    if (NULL == sc->pFrame || NULL == sc->pRGB)
    {
        _F("fail to avcodec_alloc_frame()");
        exit(0);
    }
}
//------------------------------------------------------------------------
// 为 AVPicture 设置数据缓冲，以便输出到文件里
void _fill_picture_for_file(FFSplitContext *sc)
{
    uint8_t *buffer;
    int numBytes;
    // Determine required buffer size and allocate buffer
    numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, sc->W, sc->H);
    buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

    avpicture_fill((AVPicture *) sc->pRGB,
                   buffer,
                   AV_PIX_FMT_RGB24,
                   sc->W,
                   sc->H);
    sc->pRGB->width = sc->W;
    sc->pRGB->height = sc->H;
}
//------------------------------------------------------------------------
/**
 * 程序的主逻辑
 */
void ffsplit(FFSplitContext *sc)
{
    // 这里顺序初始化视频资源
    _open_video(sc);
    //_open_stream(sc);
    _find_streams(sc);
    _open_codec(sc);
    _init_sws(sc);

    // 分配帧
    _alloc_frames(sc);
    _fill_picture_for_file(sc);

    // 初始化发包线程
    _init_threads(sc);

    // 开始读包
    _read_packets_from_video(sc);

    // TODO: 最后释放
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
/**
 * 主程序入口：主要检查参数等
 */
int main(int argc, char *argv[])
{

    // 防止错误
    if (argc != 3)
    {
        printf("ffsplit useage: \n\n    %s\n\n",
               "ffsplit [/path/to/video] [/path/to/dest]");
        return EXIT_FAILURE;
    }

    // 初始化 ffmpeg
    av_register_all();
    avformat_network_init();

    _I("abc999999999999999999999999999999999999999999999999999");

    // 得到参数
    FFSplitContext sc;
    memset(&sc, 0, sizeof(FFSplitContext));
    sc.vph = argv[1];
    sc.dph = argv[2];

    // 初始化一下 TLD 堆栈
    sc.tlds = z_lnklst_alloc(_free_li);

    // 调用主逻辑
    printf("ffsplit v1.0 || %s", sc.vph);
    printf("\n-------------------------------------------------\n");
    ffsplit(&sc);
    printf("\n-------------------------------------------------\n");

    // 返回成功
    return EXIT_SUCCESS;
}
