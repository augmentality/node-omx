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
        delete nativePlayer;
    }
}

NAN_METHOD(Player::handleStopped)
{

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
};

NAN_INLINE void playbackCompletedEvent (uv_work_t* req)
{

}
NAN_INLINE void videoPlaybackComplete (uv_work_t* req)
{
    runPlayerData * data = static_cast<runPlayerData*>(req->data);
    Player * objRef = data->player;
    objRef->completePlayback();
}
NAN_METHOD(Player::loadURL)
{
    Player * obj = Nan::ObjectWrap::Unwrap<Player>(info.Holder());
    if (info.Length() > 0)
    {
        v8::String::Utf8Value param1(info[0]->ToString());
        obj->nativePlayer = new NativePlayer(*param1, [obj]()
        {
            runPlayerData * data = new runPlayerData();
            data->request.data = (void *)data;
            data->player = obj;
            uv_queue_work(uv_default_loop(), &data->request, videoPlaybackComplete, reinterpret_cast<uv_after_work_cb>(playbackCompletedEvent));
        });
        obj->playState = 1;
    }
    else
    {
        return Nan::ThrowError(Nan::New("String URL expected").ToLocalChecked());
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
    delete obj->nativePlayer;
    obj->playState = 0;
}

