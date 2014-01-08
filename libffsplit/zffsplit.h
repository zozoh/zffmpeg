/*
 * zffsplit.h
 *
 * 这里我们给出一个头文件，以便其他程序使用  ffsplit 提供的方法
 *
 *  Created on: Dec 23, 2013
 *      Author: zozoh
 */

#ifndef ZFFSPLIT_H_
#define ZFFSPLIT_H_
//--------------------------------------------------------
// 根据给定的指针，读取四个字节，组成一个 uint32_t 的整数
extern uint32_t zff_read_tld_len(uint8_t *p);
//--------------------------------------------------------
/**
 * 将一个 TLD 数据写入目标内存区，data 指针表示的数据长度为 len
 * 写入的顺序为 tag->len->data ，一共将写入 1+2+len+pad 个8位字节
 * 函数返回下一个区域的指针，即 dest 偏移 len 长度后的位置
 */
extern uint8_t *zff_tld_w(uint8_t *dest,
        uint8_t tag,
        uint32_t len,
        const void *data,
        int pad);

/**
 * 从一块内存区域，都取一个 TLD 数据
 *
 * [in]  src  - 传入的内存指针，符合 TLD 的布局
 * [out] tag  - src 的第一个字节，作为 tag
 * [out] len  - 数据段的长度
 * [out] data - 分配 len，个字节，并 copy 数据
 *
 * 函数返回下一个区域的指针，即 src 偏移 1+2+len 长度后的位置
 */
extern uint8_t *zff_tld_r(uint8_t *src, uint8_t *tag, uint32_t *len, void *data);

/**
 * 这个函数会把 AVCodecContext 写入内存中，输出的内存中的结构为
 *
 *  E1              (表示 AVCodecContext 的数据)
 *  00              (空一个字节来对齐）
 *  00 00 9C 8A     (四个字节表示后面的长度)
 *  AE C3 90 .. 8A 5E C1 ... (表示数据，这个可以直接 memcpy 为 AVCodecContext)
 *  #----------------------------------------------------------------------
 *  # 接着判断一下数据是否超出了 E0 的 data 段长度
 *  # 如果没有超出怎根据读取的 TAG 来判断要修改 AVCodecContext 哪个字段
 *  # 之后的 TAG 有如下可能（统统 memcpy就好）:
 *  #----------------------------------------------------------------------
 *  E2    # rc_eq        : char *       : 长度为字符串长度+1容纳 \0
 *  #----------------------------------------------------------------------
 *  E3    # extradata    : uint8_t *    : 必须长度是 extradata_size
 *  #----------------------------------------------------------------------
 *  E4    # intra_matrix : uint16_t *   : 必须长度是 64
 *  #----------------------------------------------------------------------
 *  E5    # inter_matrix : uint16_t *   : 必须长度是 64
 *  #----------------------------------------------------------------------
 *  E6    # rc_override  : RcOverride * : 必须长度是
 *        #                         rc_override_count * sizeof(RcOverride)
 *
 * 函数 zff_avcodec_context_recover 将会从这片内存中恢复回一个 AVCodecContext 的实例
 * 返回一个新的指针指向新分配的内存，输出 sz 表示这片内存区域大大小。
 * 如果返回 NULL 表示失败，可能是没有更多内存了。
 */
extern uint8_t *zff_avcodec_context_w(int *out_size, AVCodecContext *src);

/**
 * 从内存中恢复一个 AVCodecContext 对象，它会为 AVCodecContext 重新分配内存，因此传入的内存可以释放了
 * 传入的参数 pTag 表示内存的起始位置，size 表示了这块内存的长度，函数读取到最后一个 TLD 会和总长度进行校验
 * 本函数将初始化传入的指针 ctx，如果一切正常，返回 0， 否则是一系列错误码（都是负值）
 */
extern int zff_avcodec_context_r(uint8_t* pTag, int size, AVCodecContext **ctx);
extern int zff_avcodec_context_rdata(uint8_t* pData,
        int size,
        AVCodecContext **ctx);

/**
 * 将一个 AVPacket 写入一块内存
 */
extern uint8_t *zff_avcodec_packet_w(int *out_size, AVPacket *src);

/**
 * 从一块内存中读出一个 AVPacket
 * 本函数将初始化传入的指针 pkt，如果一切正常，返回 0， 否则是一系列错误码（都是负值）
 */
extern int zff_avcodec_packet_r(uint8_t* pTag, int size, AVPacket **pkt);
extern int zff_avcodec_packet_rdata(uint8_t* pData, int size, AVPacket **pkt);

/**
 * 这个函数基本相当于从 ffmpeg 扒下来的代码，
 * 就是为了能完整复制一个 AVCodecContext 的实例
 * 返回 0 表示一切 OK, 否则返回一个负值表示错误号
 */
extern int zff_avcodec_context_copy(AVCodecContext *dest,
        const AVCodecContext *src);
extern void mp_copy_lav_codec_headers(AVCodecContext *avctx, AVCodecContext *st);
//--------------------------------------------------------
/**
 * 将一个 AVFrame （已经被转换成 RGB 格式的）写入到一个 ppm 文件里，以便察看
 */
extern int zff_save_rgb_frame_to_file(char *dest, AVFrame *fr);
//--------------------------------------------------------
#endif /* ZFFSPLIT_H_ */
