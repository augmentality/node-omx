/*

node-omx - A media player for node.js on the Raspberry Pi
Copyright (C) 2018 Augmentality Ltd <info@augmentality.uk>

This file is part of node-omx.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/


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
