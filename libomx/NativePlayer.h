#pragma once
#include <string>
#include "ffsource/FFSource.h"
#include "ilclient/ClockComponent.h"
#include "ilclient/ILClient.h"
#include "VideoThread.h"
#include "AudioThread.h"
#include "prebufferBlock.h"
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
    std::function<void()> playbackComplete;
    std::queue<PrebufferBlock *> prebuffer;
    FFFrame * frame = nullptr;
    bool playing = false;

    void playThreadFunc();

    PrebufferBlock * getNextBlock(bool fromPrebuffer);

    int playState = 0;
    float playSpeed = 1.0f;
    std::thread playThread;
    FFSource * src = nullptr;
    ILClient * client = nullptr;
    ClockComponent * clock = nullptr;
    VideoThread * videoThread = nullptr;
    AudioThread * audioThread = nullptr;
};