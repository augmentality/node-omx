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
#include <string>
#include "ffsource/FFSource.h"
#include "ilclient/ClockComponent.h"
#include "ilclient/ILClient.h"
#include "VideoThread.h"
#include "AudioThread.h"
#include "prebufferBlock.h"
#include "ControlQueueCommand.h"
#include <thread>
#include <queue>
#include <functional>

class NativePlayer
{
public:
    NativePlayer(std::string url, std::function<void()> pCompletedFunction);

    ~NativePlayer();

    void play();

    void pause();

    void setSpeed(float scale);

    void setLoop(bool loop);

    double getTime();

    void waitForCompletion();

private:
    std::string playerURL;
    std::function<void()> playbackComplete;
    std::queue<PrebufferBlock *> prebuffer;
    FFFrame * frame = nullptr;
    bool playing = false;
    bool nativePlayerActive = true;
    std::mutex controlMutex;
    std::condition_variable controlCommandReady;
    std::queue<ControlQueueCommand *> controlCommandQueue;
    bool skipAudio = false;
    bool skipVideo = false;
    void playThreadFunc();
    void controlThreadFunc();

    PrebufferBlock * getNextBlock(bool fromPrebuffer);

    int playState = 0;
    float playSpeed = 1.0f;
    std::thread playThread;
    std::thread controlThread;
    FFSource * src = nullptr;
    ILClient * client = nullptr;
    ClockComponent * clock = nullptr;
    VideoThread * videoThread = nullptr;
    AudioThread * audioThread = nullptr;
};