//
// node-omx - A media player for node.js on the Raspberry Pi
// Copyright (C) 2018 Augmentality Ltd <info@augmentality.uk>
//
// This file is part of node-omx.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; version 2.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
// Street, Fifth Floor, Boston, MA 02110-1301, USA.


#include "NativePlayer.h"
#include "ffsource/FFSource.h"
#include "audioBlock.h"
#include "videoBlock.h"
#include <stdexcept>


NativePlayer::NativePlayer(std::string url, std::function<void()> pCompletedFunction):
    playbackComplete(pCompletedFunction),
    playerURL(url)
{
    controlThread = std::thread(&NativePlayer::controlThreadFunc, this);
}
void NativePlayer::play()
{
    ControlQueueCommand * cmd = new ControlQueueCommand();
    cmd->commandType = ControlQueueCommandType::Play;
    std::unique_lock<std::mutex> lk(controlMutex);
    controlCommandQueue.push(cmd);
    controlCommandReady.notify_one();
}
void NativePlayer::pause()
{
    ControlQueueCommand * cmd = new ControlQueueCommand();
    cmd->commandType = ControlQueueCommandType::Pause;
    std::unique_lock<std::mutex> lk(controlMutex);
    controlCommandQueue.push(cmd);
    controlCommandReady.notify_one();
}
void NativePlayer::setSpeed(float scale)
{
    ControlQueueCommand * cmd = new ControlQueueCommand();
    cmd->commandType = ControlQueueCommandType::SetSpeed;
    cmd->floatData = scale;
    std::unique_lock<std::mutex> lk(controlMutex);
    controlCommandQueue.push(cmd);
    controlCommandReady.notify_one();
}
void NativePlayer::setLoop(bool loop)
{
    ControlQueueCommand * cmd = new ControlQueueCommand();
    cmd->commandType = ControlQueueCommandType::SetLoop;
    cmd->boolData = loop;
    std::unique_lock<std::mutex> lk(controlMutex);
    controlCommandQueue.push(cmd);
    controlCommandReady.notify_one();
}
double NativePlayer::getTime()
{
    std::unique_lock<std::mutex> lk(controlMutex);
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
void NativePlayer::controlThreadFunc()
{
    try
    {
        src = new FFSource(playerURL);
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
        src = nullptr;
        throw std::runtime_error(std::string("No playable streams"));
    }

    this->client = new ILClient();

    this->clock = new ClockComponent(client);
    this->clock->setTimeScale(1.0);

    if (src->hasAudio && !this->skipAudio)
    {
        this->audioThread = new AudioThread(this->client, this->clock, this->src, this->skipVideo);
    }
    if (src->hasVideo && !this->skipVideo)
    {
        this->videoThread = new VideoThread(this->client, this->clock, this->src);
    }

    // Prebuffer some frames
    for (int x = 0; x < 100; x++)
    {
        prebuffer.push(getNextBlock(false));
    }
    bool completed = false;
    while(this->nativePlayerActive)
    {
        std::unique_lock<std::mutex> lk(controlMutex);
        while (this->nativePlayerActive && controlCommandQueue.empty())
        {
            if (controlCommandQueue.empty() && !this->nativePlayerActive)
            {
                break;
            }
            controlCommandReady.wait_for(lk,  std::chrono::milliseconds(100), [&]()
            {
                return !controlCommandQueue.empty();
            });
        }
        if (!controlCommandQueue.empty())
        {
            ControlQueueCommand * cmd = controlCommandQueue.front();
            controlCommandQueue.pop();
            switch(cmd->commandType)
            {
                case ControlQueueCommandType::Pause:
                    if (playState == 1)
                    {
                        playState = 2;
                        this->clock->setTimeScale(0.0f);
                        clock->changeState(OMX_StateIdle);
                    }
                    break;
                case ControlQueueCommandType::SetSpeed:
                    playSpeed = cmd->floatData;
                    this->clock->setTimeScale(playSpeed);
                    break;
                case ControlQueueCommandType::Play:
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
                    break;
                case ControlQueueCommandType::SetLoop:
                    if (this->src != nullptr)
                    {
                        this->src->setLoop(cmd->boolData);
                    }
                    break;
                case ControlQueueCommandType::Stop:
                    this->nativePlayerActive = false;
                    break;
                case ControlQueueCommandType::Completed:
                    this->nativePlayerActive = false;
                    completed = true;
                    break;
            }
            delete cmd;
        }
    }
    printf("PlayThread Join?\n"); fflush(stdout);
    if (playThread.joinable())
    {
        printf("Joining\n"); fflush(stdout);
        this->playing = false;
        playThread.join();
    }
    if (this->src != nullptr)
    {
        printf("Delete src\n"); fflush(stdout);
        delete this->src;
        this->src = nullptr;
    }
    printf("Clearing prebuffer\n"); fflush(stdout);
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
    if (this->audioThread != nullptr)
    {
        this->audioThread->stop(!completed);
    }
    if (this->videoThread != nullptr)
    {
        this->videoThread->stop(!completed);
    }
    if (this->audioThread != nullptr)
    {
        delete this->audioThread;
        this->audioThread = nullptr;
    }
    if (this->videoThread != nullptr)
    {
        delete this->videoThread;
        this->videoThread = nullptr;
    }
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
    if (this->clock != nullptr)
    {
        delete this->clock;
        this->clock = nullptr;
    }
    if (this->client != nullptr)
    {
        delete this->client;
        this->client = nullptr;
    }
    {
        std::unique_lock<std::mutex> lk(controlMutex);
        while (!controlCommandQueue.empty())
        {
            ControlQueueCommand * cmd = controlCommandQueue.front();
            controlCommandQueue.pop();
            delete cmd;
        }
    }
    playbackComplete();
}
void NativePlayer::playThreadFunc()
{
    clock->changeState(OMX_StateExecuting);
    this->playing = true;
    playState = 1;
    PrebufferBlock * block = getNextBlock(true);
    while(block != nullptr && this->playing)
    {
        if (block->video)
        {
            if (this->videoThread != nullptr && !this->skipVideo)
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
            if (this->audioThread != nullptr && !this->skipAudio)
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
    if (!skipAudio)
    {
        this->audioThread->addData((AudioBlock *)nullptr);
    }
    if (!skipVideo)
    {
        this->videoThread->addData((VideoBlock *)nullptr);
    }
    this->playing = false;
    if (this->nativePlayerActive)
    {
        ControlQueueCommand * cmd = new ControlQueueCommand();
        cmd->commandType = ControlQueueCommandType::Completed;
        std::unique_lock<std::mutex> lk(controlMutex);
        controlCommandQueue.push(cmd);
        controlCommandReady.notify_one();
    }
}
void NativePlayer::waitForCompletion()
{
    if (this->audioThread != nullptr)
    {
        this->audioThread->waitForCompletion();
    }
    if (this->videoThread != nullptr)
    {
        this->videoThread->waitForCompletion();
    }
}
NativePlayer::~NativePlayer()
{
    if (controlThread.joinable())
    {
        {
            ControlQueueCommand * cmd = new ControlQueueCommand();
            cmd->commandType = ControlQueueCommandType::Stop;
            std::unique_lock<std::mutex> lk(controlMutex);
            controlCommandQueue.push(cmd);
            controlCommandReady.notify_one();
        }
        controlThread.join();
    }
}