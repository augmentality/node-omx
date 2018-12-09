//
// node-omx - A media player for node.js on the Raspberry Pi
// Copyright (C) 2018 Augmentality Ltd <info@augmentality.uk>
//
// This file is part of node-omx.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; version 2.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
// Street, Fifth Floor, Boston, MA 02110-1301, USA.


#pragma once

#define DVD_TIME_BASE 1000000
#define DVD_NOPTS_VALUE    (-1LL<<52)

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

struct FFFrame
{
    bool video = false;
    bool loopedVideo = false;
    bool loopedAudio = false;
    AVPacket * pkt = nullptr;
    AVFrame * frame = nullptr;
    int audioDataSize = 0;
    uint8_t *  convertedAudio = nullptr;
    int convertedSize = 0;
    double pts = DVD_NOPTS_VALUE;
    double dts = DVD_NOPTS_VALUE;
    double duration = 0;
};
