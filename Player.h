#pragma once

#include <nan.h>
#include "libomx/NativePlayer.h"
#include <stdexcept>

class Player : public Nan::ObjectWrap
{
    public:
        static NAN_MODULE_INIT(Init);

    private:

        NativePlayer * nativePlayer = nullptr;
        explicit Player();
        ~Player();
        static NAN_METHOD(New);
        static NAN_METHOD(loadURL);
        static NAN_METHOD(play);
        static NAN_METHOD(pause);
        static NAN_METHOD(setSpeed);
        static Nan::Persistent<v8::Function> constructor;
};

