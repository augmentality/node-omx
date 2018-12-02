#pragma once

#define DVD_TIME_BASE 1000000
#define DVD_NOPTS_VALUE    (-1LL<<52)

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

struct FFFrame
{
    bool video = false;
    bool loopedVideo = false;
    bool loopedAudio = false;
    AVPacket * pkt = nullptr;
    AVFrame * frame = nullptr;
    int audioDataSize = 0;
    uint8_t *  convertedAudio = nullptr;
    int convertedSize = 0;
    double pts = DVD_NOPTS_VALUE;
    double dts = DVD_NOPTS_VALUE;
    double duration = 0;
};
