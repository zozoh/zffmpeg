/*
 * zffsplit.c
 *
 * 实现所有其他程序（包括 ffsplit 本身）可能会用到的函数
 *
 *  Created on: Dec 23, 2013
 *      Author: zozoh
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <libavcodec/avcodec.h>

#include "zffsplit.h"
//------------------------------------------------------------------------------
#define ZFF_TLD_HEAD_SIZE 6
//------------------------------------------------------------------------------
// 根据给定的指针，读取四个字节，组成一个 uint32_t 的整数
uint32_t zff_read_tld_len(uint8_t *p)
{
    uint32_t a = *(p);
    uint32_t b = *(p + 1);
    uint32_t c = *(p + 2);
    uint32_t d = *(p + 3);

    return a | (b << 8) | (c << 16) | (d << 24);
}
//------------------------------------------------------------------------------
uint8_t *zff_tld_w(uint8_t *dest,
        uint8_t tag,
        uint32_t len,
        const void *data,
        int pad)
{
    int sz_tag = sizeof(tag);
    int sz_len = sizeof(len);
    uint32_t sz_data = len + pad;

    uint8_t *p = dest;

    memcpy(p, &tag, sz_tag);
    p += sz_tag;

    memset(p, 0, 1);
    p += 1;

    memcpy(p, &sz_data, sz_len);
    p += sz_len;

    memcpy(p, (uint8_t *) data, len);
    p += len;

    if (pad)
    {
        memset(p, 0, pad);
        p += pad;
    }

    return p;
}
//------------------------------------------------------------------------------
uint8_t *zff_tld_r(uint8_t *src, uint8_t *tag, uint32_t *len, void *data)
{
    *tag = *src;
    *len = zff_read_tld_len(src + 2);

    // 针对 extradata（0xE3）需要认为有 16 个 pad
    int pad = 0xE3 == *tag ? FF_INPUT_BUFFER_PADDING_SIZE : 0;

    int sz = (*len) - pad;
    uint8_t *dest = (uint8_t *) malloc(sz);
    memcpy(dest, src + ZFF_TLD_HEAD_SIZE, sz);

    *((void **) data) = dest;

    return src + ZFF_TLD_HEAD_SIZE + *len;
}
//------------------------------------------------------------------------------
// 在一块分配好的内存上，写入 TLD 的头部，out_size 为这块内存的总大小
// 返回指向 TLD 的 data 部分的指针
uint8_t *_write_tld_head(uint8_t tag, int out_size, uint8_t *dest)
{
    *dest = tag;
    *(dest + 1) = 0x00;
    uint32_t data_size = out_size - ZFF_TLD_HEAD_SIZE;
    memcpy(dest + 2, &data_size, sizeof(data_size));
    return dest + ZFF_TLD_HEAD_SIZE;
}
//------------------------------------------------------------------------------
// 在一块 TLD 布局的内存上，检查头部的 6 个字节是否合法
int _check_tld_head(uint8_t *src, int size, uint8_t expectTag)
{
    // 首先校验传来的 TLD 头部是不是合法
    if (*src != expectTag) return -1;
    if (*(src + 1) != 0x00) return -2;
    // 读取长度
    uint32_t len = zff_read_tld_len(src + 2);
    if (len + ZFF_TLD_HEAD_SIZE != size) return -3;
    // 恩 ... 看样子没啥子问题
    return 0;
}
//------------------------------------------------------------------------------
uint8_t *zff_avcodec_packet_w(int *out_size, AVPacket *src)
{
    // 准备要 copy 的上下文
    AVPacket *pkt = src;

    // 首先需要计算要分配的内存大小
    // 首先是 6 个字节，为 0xE0
    *out_size = ZFF_TLD_HEAD_SIZE;
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~: F1
    int sz_f1 = sizeof(AVPacket);
    if (sz_f1) *out_size += sz_f1 + ZFF_TLD_HEAD_SIZE;
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~: F2
    int sz_f2 = pkt->size;
    if (sz_f2) *out_size += sz_f2 + ZFF_TLD_HEAD_SIZE;
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~:
    // 总大小为已经记录到 *out_size 中了

    // 分配内存
    uint8_t *dest = (uint8_t *) av_malloc(*out_size);
    if (NULL == dest)
    {
        printf("!!! zff_avcodec_packet_w: Fail to av_malloc(%d)\n", *out_size);
        return NULL;
    }

    // 写入头部
    void *p = _write_tld_head(0xF0, *out_size, dest);

    // 开始 copy 主结构体部分
    p = zff_tld_w(p, 0xF1, sz_f1, pkt, 0);
    p = zff_tld_w(p, 0xF2, sz_f2, pkt->data, 0);

    // 返回成功
    return dest;
}
//------------------------------------------------------------------------------
int zff_avcodec_packet_r(uint8_t* pTag, int size, AVPacket **pkt)
{
    // 首先校验传来的 TLD 头部是不是合法
    int re = _check_tld_head(pTag, size, 0xF0);
    if (re != 0) return re;

    return zff_avcodec_packet_rdata(pTag + ZFF_TLD_HEAD_SIZE,
                                    size - ZFF_TLD_HEAD_SIZE,
                                    pkt);
}
//------------------------------------------------------------------------------
int zff_avcodec_packet_rdata(uint8_t* pData, int size, AVPacket **pkt)
{
    // 准备开始复制 ...
    uint8_t tag;
    uint32_t len;
    uint8_t *p = pData;     // 记录了每个 TLD copy 开始的位置
    uint8_t *data = NULL;   // 记录每个传出的字段

    // 第一个 TLD 一定是 AVCodecContext 本身
    p = zff_tld_r(p, &tag, &len, &data);
    *pkt = (AVPacket *) data;

    // 继续读取 TLD ...
    for (;;)
    {
        // 如果已经全部读完了，返回
        if ((p - pData) == size) goto OK;
        // 超出了的话，我也不知道该怎么办，自裁吧 -_-!
        if ((p - pData) > size)
        {
            printf("!!! zff_avcodec_context_r:: out of range : %d/%d\n",
                   (p - pData),
                   size);
            exit(0);
        }
        // 读取一个 TLD
        p = zff_tld_r(p, &tag, &len, &data);
        // 根据 tag 来设置不同的字段
        switch (tag)
        {
        case 0xF2:
            (*pkt)->data = data;
            break;
        default:
            printf("!!! zff_avcodec_context_r:: unknowns tag %02X\n", tag);
            exit(0);
        }

    }
    // 一切 OK
    OK: return 0;
}
//------------------------------------------------------------------------------
int zff_avcodec_context_r(uint8_t* pTag, int size, AVCodecContext **ctx)
{
    // 首先校验传来的 TLD 头部是不是合法
    int re = _check_tld_head(pTag, size, 0xE0);
    if (re != 0) return re;

    return zff_avcodec_context_rdata(pTag + ZFF_TLD_HEAD_SIZE,
                                     size - ZFF_TLD_HEAD_SIZE,
                                     ctx);
}
//------------------------------------------------------------------------------
int zff_avcodec_context_rdata(uint8_t* pData, int size, AVCodecContext **ctx)
{
    // 准备开始复制 ...
    uint8_t tag;
    uint32_t len;
    uint8_t *p = pData;     // 记录了每个 TLD copy 开始的位置
    uint8_t *data = NULL;   // 记录每个传出的字段

    // 第一个 TLD 一定是 AVCodecContext 本身
    p = zff_tld_r(p, &tag, &len, &data);
    AVCodecContext *c = (AVCodecContext *) data;

    /* set values specific to opened codecs back to their default state */
    c->priv_data = NULL;
    c->codec = NULL;
    c->slice_offset = NULL;
    c->hwaccel = NULL;
    c->thread_opaque = NULL;
    c->internal = NULL;

    /* reallocate values that should be allocated separately */
    c->rc_eq = NULL;
    c->extradata = NULL;
    c->intra_matrix = NULL;
    c->inter_matrix = NULL;
    c->rc_override = NULL;

    // 继续读取 TLD ...
    for (;;)
    {
        // 如果已经全部读完了，返回
        if ((p - pData) == size) goto OK;
        // 超出了的话，我也不知道该怎么办，自裁吧 -_-!
        if ((p - pData) > size)
        {
            printf("!!! zff_avcodec_context_r:: out of range : %d/%d\n",
                   (p - pData),
                   size);
            exit(0);
        }
        // 读取一个 TLD
        p = zff_tld_r(p, &tag, &len, &data);
        // 根据 tag 来设置不同的字段
        switch (tag)
        {
        case 0xE2:
            c->rc_eq = (char *) data;
            break;
        case 0xE3:
            c->extradata = data;
            break;
        case 0xE4:
            c->intra_matrix = (uint16_t *) data;
            break;
        case 0xE5:
            c->intra_matrix = (uint16_t *) data;
            break;
        case 0xE6:
            c->rc_override = (RcOverride *) data;
            break;
        default:
            printf("!!! zff_avcodec_context_r:: unknowns tag %02X\n", tag);
            exit(0);
        }

    }

    // 一切 OK
    OK: *ctx = c;
    return 0;
}
//------------------------------------------------------------------------------
uint8_t *zff_avcodec_context_w(int *out_size, AVCodecContext *src)
{
    // 准备要 copy 的上下文
    AVCodecContext *c = src;

    // 首先需要计算要分配的内存大小
    int pad = FF_INPUT_BUFFER_PADDING_SIZE;
    *out_size = ZFF_TLD_HEAD_SIZE;
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~: E1
    int sz_e1 = sizeof(AVCodecContext);
    if (sz_e1) *out_size += sz_e1 + ZFF_TLD_HEAD_SIZE;
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~: E2
    int sz_e2 = NULL == c->rc_eq ? 0 : strlen(c->rc_eq);
    if (sz_e2) *out_size += sz_e2 + ZFF_TLD_HEAD_SIZE;
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~: E3
    int sz_e3 = c->extradata_size > 0 ? c->extradata_size : 0;
    if (sz_e3) *out_size += sz_e3 + pad + ZFF_TLD_HEAD_SIZE;
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~: E4
    int sz_e4 = c->intra_matrix ? 64 : 0;
    if (sz_e4) *out_size += sz_e4 + ZFF_TLD_HEAD_SIZE;
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~: E5
    int sz_e5 = c->inter_matrix ? 64 : 0;
    if (sz_e5) *out_size += sz_e5 + ZFF_TLD_HEAD_SIZE;
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~: E6
    int sz_e6 = c->rc_override_count * sizeof(RcOverride);
    if (sz_e6) *out_size += sz_e6 + ZFF_TLD_HEAD_SIZE;
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~:
    // 总大小为已经记录到 *out_size 中了

    // 分配内存
    uint8_t *dest = (uint8_t *) av_malloc(*out_size);
    if (NULL == dest)
    {
        printf("!!! zff_avcodec_context_w: Fail to av_malloc(%d)\n", *out_size);
        return NULL;
    }

    // 写入头部
    void *p = _write_tld_head(0xE0, *out_size, dest);

    // 开始 copy 主结构体部分
    if (sz_e1) p = zff_tld_w(p, 0xE1, sz_e1, c, 0);
    if (sz_e2) p = zff_tld_w(p, 0xE2, sz_e2, c->rc_eq, 0);
    if (sz_e3) p = zff_tld_w(p, 0xE3, sz_e3, c->extradata, pad);
    if (sz_e4) p = zff_tld_w(p, 0xE4, sz_e4, c->intra_matrix, 0);
    if (sz_e5) p = zff_tld_w(p, 0xE5, sz_e5, c->inter_matrix, 0);
    if (sz_e6) p = zff_tld_w(p, 0xE6, sz_e6, c->rc_override, 0);

    // 返回成功
    return dest;
}
//------------------------------------------------------------------------------
int zff_avcodec_context_copy(AVCodecContext *dest, const AVCodecContext *src)
{
    if (avcodec_is_open(dest))
    { // check that the dest context is uninitialized
        av_log(dest,
               AV_LOG_ERROR,
               "Tried to copy AVCodecContext %p into already-initialized %p\n",
               src,
               dest);
        return AVERROR(EINVAL);
    }
    memcpy(dest, src, sizeof(*dest));

    /* set values specific to opened codecs back to their default state */
    dest->priv_data = NULL;
    dest->codec = NULL;
    dest->slice_offset = NULL;
    dest->hwaccel = NULL;
    dest->thread_opaque = NULL;
    dest->internal = NULL;

    /* reallocate values that should be allocated separately */
    dest->rc_eq = NULL;
    dest->extradata = NULL;
    dest->intra_matrix = NULL;
    dest->inter_matrix = NULL;
    dest->rc_override = NULL;
    if (src->rc_eq)
    {
        dest->rc_eq = av_strdup(src->rc_eq);
        if (!dest->rc_eq) return AVERROR(ENOMEM);
    }

#define alloc_and_copy_or_fail(obj, size, pad) \
        if (src->obj && size > 0) { \
            dest->obj = av_malloc(size + pad); \
            if (!dest->obj) \
                goto fail; \
            memcpy(dest->obj, src->obj, size); \
            if (pad) \
                memset(((uint8_t *) dest->obj) + size, 0, pad); \
        }
    alloc_and_copy_or_fail(extradata,
                           src->extradata_size,
                           FF_INPUT_BUFFER_PADDING_SIZE);
    alloc_and_copy_or_fail(intra_matrix, 64 * sizeof(int16_t), 0);
    alloc_and_copy_or_fail(inter_matrix, 64 * sizeof(int16_t), 0);
    alloc_and_copy_or_fail(rc_override,
                           src->rc_override_count * sizeof(*src->rc_override),
                           0);
#undef alloc_and_copy_or_fail

    return 0;

    fail:  //
    av_freep(&dest->rc_override);
    av_freep(&dest->intra_matrix);
    av_freep(&dest->inter_matrix);
    av_freep(&dest->extradata);
    av_freep(&dest->rc_eq);
    return AVERROR(ENOMEM);
}
//------------------------------------------------------------------------------
void mp_copy_lav_codec_headers(AVCodecContext *avctx, AVCodecContext *st)
{
    if (st->extradata_size) {
        av_free(avctx->extradata);
        avctx->extradata_size = 0;
        avctx->extradata =
            av_mallocz(st->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
        if (avctx->extradata) {
            avctx->extradata_size = st->extradata_size;
            memcpy(avctx->extradata, st->extradata, st->extradata_size);
        }
    }
    avctx->codec_tag                = st->codec_tag;
    avctx->stream_codec_tag         = st->stream_codec_tag;
    avctx->bit_rate                 = st->bit_rate;
    avctx->width                    = st->width;
    avctx->height                   = st->height;
    avctx->pix_fmt                  = st->pix_fmt;
    avctx->sample_aspect_ratio      = st->sample_aspect_ratio;
    avctx->chroma_sample_location   = st->chroma_sample_location;
    avctx->sample_rate              = st->sample_rate;
    avctx->channels                 = st->channels;
    avctx->block_align              = st->block_align;
    avctx->channel_layout           = st->channel_layout;
    avctx->audio_service_type       = st->audio_service_type;
    avctx->bits_per_coded_sample    = st->bits_per_coded_sample;
}
//------------------------------------------------------------------------------
int zff_save_rgb_frame_to_file(char *dest, AVFrame *fr)
{
    // Open file
    FILE *pFile = fopen(dest, "wb");
    if (pFile == NULL) return -1;

    // Write header
    int width = fr->width;
    int height = fr->height;
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for (int y = 0; y < height; y++)
        fwrite(fr->data[0] + y * fr->linesize[0], 1, width * 3, pFile);

    // Close file
    fclose(pFile);

    return 0;
}
//------------------------------------------------------------------------------

