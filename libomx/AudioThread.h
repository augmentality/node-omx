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

#include "ilclient/AudioRenderComponent.h"
#include "ilclient/ClockComponent.h"
#include "ilclient/ILTunnel.h"
#include "ilclient/ILClient.h"
#include "ffsource/FFSource.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "audioBlock.h"

class AudioThread
{
public:
    AudioThread(ILClient * pClient, ClockComponent * pClock, FFSource * src, bool skipClock);
    ~AudioThread();
    void start();
    void addData(AudioBlock * block);
    void waitForCompletion();
    void stop(bool immediately);
private:
    bool waitingForEnd = false;
    bool playbackComplete = false;
    bool threadRunning = false;
    void audioThreadFunc();
    void waitForBuffer();
    AudioBlock * dequeue();


    ILClient * client;
    ClockComponent * clock;
    AudioRenderComponent * arc = nullptr;
    ILTunnel * clockAudioTunnel = nullptr;

    std::thread audioThread;
    std::mutex audioPlayingMutex;
    std::mutex audioQueueMutex;
    std::condition_variable audioReady;
    std::mutex syncMutex;
    std::condition_variable playThreadFinished;
    std::queue<AudioBlock *> audioQueue;
    std::condition_variable readyForData;
    std::mutex readyForDataMutex;
    bool bufferReady = true;
};
