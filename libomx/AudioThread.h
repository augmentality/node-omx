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
    AudioThread(ILClient * pClient, ClockComponent * pClock, FFSource * src);
    ~AudioThread();
    void start();
    void addData(AudioBlock * block);

private:
    uint64_t baseTime = 0;
    uint64_t lastTime = 0;

    bool playbackComplete = false;

    void audioThreadFunc();
    void waitForBuffer();
    AudioBlock * dequeue();

    ILClient * client;
    ClockComponent * clock;
    AudioRenderComponent * arc = nullptr;
    ILTunnel * clockAudioTunnel = nullptr;

    std::thread audioThread;
    std::mutex audioQueueMutex;
    std::condition_variable audioReady;
    std::queue<AudioBlock *> audioQueue;
    std::condition_variable readyForData;
    std::mutex readyForDataMutex;
    bool bufferReady = true;
};
