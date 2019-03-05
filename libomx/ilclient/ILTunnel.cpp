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

ILTunnel::ILTunnel(ILClient * pClient, ILComponent * source, ILComponent * dest, int srcPort, int dstPort):
    sourceComponent(source),
    sinkComponent(dest),
    sourcePort(srcPort),
    sinkPort(dstPort),
    client(pClient)
{

}

ILTunnel::~ILTunnel()
{
    OMX_ERRORTYPE error;
    error = OMX_SetupTunnel(sourceComponent->getComp()->comp, sourcePort, NULL, 0);
    vc_assert(error == OMX_ErrorNone);
    error = OMX_SetupTunnel(sinkComponent->getComp()->comp, sinkPort, NULL, 0);
    vc_assert(error == OMX_ErrorNone);
}
void ILTunnel::flush()
{
    OMX_ERRORTYPE error;
    error = OMX_SendCommand(sourceComponent->getComp()->comp, OMX_CommandFlush, sourcePort, NULL);
    vc_assert(error == OMX_ErrorNone);
    error = OMX_SendCommand(sinkComponent->getComp()->comp, OMX_CommandFlush, sinkPort, NULL);
    vc_assert(error == OMX_ErrorNone);
    sourceComponent->waitForEvent(OMX_EventCmdComplete, OMX_CommandFlush, 0, sourcePort, 0, ILCLIENT_PORT_FLUSH, VCOS_EVENT_FLAGS_SUSPEND);
    sinkComponent->waitForEvent(OMX_EventCmdComplete, OMX_CommandFlush, 0, sinkPort, 0, ILCLIENT_PORT_FLUSH, VCOS_EVENT_FLAGS_SUSPEND);
}

int ILTunnel::enable()
{
    OMX_STATETYPE state;
    OMX_ERRORTYPE error;

    this->client->debugOutput("ilclient: enable tunnel from %x/%d to %x/%d",
                              sourceComponent->getComp(), sourcePort,
                              sinkComponent->getComp(), sinkPort);

    error = OMX_SendCommand(sourceComponent->getComp()->comp, OMX_CommandPortEnable, sourcePort, NULL);
    vc_assert(error == OMX_ErrorNone);

    error = OMX_SendCommand(sinkComponent->getComp()->comp, OMX_CommandPortEnable, sinkPort, NULL);
    vc_assert(error == OMX_ErrorNone);

    // to complete, the sink component can't be in loaded state
    error = OMX_GetState(sinkComponent->getComp()->comp, &state);
    vc_assert(error == OMX_ErrorNone);
    if (state == OMX_StateLoaded)
    {
        int ret = 0;

        if(sinkComponent->waitForCommandComplete(OMX_CommandPortEnable, sinkPort) != 0 ||
           OMX_SendCommand(sinkComponent->getComp()->comp, OMX_CommandStateSet, OMX_StateIdle, NULL) != OMX_ErrorNone ||
           (ret = sinkComponent->waitForCommandCompleteDual(OMX_CommandStateSet, OMX_StateIdle, sourceComponent->getComp())) < 0)
        {
            if(ret == -2)
            {
                // the error was reported fom the source component: clear this error and disable the sink component
                sourceComponent->waitForCommandComplete(OMX_CommandPortEnable, sourcePort);
                sinkComponent->disablePort(sinkPort);
            }

            sourceComponent->disablePort(sourcePort);
            throw ILComponentException(std::string("ilclient: could not change component state to IDLE"));
        }
    }
    else
    {
        if (sinkComponent->waitForCommandComplete(OMX_CommandPortEnable, sinkPort) != 0)
        {
            //Oops failed to enable the sink port
            sourceComponent->disablePort(sourcePort);
            sourceComponent->waitForEvent(OMX_EventCmdComplete,
                                                  OMX_CommandPortEnable, 0, sourcePort, 0,
                                                  ILCLIENT_PORT_ENABLED | ILCLIENT_EVENT_ERROR, VCOS_EVENT_FLAGS_SUSPEND);
            throw ILComponentException(std::string("Could not change sink port port to enabled"));
        }
    }

    if(sourceComponent->waitForCommandComplete(OMX_CommandPortEnable, sourcePort) != 0)
    {
        sinkComponent->disablePort(sinkPort);
        throw ILComponentException(std::string("Could not change source port to enabled"));
    }

    return 0;
}
int ILTunnel::disable()
{
    OMX_ERRORTYPE error;

    if(sourceComponent->getComp() == nullptr || sinkComponent->getComp() == nullptr)
        return 0;

    sourceComponent->getComp()->error_mask |= ILCLIENT_ERROR_UNPOPULATED;
    sinkComponent->getComp()->error_mask |= ILCLIENT_ERROR_UNPOPULATED;

    error = OMX_SendCommand(sourceComponent->getComp()->comp, OMX_CommandPortDisable, sourcePort, NULL);
    vc_assert(error == OMX_ErrorNone);

    error = OMX_SendCommand(sinkComponent->getComp()->comp, OMX_CommandPortDisable, sinkPort, NULL);
    vc_assert(error == OMX_ErrorNone);

    if(sourceComponent->waitForCommandComplete(OMX_CommandPortDisable, sourcePort) < 0)
        vc_assert(0);

    if(sinkComponent->waitForCommandComplete(OMX_CommandPortDisable, sinkPort) < 0)
        vc_assert(0);

    sourceComponent->getComp()->error_mask &= ~ILCLIENT_ERROR_UNPOPULATED;
    sinkComponent->getComp()->error_mask &= ~ILCLIENT_ERROR_UNPOPULATED;

    return 0;
}
