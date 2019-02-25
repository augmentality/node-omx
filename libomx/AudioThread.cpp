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


#include "AudioThread.h"

#define AUDIO_QUEUE_SIZE 2000

AudioThread::AudioThread(ILClient * pClient, ClockComponent * pClock, FFSource * src):
        client(pClient), clock(pClock)
{
    this->arc = new AudioRenderComponent(this->client);
    this->clockAudioTunnel = this->clock->tunnelTo(81, this->arc, 101, 0, 0);
    this->arc->setPCMMode(src->sampleRate, src->channels, AVSampleFormat::AV_SAMPLE_FMT_S16);
    this->arc->changeState(OMX_StateIdle);
    this->arc->enablePortBuffers(100, nullptr, nullptr, nullptr);
    this->arc->setAudioDest("hdmi");
}

void AudioThread::start()
{
    this->audioThread = std::thread(&AudioThread::audioThreadFunc, this);
}
void AudioThread::waitForCompletion()
{
    waitingForEnd = true;
    {
        std::unique_lock <std::mutex> lk(audioPlayingMutex);
    }
}
void AudioThread::stop()
{
    waitForBuffer();
    {
        waitingForEnd = true;
        std::unique_lock<std::mutex> lk(audioQueueMutex);

        // Empty queue
        while(audioQueue.size() > 0)
        {
            AudioBlock * b = audioQueue.front();
            audioQueue.pop();
            if (b != nullptr)
            {
                delete[] b->data;
                delete b;
            }
        }

        audioQueue.push(nullptr);
        bufferReady = false;
        audioReady.notify_one();
    }
    {
        std::unique_lock<std::mutex> lk(syncMutex);
        while (threadRunning)
        {
            playThreadFinished.wait_for(lk,  std::chrono::milliseconds(100), [&]()
            {
                return !threadRunning;
            });
        }
    }
}
void AudioThread::audioThreadFunc()
{
    {
        std::unique_lock<std::mutex> lk(syncMutex);
        threadRunning = true;
    }
    this->arc->changeState(OMX_StateExecuting);
    {
        std::unique_lock<std::mutex> lk(audioPlayingMutex);
        while (!playbackComplete)
        {
            AudioBlock * block = dequeue();
            if (block != nullptr)
            {
                int format = block->audioFormat;
                int readBytes = 0;
                size_t pSize = block->dataSize;
                bool error = false;

                uint64_t timestamp = (uint64_t)(
                        block->pts != DVD_NOPTS_VALUE ? block->pts : block->dts != DVD_NOPTS_VALUE ? block->dts : 0);

                size_t sample_size = pSize / (block->streamCount * block->sampleCount);

                OMX_ERRORTYPE r;
                OMX_BUFFERHEADERTYPE *buff_header = nullptr;
                int k, m, n;
                for (k = 0, n = 0; n < block->sampleCount; n++)
                {
                    if (k == 0)
                    {
                        buff_header = nullptr;
                        while(!playbackComplete && buff_header == nullptr)
                        {
                            buff_header = arc->getInputBuffer(100, 0 /* block */);
                            if (buff_header == nullptr)
                            {
                                usleep(10000);
                            }
                        }
                        if (playbackComplete)
                        {
                            buff_header = nullptr;
                            break;
                        }
                    }
                    if (playbackComplete)
                    {
                        break;
                    }
                    memcpy(&buff_header->pBuffer[k], &block->data[n * sample_size], sample_size);
                    k += sample_size;
                    if (k >= buff_header->nAllocLen)
                    {
                        // this buffer is full

                        buff_header->nFilledLen = k;

                        if (block->pts == DVD_NOPTS_VALUE && block->dts == DVD_NOPTS_VALUE)
                        {
                            buff_header->nFlags |= OMX_BUFFERFLAG_TIME_UNKNOWN;
                        }
                        else
                        {
                            if (block->pts == DVD_NOPTS_VALUE)
                            {
                                buff_header->nFlags |= OMX_BUFFERFLAG_TIME_IS_DTS;
                            }
                            buff_header->nTimeStamp = ToOMXTime(timestamp);
                        }

                        r = arc->emptyBuffer(buff_header);
                        if (r != OMX_ErrorNone)
                        {
                            fprintf(stderr, "Empty buffer error\n");
                        }
                        k = 0;
                        buff_header = NULL;
                    }
                }
                if (!playbackComplete && buff_header != NULL)
                {
                    buff_header->nFilledLen = k;

                    if (block->pts == DVD_NOPTS_VALUE && block->dts == DVD_NOPTS_VALUE)
                    {
                        buff_header->nFlags |= OMX_BUFFERFLAG_TIME_UNKNOWN;
                    }
                    else
                    {
                        if (block->pts == DVD_NOPTS_VALUE)
                        {
                            buff_header->nFlags |= OMX_BUFFERFLAG_TIME_IS_DTS;
                        }
                        buff_header->nTimeStamp = ToOMXTime(timestamp);
                    }

                    r = arc->emptyBuffer(buff_header);
                    if (r != OMX_ErrorNone)
                    {
                        fprintf(stderr, "Empty buffer error %s\n");
                    }
                }
                delete[] block->data;
                delete block;
            }
            else
            {
                if (waitingForEnd)
                {
                    playbackComplete = true;
                }
            }
        }
    }

    OMX_BUFFERHEADERTYPE * buff_header = arc->getInputBuffer(100, 1 /* block */);
    if (buff_header != nullptr)
    {
        buff_header->nOffset = 0;
        buff_header->nFilledLen = 0;
        buff_header->nTimeStamp = ToOMXTime(0LL);

        buff_header->nFlags = OMX_BUFFERFLAG_ENDOFFRAME | OMX_BUFFERFLAG_EOS | OMX_BUFFERFLAG_TIME_UNKNOWN;
        arc->emptyBuffer(buff_header);
    }
    {
        std::unique_lock<std::mutex> lk(syncMutex);
        playThreadFinished.notify_one();
        threadRunning = false;
    }
}

