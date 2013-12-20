/*
 * Author: sijiewang
 * Date: 2013-12-18
 * E-mail: lnmcc@hotmail.com
 * Site: lnmcc.net
*/

#ifndef _DUMPPKG_H
#define _DUMPPKG_H

#include <string>
#include <queue>
#include <iostream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

extern "C" {
#ifdef __cplusplus
	#define __STDC_CONSTANT_MACROS
	#ifdef _STDIN_H
		#undef _STDIN_H
	#endif
	#include <stdint.h>
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/avstring.h>
#include <libavutil/pixfmt.h>
#include <libavutil/log.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <math.h>
}

using namespace std;

class SerializePkt {
	
public:
	SerializePkt();
	SerializePkt(AVPacket *pkt);

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive &ar, const unsigned int version); 

private:
	uint8_t *AVPacket_AVBufferRef_AVBuffer_data;		
	int AVPacket_AVBufferRef_AVBuffer_size;
	volatile int AVPacket_AVBufferRef_AVBuffer_refcount;
	void(*AVPacket_AVBufferRef_AVBuffer_free)(void *opaque, uint8_t *data);
	void *AVPacket_AVBufferRef_AVBuffer_opaque;
	int AVPacket_AVBufferRef_AVBuffer_flags;

	uint8_t AVPacket_AVBufferRef_data;
	int AVPacket_AVBufferRef size;

	int64_t AVPacket_pts;
	int64_t AVPacket_dts;

	uint8_t *AVPacket_data;
	int AVPacket_size;

	int AVPacket_stream_index;
	int AVPacket_flags;

	uint8_t *AVPacket_side_data_data;
	int AVPacket_size_data_size;
	enum AVPacketSideDataType  AVPacket_size_data_type;

	int AVPacket_side_data_elems;

	int AVPacket_duration;
	int64_t AVPacket_pos;
	int64_t AVPacket_convergence_duration;

	AVPacket *pkt;
};

class MyAVPacket {
public:
	MyAVPacket();

private:
	friend boost::serialization::access;
	template<class Archive>
	void serialize(Archive &ar, const unsigned int version); 

public:
    int64_t pts;
    int64_t dts;
    uint8_t *data;
    int   size;
    int   stream_index;
    int   flags;
    struct {
        uint8_t *data;
        int      size;
        enum AVPacketSideDataType type;
    } *side_data;
    int side_data_elems;

    int   duration;
    void  (*destruct)(struct AVPacket *);
    void  *priv;
    int64_t pos;                           
	int64_t convergence_duration;
};

class Film {

	public:
		Film(string filePath);
		~Film();
		void play();
		void close();
	
	private:
		void init();
		void open();
		bool stream_open(int streamIdx);
		void push_pkt(AVPacket *pkt);

	private:
		string m_filePath;
		AVFormatContext *m_ic; 

		AVStream *m_videoStream;
		AVStream *m_audioStream;

		int m_videoIdx;
		int m_audioIdx;

		bool m_quit;

		queue<AVPacket*> m_pktQueue;
		unsigned long m_pktCount;
};

class Cinema {

	public:
		Cinema();
		~Cinema();
		
		void playFilm(Film *film);
		
	private:
		void initDevice();

	private:
		unsigned int m_screenWidth;
		unsigned int m_screenHeight; 

		Film *m_film; 
};
#endif
