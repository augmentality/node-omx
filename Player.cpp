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

#include "Player.h"
#include <string>

using v8::String;

Nan::Persistent<v8::Function> Player::constructor;

NAN_MODULE_INIT(Player::Init) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("Player").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "loadURL", loadURL);
    Nan::SetPrototypeMethod(tpl, "play", play);
    Nan::SetPrototypeMethod(tpl, "stop", stop);
    Nan::SetPrototypeMethod(tpl, "pause", pause);
    Nan::SetPrototypeMethod(tpl, "setSpeed", setSpeed);
    Nan::SetPrototypeMethod(tpl, "setLoop", setLoop);
    Nan::SetPrototypeMethod(tpl, "getTime", getTime);

    constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("Player").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

Player::Player()
{

}

Player::~Player()
{
    if (nativePlayer != nullptr)
    {
        NativePlayer * np = nativePlayer;
        nativePlayer = nullptr;
        delete np;
    }
}

NAN_METHOD(Player::New)
{
    if (info.IsConstructCall())
    {
        Player *obj = new Player();
        obj->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
    }
    else
    {
        const int argc = 1;
        v8::Local <v8::Value> argv[argc] = {};
        v8::Local <v8::Function> cons = Nan::New(constructor);
        info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
    }
}

struct runPlayerData
{
    uv_work_t request;
    std::string url;
    Player * player;
    Nan::Callback loadCallback;
};
struct playbackFinishedData
{
    uv_work_t request;
    Player * player;
};
NAN_INLINE void playbackCompletedEvent (uv_work_t* req)
{
    Nan::HandleScope scope;
    playbackFinishedData * data = static_cast<playbackFinishedData*>(req->data);
    v8::Local<v8::Value> argv[] = {
            Nan::New<v8::Number>(0)
    };
    if (!data->player->playbackStateCallback.IsEmpty())
    {
        data->player->playbackStateCallback.Call(1, argv);
    }
    delete data;
}
NAN_INLINE void fileLoadedEvent (uv_work_t* req)
{
    Nan::HandleScope scope;
    runPlayerData * data = static_cast<runPlayerData*>(req->data);

    v8::Local<v8::Value> argv[] = {
            Nan::New<v8::Number>(1)
    };
    if (!data->player->playbackStateCallback.IsEmpty())
    {
        data->player->playbackStateCallback.Call(1, argv);
    }
    if (!data->loadCallback.IsEmpty())
    {
        data->loadCallback.Call(0, 0);
    }
    delete data;
}
NAN_INLINE void completePlayback (uv_work_t* req)
{
    playbackFinishedData * data = static_cast<playbackFinishedData*>(req->data);
    data->player->completePlayback();
}
NAN_INLINE void loadVideo (uv_work_t* req)
{
    //Nan::HandleScope scope;
    runPlayerData * data = static_cast<runPlayerData*>(req->data);
    Player * objRef = data->player;
    if (objRef->nativePlayer != nullptr)
    {
        NativePlayer * np = objRef->nativePlayer;
        objRef->nativePlayer = nullptr;
        delete np;
    }
    objRef->nativePlayer = new NativePlayer(data->url, [objRef]()
    {
        playbackFinishedData * data = new playbackFinishedData();
        data->player = objRef;
        data->request.data = (void *)data;
        uv_queue_work(uv_default_loop(), &data->request, completePlayback, reinterpret_cast<uv_after_work_cb>(playbackCompletedEvent));
    });
    objRef->playState = 1;
}
NAN_METHOD(Player::loadURL)
{
    Player * obj = Nan::ObjectWrap::Unwrap<Player>(info.Holder());
    if (info.Length() > 2)
    {
        v8::String::Utf8Value param(info[0]->ToString());
        runPlayerData * data = new runPlayerData();
        data->request.data = (void *)data;
        data->player = obj;
        data->url = *param;

        data->loadCallback.Reset(info[1].As<v8::Function>());
        obj->playbackStateCallback.Reset(info[2].As<v8::Function>());

        uv_queue_work(uv_default_loop(), &data->request, loadVideo, reinterpret_cast<uv_after_work_cb>(fileLoadedEvent));
    }
    else if (info.Length() == 2)
    {
        return Nan::ThrowError(Nan::New("PlaybackState callback expected").ToLocalChecked());
    }
    else if (info.Length() == 1)
    {
        return Nan::ThrowError(Nan::New("FileLoaded callback expected").ToLocalChecked());
    }
    else if (info.Length() == 0)
    {
        return Nan::ThrowError(Nan::New("URL expected").ToLocalChecked());
    }
}

