#include "dumpPkg.h"

SideData::SideData() {
	init();
}

SideData::SideData(const int size) {
	init();
	this->size = size;
	data = (uint8_t*)malloc(this->size);
	memset(data, 0x0, sizeof(data));
}

SideData::~SideData() {
	if(data) {
		free(data);
	}
}

void SideData::init() {
	data = NULL;
	size = 0;
	type = AV_PKT_DATA_PALETTE; 
}

template<class Archive>
void SideData::serialize(Archive &ar, const unsigned int version) {
	for(int i = 0; i < size; i++) {
		ar & data[i];
	}	
	ar & type;
}

template<class Archive>
inline void load_construct_data(Archive &ar, SideData *t, const unsigned int file_version) {
	int size;
	ar >> size;
	::new(t)SideData(size);
}

template<class Archive>
inline void save_construct_data(Archive &ar, const SideData *t, const unsigned int file_version) {
	ar & t->size;	
}

MyAVPacket::MyAVPacket() {
	init();	
}

MyAVPacket::MyAVPacket(const int size) {
	init();
	this->size = size;
	data = (uint8_t*)malloc(this->size);
	memset(data, 0x0, sizeof(data));
	//data = (uint8_t*)operator new(size);
}

MyAVPacket::~MyAVPacket() {
	if(data) {
		free(data);
	}
	//FIXME
}

void MyAVPacket::init() {
	pts = 0;
	dts = 0;
	data = NULL;
	size = 0;
	stream_index = -1;
	flags = -1;
	side_data = NULL;
	side_data_elems = 0;
	duration = 0;
	destruct = NULL;
	priv = NULL;
	pos = -1;
	convergence_duration = 0;
}

template<class Archive>
void MyAVPacket::serialize(Archive &ar, const unsigned int version) {
	ar & pts;
	ar & dts;
	for(int i = 0; i < size; i++) {
		ar & data[i]; 
	}
	ar & stream_index;
	ar & flags;
	ar & side_data;
	ar & side_data_elems;
	ar & duration;
	//FIXME
	ar & pos;
	ar & convergence_duration;
}

template<class Archive>
inline void load_construct_data(Archive &ar, MyAVPacket *t, const unsigned int file_version) {

	ar << t->size;
}

template<class Archive>
inline void save_construct_data(Archive &ar, MyAVPacket *t, const unsigned int file_version) {

	int size;
	ar >> size;
	::new(t)MyAVPacket(size);
}


Film::Film(string filePath) {
	m_filePath = filePath;	
	m_ic = NULL;
	m_videoIdx = -1;
	m_audioIdx = -1;
	m_quit = false;
	m_pktCount = 0;
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

	for(unsigned int i = 0; i < m_ic->nb_streams; i++) {
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

	while(!m_quit) {
		AVPacket *pkt = (AVPacket*)av_malloc(sizeof(AVPacket));
		av_init_packet(pkt);

		int ret = av_read_frame(m_ic, pkt);				
		if(ret < 0) {
			if(ret == AVERROR_EOF || url_feof(m_ic->pb))
				break;
			if(m_ic->pb && m_ic->pb->error)
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

void Film::push_pkt(AVPacket *pkt) {
	m_pktCount++;

	cout << "push pkt: "
			 << " NO! " << m_pktCount
			 << " size: " << pkt->size
			 << " type: " << ((pkt->stream_index == m_videoIdx) ? "video" : "audio")
			 << endl;

	m_pktQueue.push(pkt);	
}


Cinema::Cinema() {
	initDevice();
}

Cinema::~Cinema() {

}

void Cinema::initDevice() {
	
	XInitThreads();
	avcodec_register_all();
	avdevice_register_all();
	avfilter_register_all();
	av_register_all();
	avformat_network_init();

	m_screenWidth = 640;
	m_screenHeight = 480;

}

void Cinema::playFilm(Film *film) {
	m_film = film;	
	m_film->play();
}










