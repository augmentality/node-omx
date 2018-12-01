#pragma once

#include "ILComponent.h"
#include "ILClient.h"
#include <string>
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/samplefmt.h>
}

class AudioRenderComponent: public ILComponent
{
    public:
        explicit AudioRenderComponent(ILClient * client);
        void setPCMMode(int sampleRate, int channelCount, AVSampleFormat format);
        void setAudioDest(const char * dest);

};
