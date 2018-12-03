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

    // Prebuffer some frames
    for (int x = 0; x < 100; x++)
    {
        prebuffer.push(getNextBlock(false));
    }
}
void NativePlayer::play()
{
    if (playState == 2)
    {
        clock->changeState(OMX_StateExecuting);
        this->setSpeed(playSpeed);
        playState = 1;
    }
    else if (playState == 0)
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
}
void NativePlayer::pause()
{
    if (playState == 1)
    {
        playState = 2;
        this->clock->setTimeScale(0.0f);
        clock->changeState(OMX_StateIdle);
    }
}
void NativePlayer::setSpeed(float scale)
{
    playSpeed = scale;
    this->clock->setTimeScale(scale);
}
double NativePlayer::getTime()
{
    return clock->getTime();
}
PrebufferBlock * NativePlayer::getNextBlock(bool fromPrebuffer)
{
    if (fromPrebuffer && prebuffer.size() > 0)
    {
        // Empty the prebuffer first
        PrebufferBlock * blk = prebuffer.front();
        prebuffer.pop();
        return blk;
    }
    if (frame == nullptr)
    {
        frame = new FFFrame();
    }
    if (src->getPacket(frame) < 0)
    {
        return nullptr;
    }
    if (frame->video)
    {
        VideoBlock * vb = new VideoBlock();

        vb->dataSize = frame->pkt->size;
        vb->data = new uint8_t[vb->dataSize];
        vb->pts = frame->pts;
        vb->dts = frame->dts;
        vb->looped = frame->loopedVideo;
        vb->duration = frame->duration;
        if (vb->looped)
        {
            frame->loopedVideo = false;
        }
        memcpy(vb->data, frame->pkt->data, vb->dataSize);
        PrebufferBlock * prebuf = new PrebufferBlock();
        prebuf->video = true;
        prebuf->block = (void *)vb;
        return prebuf;
    }
    else
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
        ab->duration = frame->duration;
        if (ab->looped)
        {
            frame->loopedAudio = false;
        }
        memcpy(ab->data, frame->convertedAudio, ab->dataSize);
        PrebufferBlock * prebuf = new PrebufferBlock();
        prebuf->video = false;
        prebuf->block = (void *)ab;
        return prebuf;
    }
}
void NativePlayer::playThreadFunc()
{
    clock->changeState(OMX_StateExecuting);
    playing = true;
    playState = 1;
    PrebufferBlock * block = getNextBlock(true);
    while(block != nullptr && playing)
    {
        if (block->video)
        {
            if (this->videoThread != nullptr)
            {
                this->videoThread->addData((VideoBlock *)block->block);
            }
            else
            {
                VideoBlock * vb = (VideoBlock *)block->block;
                delete[] vb->data;
                delete vb;
            }
        }
        else
        {
            if (this->audioThread != nullptr)
            {
                this->audioThread->addData((AudioBlock *)block->block);
            }
            else
            {
                AudioBlock * ab = (AudioBlock *)block->block;
                delete[] ab->data;
                delete ab;
            }
        }
        delete block;
        block = getNextBlock(true);
    }
    if (block != nullptr)
    {
        if (block->video)
        {
            VideoBlock * vb = (VideoBlock *)block->block;
            delete[] vb->data;
            delete vb;
        }
        else
        {
            AudioBlock * ab = (AudioBlock *)block->block;
            delete[] ab->data;
            delete ab;
        }
        delete block;
    }
}
NativePlayer::~NativePlayer()
{
    this->playing = false;
    this->playThread.join();
    if (frame != nullptr)
    {
        if (frame->frame != nullptr)
        {
            av_frame_free(&frame->frame);
            frame->frame = nullptr;
        }
        if (frame->convertedAudio != nullptr)
        {
            delete[] frame->convertedAudio;
            frame->convertedAudio = nullptr;
        }
        delete frame;
    }
    while (prebuffer.size() > 0)
    {
        PrebufferBlock * blk = prebuffer.front();
        prebuffer.pop();
        if (blk->video)
        {
            VideoBlock * vb = (VideoBlock *)blk->block;
            delete[] vb->data;
            delete vb;
        }
        else
        {
            AudioBlock * ab = (AudioBlock *)blk->block;
            delete[] ab->data;
            delete ab;
        }
        delete blk;
    }
    if (this->src != nullptr)
    {
        delete this->src;
    }
    if (this->audioThread != nullptr)
    {
        delete this->audioThread;
    }
    if (this->videoThread != nullptr)
    {
        delete this->videoThread;
    }
    if (this->clock != nullptr)
    {
        delete this->clock;
    }
    if (this->client != nullptr)
    {
        delete this->client;
    }
}