NAN_METHOD(Player::play)
{
    Player * obj = Nan::ObjectWrap::Unwrap<Player>(info.This());
    if (obj->playState < 1)
    {
        return Nan::ThrowError(Nan::New("No media loaded").ToLocalChecked());
    }
    if (obj->playState == 2)
    {
        return Nan::ThrowError(Nan::New("Media already playing").ToLocalChecked());
    }
    if (obj->nativePlayer == nullptr)
    {
        return Nan::ThrowError(Nan::New("Player is null").ToLocalChecked());
    }
    obj->nativePlayer->play();
    obj->playState = 2;
    v8::Local<v8::Value> argv[] = {
            Nan::New<v8::Number>(2)
    };
    if (!obj->playbackStateCallback.IsEmpty())
    {
        obj->playbackStateCallback.Call(1, argv);
    }
}

NAN_METHOD(Player::pause)
{
    Player * obj = Nan::ObjectWrap::Unwrap<Player>(info.This());
    if (obj->playState < 2)
    {
        return Nan::ThrowError(Nan::New("Stream is not started").ToLocalChecked());
    }
    if (obj->nativePlayer == nullptr)
    {
        return Nan::ThrowError(Nan::New("Player is null").ToLocalChecked());
    }
    obj->nativePlayer->pause();
    obj->playState = 3;
    v8::Local<v8::Value> argv[] = {
            Nan::New<v8::Number>(3)
    };
    if (!obj->playbackStateCallback.IsEmpty())
    {
        obj->playbackStateCallback.Call(1, argv);
    }
}

NAN_METHOD(Player::setSpeed)
{
    Player * obj = Nan::ObjectWrap::Unwrap<Player>(info.This());
    if (obj->playState < 1)
    {
        return Nan::ThrowError(Nan::New("No media loaded").ToLocalChecked());
    }
    if (info.Length() < 1)
    {
        return Nan::ThrowError(Nan::New("Speed not provided").ToLocalChecked());
    }
    if (obj->nativePlayer == nullptr)
    {
        return Nan::ThrowError(Nan::New("Player is null").ToLocalChecked());
    }
    double speed = info[0]->NumberValue();
    obj->nativePlayer->setSpeed((float)speed);
}

NAN_METHOD(Player::setLoop)
{
    Player * obj = Nan::ObjectWrap::Unwrap<Player>(info.This());
    if (info.Length() < 1)
    {
        return Nan::ThrowError(Nan::New("Loop parameter not provided").ToLocalChecked());
    }
    if (obj->nativePlayer == nullptr)
    {
        return Nan::ThrowError(Nan::New("Player is null").ToLocalChecked());
    }
    bool loop  = info[0]->BooleanValue();
    obj->nativePlayer->setLoop(loop);
}

NAN_METHOD(Player::getTime)
{
    Player * obj = Nan::ObjectWrap::Unwrap<Player>(info.This());
    if (obj->playState < 1)
    {
        return Nan::ThrowError(Nan::New("No media loaded").ToLocalChecked());
    }
    if (obj->nativePlayer == nullptr)
    {
        return Nan::ThrowError(Nan::New("Player is null").ToLocalChecked());
    }
    float time = obj->nativePlayer->getTime();
    info.GetReturnValue().Set(time);
}

NAN_METHOD(Player::stop)
{
    Player * obj = Nan::ObjectWrap::Unwrap<Player>(info.This());
    if (obj->playState < 1)
    {
        return Nan::ThrowError(Nan::New("No media loaded").ToLocalChecked());
    }
    if (obj->nativePlayer == nullptr)
    {
        return Nan::ThrowError(Nan::New("Player is null").ToLocalChecked());
    }
    if (obj->nativePlayer != nullptr)
    {
        NativePlayer * np = obj->nativePlayer;
        obj->nativePlayer = nullptr;
        delete np;
    }
    obj->playState = 0;
    v8::Local<v8::Value> argv[] = {
            Nan::New<v8::Number>(0)
    };
    if (!obj->playbackStateCallback.IsEmpty())
    {
        obj->playbackStateCallback.Call(1, argv);
    }
}
