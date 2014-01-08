用 ffserver 搭建流媒体
=====

## ffmpeg 编译

必须确保安装 libx264

    zozoh@zozoh-va:~/tmp/ffserver$ apt-cache search libx264
    libx264-120 - x264 video coding library
    libx264-dev - development files for libx264
    zozoh@zozoh-va:sudo apt-get install libx264-dev

ffmpeg 的编译参数如下:

    zozoh@zozoh-va:~/opt/ffmpeg/ffmpeg_xplay$ ./ffmpeg
    ffmpeg version 1.0.git Copyright (c) 2000-2012 the FFmpeg developers
      built on Jan  7 2014 18:56:20 with gcc 4.6 (Ubuntu/Linaro 4.6.3-1ubuntu5)
      configuration: --enable-gpl --enable-shared --enable-debug --enable-libx264
      WARNING: library configuration mismatch
      postproc    configuration: --extra-version='4:0.8.9-0ubuntu0.12.04.1' --arch=i386 --prefix=/usr --libdir=/usr/lib/i386-linux-gnu --enable-vdpau --enable-bzlib --enable-libgsm --enable-libschroedinger --enable-libspeex --enable-libtheora --enable-libvorbis --enable-pthreads --enable-zlib --enable-libvpx --enable-runtime-cpudetect --enable-libfreetype --enable-vaapi --enable-gpl --enable-postproc --enable-swscale --enable-x11grab --enable-libdc1394 --shlibdir=/usr/lib/i386-linux-gnu/i686/cmov --cpu=i686 --enable-shared --disable-static
      libavutil      52.  9.102 / 52.  9.102
      libavcodec     54. 77.100 / 54. 77.100
      libavformat    54. 38.100 / 54. 38.100
      libavdevice    54.  3.100 / 54.  3.100
      libavfilter     3. 23.103 /  3. 23.103
      libswscale      2.  1.102 /  2.  1.102
      libswresample   0. 17.101 /  0. 17.101
      libpostproc    52.  2.100 / 52.  0.  0
    Hyper fast Audio and Video encoder
    usage: ffmpeg [options] [[infile options] -i infile]... {[outfile options] outfile}...


## ffserver 的配置文件

    -------------------------- Begin Copy ----------------------
    Port 8090
    BindAddress 0.0.0.0
    MaxHTTPConnections 2000
    MaxClients 1000
    MaxBandwidth 10000000
    CustomLog -
    NoDaemon

    <Feed feed1.ffm>
    File /home/zozoh/tmp/ffserver/tmp/feed1.ffm
    FileMaxSize 2MB
    ACL allow 127.0.0.1
    </Feed>

    <Stream test.flv>
    Feed feed1.ffm
    Format flv 

    #audio
    AudioBitRate  32  
    AudioChannels 2
    AudioSampleRate  44100
    AVOptionAudio flags +global_header
    #AudioCodec libmp3lame 

    #video
    VideoBitRate 2M
    VideoBufferSize 10000
    VideoFrameRate 25
    VideoBitRateTolerance 100 
    VideoSize 1024x768
    VideoGopSize 25
    VideoCodec libx264 
    StartSendOnKey
    AVOptionVideo qmin 3
    AVOptionVideo qmax 31
    AVOptionVideo flags +global_header
    AVOptionVideo keyint_min 25
    AVOptionVideo qcomp 0.6 
    AVOptionVideo qdiff 4
    AVOptionVideo me_range 16
    Preroll 600 
    </Stream>

    <Stream stat.html>
    Format status
    ACL allow localhost
    ACL allow 192.168.0.0 192.168.255.255
    </Stream>
    --------------------------- End Copy -----------------------

## ffmpeg 的启动参数

    ./ffmpeg  -re  \
          -fix_sub_duration \
          -i /path/to/video  \
          -vcodec libx264 \
          -qmin 3 \
          -qmax 31 \
          -qdiff 4 \
          -me_range 16 \
          -keyint_min 25 \
          -qcomp 0.6 \
          -b:v 9000K \
          http://localhost:8090/feed1.ffm




