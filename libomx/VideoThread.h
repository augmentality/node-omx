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

private:
    uint64_t baseTime = 0;
    uint64_t lastTime = 0;

    void videoThreadFunc();
    void waitForBuffer();
    VideoBlock * dequeue();

    bool playbackComplete = false;

    ILTunnel * decodeTunnel = nullptr;
    ILTunnel * schedTunnel = nullptr;
    ILClient * client = nullptr;
    ClockComponent * clock = nullptr;
    VideoDecodeComponent * vdc = nullptr;
    VideoRenderComponent * vdr = nullptr;
    VideoSchedulerComponent * sched = nullptr;
    ILTunnel * clockSchedTunnel = nullptr;

    std::thread videoThread;
    std::mutex videoQueueMutex;
    std::condition_variable videoReady;
    std::queue<VideoBlock *> videoQueue;
    std::condition_variable readyForData;
    std::mutex readyForDataMutex;
    bool bufferReady = true;
};
