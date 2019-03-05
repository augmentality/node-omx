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

#include "ILComponent.h"
#include "ILClient.h"
#include <string>
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/samplefmt.h>
}

class AudioRenderComponent: public ILComponent
{
    public:
        explicit AudioRenderComponent(ILClient * client);
        void setPCMMode(int sampleRate, int channelCount, AVSampleFormat format);
        void setAudioDest(const char * dest);
        void waitForEOS();

};
