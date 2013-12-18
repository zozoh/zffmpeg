ffsplit|ffcombine 使用介绍
============

# ffsplit 将视频逐包拆分

    $:> ffsplit /path/to/video.mp4 /path/to/dest
    ffsplit v1.0 || /path/to/video.mp4
    ------------------------------------------
    dump to : /path/to/dest
    video_head
    : 00000000000000
    : 00000000000001
    ...
    : 00000078aec579
    ------------------------------------------
    dump 89213 AVPacket

 * 增加 `-play` 为一边拆分一边播放

# ffcombine 读取拆分的视频目录进行播放


    $:> ffcombine /path/to/dest
    ffcombine v1.0 || /path/to/dest
    ------------------------------------------
    ... 一些 Video 的信息 ...
    : 00000000000000
    : 00000000000001
    ...
    : 00000078aec579
    ------------------------------------------
    done


