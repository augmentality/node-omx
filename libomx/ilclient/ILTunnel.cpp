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


#include "ILTunnel.h"
#include "ILComponent.h"

ILTunnel::~ILTunnel()
{
    OMX_SendCommand(sourceComponent->getComp()->comp, OMX_CommandPortDisable, sourcePort, NULL);
    OMX_SendCommand(sinkComponent->getComp()->comp, OMX_CommandPortDisable, sinkPort, NULL);
    sourceComponent->waitForCommandComplete(OMX_CommandPortDisable, sourcePort);
    sinkComponent->waitForCommandComplete(OMX_CommandPortDisable, sinkPort);
}
