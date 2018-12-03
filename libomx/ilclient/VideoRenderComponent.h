#pragma once

#include "ILComponent.h"
#include "ILClient.h"
#include <string>

class VideoRenderComponent: public ILComponent
{
    public:
        explicit VideoRenderComponent(ILClient * client);
        void flush();
};
