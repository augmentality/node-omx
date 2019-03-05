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
#include <string>

#include "ILComponentException.h"
#include "ilc.h"
#include "ILClient.h"
#include "ILTunnel.h"

class ILComponent
{
    public:
        ILComponent(ILClient *client, const std::string _componentName, const ILCLIENT_CREATE_FLAGS_T _flags);
        ~ILComponent();
        ILTunnel * tunnelTo(int sourcePort, ILComponent * targetComponent, int targetPort, unsigned int portStream, int timeout);
        int changeState(OMX_STATETYPE state);
        void flushInput();
        void flushOutput();
        int enablePortBuffers(int portIndex, ILCLIENT_MALLOC_T ilclient_malloc, ILCLIENT_FREE_T ilclient_free, void * priv);
        int disablePortBuffers(int portIndex, OMX_BUFFERHEADERTYPE *bufferList, ILCLIENT_FREE_T ilclient_free, void *priv);
        OMX_ERRORTYPE  setOMXParameter(OMX_INDEXTYPE paramName, void * param);
        OMX_ERRORTYPE  getOMXParameter(OMX_INDEXTYPE paramName, void * param);
        OMX_ERRORTYPE  getOMXConfig(OMX_INDEXTYPE paramName, void * param);
        OMX_ERRORTYPE  setOMXConfig(OMX_INDEXTYPE configName, void * param);
        COMPONENT_T * getComp();
        OMX_BUFFERHEADERTYPE * getInputBuffer(int portIndex, int block);
        int removeEvent(OMX_EVENTTYPE eEvent, OMX_U32 nData1, int ignore1, OMX_IN OMX_U32 nData2, int ignore2);
        int waitForEvent(OMX_EVENTTYPE event, OMX_U32 nData1, int ignore1, OMX_IN OMX_U32 nData2, int ignore2, int event_flag, int suspend);
        OMX_ERRORTYPE emptyBuffer(OMX_BUFFERHEADERTYPE *buf);
        int waitForCommandComplete(OMX_COMMANDTYPE command, OMX_U32 nData2);
        int waitForCommandCompleteDual(OMX_COMMANDTYPE command, OMX_U32 nData2, COMPONENT_T *other);
        void disablePort(int portIndex);

    private:
        ILClient * client = nullptr;

        static OMX_ERRORTYPE eventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                    OMX_IN OMX_PTR pAppData,
                    OMX_IN OMX_EVENTTYPE eEvent,
                    OMX_IN OMX_U32 nData1,
                    OMX_IN OMX_U32 nData2,
                    OMX_IN OMX_PTR pEventData);

        static OMX_ERRORTYPE emptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                             OMX_IN OMX_PTR pAppData,
                                             OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

        static OMX_ERRORTYPE emptyBufferDoneError(OMX_IN OMX_HANDLETYPE hComponent,
                                                              OMX_IN OMX_PTR pAppData,
                                                              OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

        static OMX_ERRORTYPE fillBufferDone(OMX_OUT OMX_HANDLETYPE hComponent,
                                                       OMX_OUT OMX_PTR pAppData,
                                                       OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);

        static OMX_ERRORTYPE fillBufferDoneError(OMX_OUT OMX_HANDLETYPE hComponent,
                                                             OMX_OUT OMX_PTR pAppData,
                                                             OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);





        OMX_ERRORTYPE handleEvent(OMX_IN OMX_HANDLETYPE hComponent,
                                               OMX_IN OMX_EVENTTYPE eEvent,
                                               OMX_IN OMX_U32 nData1,
                                               OMX_IN OMX_U32 nData2,
                                               OMX_IN OMX_PTR pEventData);

        OMX_ERRORTYPE handleEmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);
        OMX_ERRORTYPE handleEmptyBufferDoneError(OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_BUFFERHEADERTYPE * pBuffer);
        OMX_ERRORTYPE handleFillBufferDone(OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);
        OMX_ERRORTYPE handleFillBufferDoneError(OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);



    protected:
        COMPONENT_T *comp = nullptr;

        std::string componentName;
        ILCLIENT_CREATE_FLAGS_T flags;
        int inputPort = -1;
        int outputPort = -1;
};

