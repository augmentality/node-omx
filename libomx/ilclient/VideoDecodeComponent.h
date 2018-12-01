#pragma once
#include "ilc.h"
#include "ILComponent.h"
#include "ILClient.h"
#include <string>

class VideoDecodeComponent: public ILComponent
{
    public:
        explicit VideoDecodeComponent(ILClient * client);
        void setVideoCompressionFormat(OMX_VIDEO_CODINGTYPE codec = (OMX_VIDEO_CODINGTYPE)0, OMX_COLOR_FORMATTYPE colorFormat = (OMX_COLOR_FORMATTYPE)0, float fps = 0.0, int width = 1920, int height = 1080);
};
