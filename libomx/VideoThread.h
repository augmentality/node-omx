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


#pragma once

#include "ilclient/VideoDecodeComponent.h"
#include "ilclient/VideoRenderComponent.h"
#include "ilclient/VideoSchedulerComponent.h"
#include "ilclient/ClockComponent.h"
#include "ilclient/ILTunnel.h"
#include "ilclient/ILClient.h"
#include "ffsource/FFSource.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "videoBlock.h"

class VideoThread
{
public:
    VideoThread(ILClient * pClient, ClockComponent * pClock, FFSource * src);
    ~VideoThread();
    void start();
    void addData(VideoBlock * block);
    void waitForCompletion();
    void stop(bool immediately);
private:
    void videoThreadFunc();
    void waitForBuffer();
    VideoBlock * dequeue();

    bool playbackComplete = false;
    bool waitingForEnd = false;
    bool threadRunning = false;
    ILTunnel * decodeTunnel = nullptr;
    ILTunnel * schedTunnel = nullptr;
    ILClient * client = nullptr;
    ClockComponent * clock = nullptr;
    VideoDecodeComponent * vdc = nullptr;
    VideoRenderComponent * vdr = nullptr;
    VideoSchedulerComponent * sched = nullptr;
    ILTunnel * clockSchedTunnel = nullptr;

    std::thread videoThread;

    std::mutex videoPlayingMutex;
    std::mutex videoQueueMutex;
    std::condition_variable videoReady;
    std::queue<VideoBlock *> videoQueue;
    std::mutex syncMutex;
    std::condition_variable playThreadFinished;
    std::condition_variable readyForData;
    std::mutex readyForDataMutex;
    bool bufferReady = true;
};
