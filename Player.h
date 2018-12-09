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

#include <nan.h>
#include "libomx/NativePlayer.h"
#include <stdexcept>
#include <thread>

class Player : public Nan::ObjectWrap
{
    public:
        static NAN_MODULE_INIT(Init);
        void completePlayback()
        {
            nativePlayer->waitForCompletion();
            playState = 0;
            delete nativePlayer;
            nativePlayer = nullptr;
        }
        int playState = 0;
        Nan::Callback playbackStateCallback;
        NativePlayer * nativePlayer = nullptr;

    private:
        std::thread runThread;
        explicit Player();
        ~Player();
        static NAN_METHOD(New);
        static NAN_METHOD(loadURL);
        static NAN_METHOD(play);
        static NAN_METHOD(pause);
        static NAN_METHOD(setSpeed);
        static NAN_METHOD(setLoop);
        static NAN_METHOD(getTime);
        static NAN_METHOD(stop);
        static NAN_METHOD(handleStopped);
        static Nan::Persistent<v8::Function> constructor;
};