AudioBlock * AudioThread::dequeue()
{
    std::unique_lock<std::mutex> lk(audioQueueMutex);
    if (audioQueue.empty())
    {
        while (!playbackComplete && audioQueue.empty())
        {
            if (audioQueue.empty() && waitingForEnd)
            {
                playbackComplete = true;
                return nullptr;
            }
            audioReady.wait_for(lk,  std::chrono::milliseconds(100), [&]()
            {
                return !audioQueue.empty();
            });
        }
    }
    if (audioQueue.empty() && waitingForEnd)
    {
        playbackComplete = true;
        return nullptr;
    }
    if (playbackComplete || audioQueue.empty())
    {
        return nullptr;
    }
    else
    {
        AudioBlock * b = audioQueue.front();
        audioQueue.pop();
        if (audioQueue.size() < AUDIO_QUEUE_SIZE && !bufferReady)
        {
            std::unique_lock<std::mutex> lk(readyForDataMutex);
            bufferReady = true;
            readyForData.notify_one();
        }
        return b;
    }
}
void AudioThread::waitForBuffer()
{
    std::unique_lock<std::mutex> lk(this->readyForDataMutex);
    if (bufferReady)
    {
        return;
    }
    while (!playbackComplete && !bufferReady)
    {
        readyForData.wait_for(lk,  std::chrono::milliseconds(100), [&]()
        {
            return bufferReady;
        });
    }
}
void AudioThread::addData(AudioBlock * ab)
{
    waitForBuffer();
    {
        std::unique_lock<std::mutex> lk(audioQueueMutex);
        audioQueue.push(ab);
        if (audioQueue.size() >= AUDIO_QUEUE_SIZE)
        {
            bufferReady = false;
        }
        audioReady.notify_one();
    }
}
AudioThread::~AudioThread()
{
    bool isRunning = false;
    {
        std::unique_lock<std::mutex> lk(syncMutex);
        isRunning = threadRunning;
    }
    if (isRunning)
    {
        stop();
    }
    this->playbackComplete = true;
    if (this->audioThread.joinable())
    {
        this->audioThread.join();
    }
    // Empty queue
    while(audioQueue.size() > 0)
    {
        AudioBlock * b = audioQueue.front();
        audioQueue.pop();
        if (b != nullptr)
        {
            delete[] b->data;
            delete b;
        }
    }

    if (this->clockAudioTunnel != nullptr)
    {
        delete this->clockAudioTunnel;
        this->clockAudioTunnel = nullptr;
    }

    this->arc->disablePortBuffers(100, nullptr, nullptr, nullptr);

    if (this->arc != nullptr)
    {
        delete this->arc;
        this->arc = nullptr;
    }
}