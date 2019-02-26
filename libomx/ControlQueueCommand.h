#pragma once
#include "ControlQueueCommandType.h"
#include <string>

struct ControlQueueCommand
{
    public:
        ControlQueueCommandType commandType = ControlQueueCommandType::None;
        float floatData = 0.0;
        bool boolData = false;
        std::string stringData = "";
};
