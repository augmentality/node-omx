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


#include "ClockComponent.h"
#include "../ffsource/FFFrame.h"

ClockComponent::ClockComponent(ILClient *client): ILComponent(client, std::string("clock"), (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS))
{
    OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
    memset(&cstate, 0, sizeof(cstate));
    cstate.nSize = sizeof(cstate);
    cstate.nVersion.nVersion = OMX_VERSION;
    cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
    cstate.nWaitMask = 1;
    this->setOMXParameter(OMX_IndexConfigTimeClockState, &cstate);
}

void ClockComponent::setTimeScale(float scale)
{
    OMX_TIME_CONFIG_SCALETYPE tScale;
    memset(&tScale, 0, sizeof(tScale));
    tScale.nSize = sizeof(tScale);
    tScale.nVersion.nVersion = OMX_VERSION;
    tScale.xScale = (long long)(1 << 16) * scale;
    this->setOMXParameter(OMX_IndexConfigTimeScale, &tScale);
}

double ClockComponent::getTime()
{
    OMX_TIME_CONFIG_TIMESTAMPTYPE timeStamp;
    memset(&timeStamp, 0, sizeof(timeStamp));
    timeStamp.nSize = sizeof(timeStamp);
    timeStamp.nPortIndex = inputPort;
    timeStamp.nVersion.nVersion = OMX_VERSION;
    int err = this->getOMXConfig(OMX_IndexConfigTimeCurrentMediaTime, &timeStamp);
    if (err < 0)
    {
        return 0.0;
    }
    return ((double)FromOMXTime(timeStamp.nTimestamp)) / DVD_TIME_BASE;
}