#include "Player.h"
#include <string>

using v8::String;

Nan::Persistent<v8::Function> Player::constructor;

NAN_MODULE_INIT(Player::Init) {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("Player").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "loadURL", loadURL);

    constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("Player").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

Player::Player()
{

}

Player::~Player()
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

NAN_METHOD(Player::loadURL)
{
    Player * obj = Nan::ObjectWrap::Unwrap<Player>(info.This());
    if (info.Length() > 0)
    {
        if (info[0]->IsString())
        {
            try
            {
                v8::String::Utf8Value param1(info[0]->ToString());
                obj->nativePlayer = new NativePlayer(std::string(*param1));
            }
            catch(std::runtime_error ex)
            {
                return Nan::ThrowError(Nan::New(ex.what()).ToLocalChecked());
            }
        }
        else
        {
            return Nan::ThrowError(Nan::New("String URL expected").ToLocalChecked());
        }
    }
    else
    {
        return Nan::ThrowError(Nan::New("String URL expected").ToLocalChecked());
    }
}