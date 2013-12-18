#include "dumpPkg.h"

#include <iostream>

Film::Film(string filePath) {
	m_filePath = filePath;	
	m_ic = NULL;
	m_videoIdx = -1;
	m_audioIdx = -1;
	m_quit = false;
}

Film::~Film() {

}

void Film::open() {
	init();

	if(avformat_open_input(&m_ic, m_filePath.c_str(), NULL, NULL) != 0) {
		cerr << "open file error" << endl;
		exit(1);
	}

	if(avformat_find_stream_info(m_ic, NULL) < 0) {
		cerr << "find stream info error" << endl;
		exit(1);
	}
	
	av_dump_format(m_ic, 0, m_filePath.c_str(), 0);

	for(int i = 0; i < m_ic->nb_streams; i++) {
		if(m_audioIdx < 0 && m_ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)			m_audioIdx = i; 
		if(m_videoIdx < 0 && m_ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			m_videoIdx = i;
	}

	if(m_videoIdx < 0 && m_audioIdx < 0) {
		cerr << "stream index error" << endl;
		exit(1);
	}

	if(m_videoIdx >= 0) 
		stream_open(m_videoIdx);
	if(m_audioIdx >= 0)
		stream_open(m_audioIdx);
}

bool Film::stream_open(int streamIdx) {
	AVCodecContext *codecCtx;
	AVCodec *codec;

	codecCtx = m_ic->streams[streamIdx]->codec;

	codec = avcodec_find_decoder(codecCtx->codec_id);
	if(!codec || (avcodec_open2(codecCtx, codec, NULL) < 0)) {
		cerr << "open codec error" << endl;
		return false;
	}

	switch(codecCtx->codec_type) {
	case AVMEDIA_TYPE_AUDIO:
		//TODO:
		m_audioStream = m_ic->streams[streamIdx];
		break;
	case AVMEDIA_TYPE_VIDEO:
		m_videoStream = m_ic->streams[streamIdx];
		break;
	default:
		break;
	}

	return true;
}

void Film::init() {
	
}

void Film::play() {
	open();

	while(m_quit) {
		AVPacket *pkt = av_malloc(sizeof(AVPacket));
		av_init_packet(pkt);

		int ret = av_read_frame(m_ic, pkt);				
		if(ret < 0) {
			if(ret == AVERROR_EOF || url_eof(m_ic->pb))
				break;
			if(m_ic->pb && m_ic->pb_error)
				break;
			continue;
		}	
		if(pkt->stream_index == m_videoIdx || pkt->stream_index == m_audioIdx) {
			//TODO:	
			push_pkt(pkt);
		} else {
			av_free_packet(pkt);
		}
	}
}

void Film::push_pkt(int *pkt) {
	m_pktQueue.push(pkt);	
}


Cinema::Cinema() {
	init();
}

Cinema::~Cinema() {

}

void Cinema::initDevice() {
	avcodec_register_all();
	avdevice_register_all();
	avfilter_register_all();
	av_register_all();
	avformat_network_init();

	XInitThreads();
	
	m_screenWidth = 640;
	m_screenHeight = 480;

}

void Cinema::playFilm(Film *film) {
	m_film = film;	
}










