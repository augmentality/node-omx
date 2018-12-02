#pragma once

#include "ILComponent.h"
#include "ILClient.h"

class ClockComponent: public ILComponent
{
    public:
        explicit ClockComponent(ILClient * client);
        void setTimeScale(float scale);
        double getTime();
};

