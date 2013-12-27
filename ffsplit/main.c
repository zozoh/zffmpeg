#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>

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
#include <libffsplit/zffsplit.h>

//------------------------------------------------------------------------
typedef struct FFStream
{

    //AVStream *stream;
    int sIndex;
    AVCodecContext *cc;
    AVCodec *codec;

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

    //FFStream a;
    FFStream v;

    int W;
    int H;

    AVFrame *pFrame;
    AVFrame *pRGB;

    struct SwsContext *swsc;

} FFSplitContext;
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

}

// 初始化 sws 以便对每帧进行图像转换
void _init_sws(FFSplitContext *sc)
{
    sc->swsc = sws_getContext(sc->W,
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

// TODO 等下实现这个函数
void _save_packet_to_file(const char *dir, AVPacket *pkt)
{

}

void _save_rgb_frame_to_file(FFSplitContext *sc, int iFrame)
{
    FILE *pFile;
    char ph[100];
    int y;

    // Open file
    sprintf(ph, "%s/ppm/fr_%08d.ppm", sc->dph, iFrame);
    pFile = fopen(ph, "wb");
    if (pFile == NULL) return;

    printf("  >>> %s\n", ph);

    // Write header
    int width = sc->W;
    int height = sc->H;
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    AVFrame *pFrame = sc->pRGB;
    for (y = 0; y < height; y++)
        fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);

    // Close file
    fclose(pFile);
}

void _free_cloned_packet(AVPacket **pkt)
{
    av_free_packet(*pkt);
    av_freep(pkt);
    *pkt = NULL;
}

// 这个函数可以看到， copy 一个 AVPacket 是很容易的。
void _clone_packet(AVPacket *src, AVPacket **pkt)
{
//    AVPacket *p = (AVPacket *) av_malloc(sizeof(AVPacket));
//    *p = *src;
//    p->side_data = NULL;
//    p->side_data_elems = 0;
//    p->data = av_malloc(src->size);
//    memcpy(p->data, src->data, p->size);
//
//    *pkt = p;

    int size = -1;
    uint8_t *data = zff_avcodec_packet_w(&size, src);
    printf("fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck fuck \n");
    *pkt = zff_avcodec_packet_r(data, size);
}

// 循环读取视频的数据包
void _read_packets_from_video(FFSplitContext *sc)
{
    AVPacket *pkt = (AVPacket *) av_malloc(sizeof(AVPacket));
    int re = 0;

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
        printf("%08d [%d]: pos:%-9lld, sz:%-6u, data:%p(%d) \n",
               i++,
               pkt->stream_index,
               pkt->pos,
               sizeof(AVPacket),
               pkt->data,
               pkt->size);

        // 试图解压一下，如果解压成功，把图片存出到目标路径
        if (pkt->stream_index == sc->v.sIndex)
        {
            AVPacket *p2 = NULL;
            _clone_packet(pkt, &p2);
            avcodec_decode_video2(sc->v.cc, sc->pFrame, &finished, p2);
            _free_cloned_packet(&p2);
            if (finished)
            {
                printf("----------------------> [%5d] : %lld\n",
                       frameIndex++,
                       sc->pFrame->best_effort_timestamp);

                // 保存到 PPM 文件中
                sws_scale(sc->swsc,
                          sc->pFrame->data,
                          sc->pFrame->linesize,
                          0,
                          sc->H,
                          sc->pRGB->data,
                          sc->pRGB->linesize);

                _save_rgb_frame_to_file(sc, frameIndex++);
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
}

void _re_init(FFSplitContext *sc)
{
    // 看到了吧，如果直接 clone 一个，也是可以正常解码的
    // AVCodecContext *cc = avcodec_alloc_context3(NULL);
    // zff_avcodec_context_copy(cc, sc->v.cc);
    // 不 clone 一个，直接写到一块内存里，然后再读回来，依然能解码
    int size = 0;
    uint8_t *data = zff_avcodec_context_w(&size, sc->v.cc);
    AVCodecContext *cc = zff_avcodec_context_r(data, size);

    avcodec_close(sc->v.cc);
    av_freep(&sc->v.cc);

    sc->v.cc = cc;
    sc->v.codec = avcodec_find_decoder(cc->codec_id);

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

/**
 * 程序的入逻辑
 */
void ffsplit(FFSplitContext *sc)
{
    // 这里顺序初始化视频资源
    _open_video(sc);
    _find_streams(sc);
    _open_codec(sc);
    _init_sws(sc);

    // 尝试一下，重新自己分配视频的解码器等
    _re_init(sc);

    // 分配帧
    _alloc_frames(sc);
    _fill_picture_for_file(sc);

    // 开始读包
    _read_packets_from_video(sc);

    // TODO: 最后释放
}

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

    // 得到参数
    FFSplitContext sc;
    memset(&sc, 0, sizeof(FFSplitContext));
    sc.vph = argv[1];
    sc.dph = argv[2];

    // 调用主逻辑
    printf("ffsplit v1.0 || %s", sc.vph);
    printf("\n-------------------------------------------------\n");
    //ffsplit(&sc);
    _I("showd %d", 23);
    printf("\n-------------------------------------------------\n");

    // 返回成功
    return EXIT_SUCCESS;
}
