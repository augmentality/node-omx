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

        void disablePort(int portIndex);
        int waitForCommandCompleteDual(OMX_COMMANDTYPE command, OMX_U32 nData2, COMPONENT_T *other);
        int enableTunnel(ILTunnel * tunnel);
        int disableTunnel(ILTunnel * tunnel);

    protected:
        COMPONENT_T *comp = nullptr;

        std::string componentName;
        ILCLIENT_CREATE_FLAGS_T flags;
        int inputPort = -1;
        int outputPort = -1;
};

