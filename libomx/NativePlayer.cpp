#include "NativePlayer.h"
#include "ffsource/FFSource.h"
#include "audioBlock.h"
#include "videoBlock.h"
#include <stdexcept>

NativePlayer::NativePlayer(std::string url)
{
    try
    {
        src = new FFSource(url);
    }
    catch(FFException e)
    {
        throw std::runtime_error(e.errorMessage);
    }
    if (this->src->hasVideo && this->src->codingType != OMX_VIDEO_CodingAVC)
    {
        this->src->hasVideo = false;
    }
    this->hasAudio = false;
    if (!this->src->hasVideo && !this->src->hasAudio)
    {
        delete src;
        throw std::runtime_error(std::string("No playable streams"));
    }

    this->client = new ILClient();

    this->clock = new ClockComponent(client);
    this->clock->setTimeScale(1.0);

    if (src->hasAudio)
    {
        this->audioThread = new AudioThread(this->client, this->clock, this->src);
    }
    if (src->hasVideo)
    {
        this->videoThread = new VideoThread(this->client, this->clock, this->src);
    }

}
void NativePlayer::play()
{
    if (this->audioThread != nullptr)
    {
        this->audioThread->start();
    }
    if (this->videoThread != nullptr)
    {
        this->videoThread->start();
    }
    playThread = std::thread(&NativePlayer::playThreadFunc, this);
}
void NativePlayer::playThreadFunc()
{
    clock->changeState(OMX_StateExecuting);
    playing = true;
    playState = 1;
    FFFrame * frame = new FFFrame();
    while(src->getPacket(frame) >= 0 && playing)
    {
        if (frame->video && this->videoThread != nullptr)
        {
            VideoBlock * vb = new VideoBlock();

            vb->dataSize = frame->pkt->size;
            vb->data = new uint8_t[vb->dataSize];
            vb->pts = frame->pts;
            vb->dts = frame->dts;
            vb->looped = frame->loopedVideo;
            if (vb->looped)
            {
                frame->loopedVideo = false;
            }
            memcpy(vb->data, frame->pkt->data, vb->dataSize);
            {
                this->videoThread->addData(vb);
            }
        }
        else if (!frame->video && this->audioThread != nullptr)
        {
            AudioBlock * ab = new AudioBlock();

            int num_streams = 0;
            while (num_streams < AV_NUM_DATA_POINTERS &&
                   frame->frame->data[num_streams] != NULL)
            {
                num_streams++;
            }

            ab->audioFormat = frame->frame->format;
            ab->dataSize = frame->convertedSize;
            ab->sampleRate = frame->frame->sample_rate;
            ab->streamCount = num_streams;
            ab->channels = frame->frame->channels;
            ab->data = new uint8_t[frame->convertedSize];
            ab->sampleCount = frame->frame->nb_samples;
            ab->pts = frame->pts;
            ab->dts = frame->dts;
            ab->looped = frame->loopedAudio;
            if (ab->looped)
            {
                frame->loopedAudio = false;
            }
            memcpy(ab->data, frame->convertedAudio, ab->dataSize);
            {
                this->audioThread->addData(ab);
            }
        }
    }
}
NativePlayer::~NativePlayer()
{
    this->playing = false;
    this->playThread.join();
    if (this->src != nullptr)
    {
        delete this->src;
    }
    if (this->clock != nullptr)
    {
        delete this->clock;
    }
    if (this->audioThread != nullptr)
    {
        delete this->audioThread;
    }
    if (this->videoThread != nullptr)
    {
        delete this->videoThread;
    }
}