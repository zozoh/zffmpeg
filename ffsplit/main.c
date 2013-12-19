#include <stdio.h>
#include <stdlib.h>

#include <zclib/log.h>
#include <zclib/util.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/avstring.h>
#include <libavutil/pixfmt.h>
#include <libavutil/log.h>
#include <libavutil/common.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
#include <libavutil/avassert.h>
#include <libavformat/avio.h>

//------------------------------------------------------------------------
typedef struct FFSplitContext
{
	char *vph;
	char *dph;
	AVFormatContext *fmtctx;

	AVStream *a;
	int a_index;

	AVStream *v;
	int v_index;

} FFSplitContext;
//------------------------------------------------------------------------

// 打开视频，初始化 AVFormatContext
void _open_video(FFSplitContext *sc)
{
	// 打开格式上下文
	sc->fmtctx = avformat_alloc_context();
	AVDictionary *opts = NULL;
	avformat_open_input(&sc->fmtctx, sc->vph, NULL, &opts);

	// 初始化 ffmpeg 流格式上下文
	if (avformat_find_stream_info(sc->fmtctx, NULL ) < 0)
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
			sc->a = s;
			sc->a_index = i;
			break;
		case AVMEDIA_TYPE_AUDIO:
			sc->v = s;
			sc->v_index = i;
			break;
		}
	}
	printf("\n");
}

void _write_packet_to_file(const char *dir, AVPacket *pkt)
{

}

// 循环读取视频的数据包
void _read_packets_from_video(FFSplitContext *sc)
{
	AVPacket *pkt = (AVPacket *) av_malloc(sizeof(AVPacket));
	int re = 0;

	printf("reading AVPackets ...\n");
	int i = 0;
	while (1)
	{
		memset(pkt, 0, sizeof(AVPacket));
		re = av_read_frame(sc->fmtctx, pkt);
		if (re == AVERROR_EOF || re < 0)
			break;

		printf("%08x [%d]: pos:%-9lld, sz:%-6ld, data:%p(%d) \n",
				i++,
				pkt->stream_index,
				pkt->pos,
				sizeof(AVPacket), pkt->data, pkt->size);
		//printf("%d : pos:%lld \n", i++, pkt->pos);
	}

	if (NULL != pkt)
	{
		av_free_packet(pkt);
		av_freep(&pkt);
	}
	printf(" %d AVPackets readed\n", i);

}

/**
 * 程序的入逻辑
 */
void ffsplit(FFSplitContext *sc)
{
	_open_video(sc);
	_find_streams(sc);
	_read_packets_from_video(sc);
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
	ffsplit(&sc);
	printf("\n-------------------------------------------------\n");

	// 返回成功
	return EXIT_SUCCESS;
}
