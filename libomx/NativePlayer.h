#pragma once
#include <string>
#include "ffsource/FFSource.h"
#include "ilclient/ClockComponent.h"
#include "ilclient/ILClient.h"
#include "VideoThread.h"
#include "AudioThread.h"
#include <thread>

class NativePlayer
{
    public:
        NativePlayer(std::string url);
        ~NativePlayer();
        void play();


    private:
        bool playing = false;
        void playThreadFunc();
        int playState =0 ;
        std::thread playThread;
        FFSource * src = nullptr;
        ILClient * client = nullptr;
        ClockComponent * clock = nullptr;
        VideoThread * videoThread = nullptr;
        AudioThread * audioThread = nullptr;
};