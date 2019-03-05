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


#include "VideoThread.h"

#define VIDEO_QUEUE_SIZE 500

VideoThread::VideoThread(ILClient * pClient, ClockComponent * pClock, FFSource * src):
    client(pClient), clock(pClock)
{
    this->vdc = new VideoDecodeComponent(this->client);
    this->vdr = new VideoRenderComponent(this->client);
    this->sched = new VideoSchedulerComponent(this->client);
    this->clockSchedTunnel = this->clock->tunnelTo(80, this->sched, 12, 0, 0);
    this->vdc->changeState(OMX_StateIdle);
    this->vdc->setVideoCompressionFormat(src->codingType, (OMX_COLOR_FORMATTYPE)0, src->fpsrate / src->fpsscale, src->width, src->height);
    this->vdc->enablePortBuffers(130, nullptr, nullptr, nullptr);
}

void VideoThread::start()
{
    this->videoThread = std::thread(&VideoThread::videoThreadFunc, this);
}
void VideoThread::waitForCompletion()
{
    bool isRunning = false;
    {
        std::unique_lock<std::mutex> lk(syncMutex);
        isRunning = threadRunning;
    }
    if (isRunning)
    {
        stop(false);
    }
    this->playbackComplete = true;
    if (this->videoThread.joinable())
    {
        this->videoThread.join();
    }
}
VideoBlock * VideoThread::dequeue()
{
    std::unique_lock<std::mutex> lk(videoQueueMutex);
    if (videoQueue.empty())
    {
        while (!playbackComplete && videoQueue.empty())
        {
            if (videoQueue.empty() && waitingForEnd)
            {
                playbackComplete = true;
                return nullptr;
            }
            videoReady.wait_for(lk,  std::chrono::milliseconds(100), [&]()
            {
                return !videoQueue.empty();
            });
        }
    }
    if (videoQueue.empty() && waitingForEnd)
    {
        playbackComplete = true;
        return nullptr;
    }
    VideoBlock * b = videoQueue.front();
    videoQueue.pop();
    if (videoQueue.empty() && waitingForEnd)
    {
        playbackComplete = true;
    }
    if (videoQueue.size() < VIDEO_QUEUE_SIZE && !bufferReady)
    {
        std::unique_lock<std::mutex> lk(readyForDataMutex);
        bufferReady = true;
        readyForData.notify_one();
    }
    if (b == nullptr)
    {
        playbackComplete = true;
    }
    return b;
}
void VideoThread::waitForBuffer()
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
void VideoThread::stop(bool immediately)
{
    waitForBuffer();
    {
        waitingForEnd = true;
        if (immediately)
        {
            std::unique_lock<std::mutex> lk(videoQueueMutex);
            while (!videoQueue.empty())
            {
                VideoBlock * b = videoQueue.front();
                videoQueue.pop();
                if (b != nullptr)
                {
                    delete[] b->data;
                    delete b;
                }
            }
            videoQueue.push(nullptr);
            bufferReady = false;
            videoReady.notify_one();
        }
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
void VideoThread::videoThreadFunc()
{
    {
        std::unique_lock<std::mutex> lk(syncMutex);
        threadRunning = true;
    }
    this->vdc->changeState(OMX_StateExecuting);
    OMX_BUFFERHEADERTYPE *buf = nullptr;
    int port_settings_changed = 0;
    int audio_port_settings_changed = 0;
    int first_packet = 1;
    unsigned int data_len = 0;
    {
        std::unique_lock<std::mutex> lk(videoPlayingMutex);
        while (!playbackComplete)
        {
            VideoBlock * block = dequeue();
            if (block != nullptr)
            {
                uint64_t timestamp = (uint64_t)(
                        block->pts != DVD_NOPTS_VALUE ? block->pts : block->dts != DVD_NOPTS_VALUE ? block->dts : 0);

                int readBytes = 0;
                int pSize = block->dataSize;
                bool error = false;
                while (pSize != 0)
                {

                    buf = nullptr;
                    while(!playbackComplete && buf == nullptr)
                    {
                        buf = vdc->getInputBuffer(130, (waitingForEnd) ? 1 : 0);
                        if (buf == nullptr)
                        {
                            usleep(10000);
                        }
                    }
                    if (playbackComplete) break;

                    if (buf == nullptr)
                        break;
                    buf->nFlags = 0;
                    buf->nOffset = 0;
                    buf->nFilledLen = (pSize > buf->nAllocLen) ? buf->nAllocLen : pSize;
                    memcpy(buf->pBuffer, block->data + readBytes, buf->nFilledLen);
                    pSize -= buf->nFilledLen;
                    readBytes += buf->nFilledLen;
                    data_len += buf->nFilledLen;
                    if (port_settings_changed == 0 &&
                        ((data_len > 0 && vdc->removeEvent(OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
                         (data_len == 0 && vdc->waitForEvent(OMX_EventPortSettingsChanged, 131, 0, 0, 1,
                                                             ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) ==
                                           0)))
                    {
                        port_settings_changed = 1;

                        decodeTunnel = vdc->tunnelTo(131, sched, 10, 0, 0);
                        schedTunnel = sched->tunnelTo(11, vdr, 90, 0, 1000);
                        sched->changeState(OMX_StateExecuting);
                        vdr->changeState(OMX_StateExecuting);
                    }

                    if (first_packet)
                    {
                        buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
                        first_packet = 0;
                    }
                    else
                    {
                        if (block->pts == DVD_NOPTS_VALUE && block->dts == DVD_NOPTS_VALUE)
                        {
                            buf->nFlags |= OMX_BUFFERFLAG_TIME_UNKNOWN;
                        }
                        else
                        {
                            if (block->pts == DVD_NOPTS_VALUE)
                            {
                                buf->nFlags |= OMX_BUFFERFLAG_TIME_IS_DTS;
                            }


                            if (!(buf->nFlags & OMX_BUFFERFLAG_TIME_UNKNOWN))
                            {
                                buf->nTimeStamp = ToOMXTime(timestamp);
                            }
                        }
                    }
                    data_len = 0;

                    if (vdc->emptyBuffer(buf) != OMX_ErrorNone)
                    {
                        error = true;
                        break;
                    }

                }
                delete[] block->data;
                delete block;
                if (error)
                {
                    break;
                }
                if (buf == nullptr)
                {
                    return;
                }
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
    OMX_BUFFERHEADERTYPE * buff_header = vdc->getInputBuffer(130, 1 /* block */);
    if (buff_header != nullptr)
    {
        buff_header->nOffset = 0;
        buff_header->nFilledLen = 0;
        buff_header->nTimeStamp = ToOMXTime(0LL);

        buff_header->nFlags = OMX_BUFFERFLAG_ENDOFFRAME | OMX_BUFFERFLAG_EOS | OMX_BUFFERFLAG_TIME_UNKNOWN;
        vdc->emptyBuffer(buff_header);
        vdr->waitForEOS();
    }
    {
        std::unique_lock<std::mutex> lk(syncMutex);
        playThreadFinished.notify_one();
        threadRunning = false;
    }
}
void VideoThread::addData(VideoBlock * vb)
{
    waitForBuffer();
    {
        std::unique_lock<std::mutex> lk(videoQueueMutex);
        videoQueue.push(vb);
        if (videoQueue.size() >= VIDEO_QUEUE_SIZE)
        {
            bufferReady = false;
        }
        videoReady.notify_one();
    }
}


VideoThread::~VideoThread()
{
    // Step 1 - Kill the noize
    if (videoThread.joinable())
    {
        videoThread.join();
    }

    // Step 2 - Flush the inputs
    if (this->vdc != nullptr)
    {
        this->vdc->flushInput();
        this->vdc->flushOutput();
    }

    // Step 3 - Kill the first tunnel
    if (this->decodeTunnel != nullptr)
    {
        this->decodeTunnel->flush();
        this->decodeTunnel->disable();
        delete this->decodeTunnel;
        this->decodeTunnel = nullptr;
    }

    // Step 4 - Kill the decode component
    if (this->vdc != nullptr)
    {
        this->vdc->disablePortBuffers(130, nullptr, nullptr, nullptr);
        this->vdc->changeState(OMX_StateIdle);
        this->vdc->changeState(OMX_StateLoaded);
        delete this->vdc;
        this->vdc = nullptr;
    }
    if (this->clockSchedTunnel != nullptr)
    {
        this->clockSchedTunnel->flush();
        this->clockSchedTunnel->disable();
        delete this->clockSchedTunnel;
        this->clockSchedTunnel = nullptr;
    }
    if (this->schedTunnel != nullptr)
    {
        this->schedTunnel->flush();
        this->schedTunnel->disable();
        delete this->schedTunnel;
        this->schedTunnel = nullptr;
    }
    if (this->sched != nullptr)
    {
        this->sched->changeState(OMX_StateIdle);
        this->sched->changeState(OMX_StateLoaded);
        delete this->sched;
        this->sched = nullptr;
    }
    if (this->vdr != nullptr)
    {
        this->vdr->changeState(OMX_StateIdle);
        this->vdr->changeState(OMX_StateLoaded);
        delete this->vdr;
        this->vdr = nullptr;
    }
    // Empty queue
    size_t dataSize = 0;
    while(videoQueue.size() > 0)
    {
        VideoBlock * b = videoQueue.front();
        videoQueue.pop();
        if (b != nullptr)
        {
            delete[] b->data;
            delete b;
        }
    }
   
}