#pragma once

#include <nan.h>
#include <stdexcept>
#include <thread>

class Player : public Nan::ObjectWrap
{
    public:
        static NAN_MODULE_INIT(Init);
        void completePlayback()
        {
            playState = 0;
        }
        int playState = 0;

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

