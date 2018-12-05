#include "Player-dummy.h"

using v8::FunctionTemplate;

NAN_MODULE_INIT(InitAll) {
        Player::Init(target);
}

NODE_MODULE(NativeExtension, InitAll)