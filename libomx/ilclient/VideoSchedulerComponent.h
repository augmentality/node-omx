#pragma once

#include "ILComponent.h"
#include "ILClient.h"
#include <string>

class VideoSchedulerComponent: public ILComponent
{
    public:
        explicit VideoSchedulerComponent(ILClient * client);
};
