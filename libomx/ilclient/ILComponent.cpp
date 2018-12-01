#include "ILComponent.h"
#include <cstring>
#include <iostream>

OMX_ERRORTYPE ILComponent::eventHandler(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData)
{
    return ((ILComponent*)pAppData)->handleEvent(hComponent, eEvent, nData1, nData2, pEventData);
}

OMX_ERRORTYPE ILComponent::emptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_IN OMX_PTR pAppData,
                                     OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    return ((ILComponent*)pAppData)->handleEmptyBufferDone(hComponent, pBuffer);
}
OMX_ERRORTYPE ILComponent::emptyBufferDoneError(OMX_IN OMX_HANDLETYPE hComponent,
                                          OMX_IN OMX_PTR pAppData,
                                          OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    return ((ILComponent*)pAppData)->handleEmptyBufferDoneError(hComponent, pBuffer);
}

OMX_ERRORTYPE ILComponent::fillBufferDone(OMX_OUT OMX_HANDLETYPE hComponent,
                                    OMX_OUT OMX_PTR pAppData,
                                    OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
    return ((ILComponent*)pAppData)->handleFillBufferDone(hComponent, pBuffer);
}

OMX_ERRORTYPE ILComponent::fillBufferDoneError(OMX_OUT OMX_HANDLETYPE hComponent,
                                         OMX_OUT OMX_PTR pAppData,
                                         OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
    return ((ILComponent*)pAppData)->handleFillBufferDoneError(hComponent, pBuffer);
}

OMX_ERRORTYPE ILComponent::handleEmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_BUFFERHEADERTYPE *list;

    this->client->debugOutput("%s: empty buffer done %p", this->comp->name, pBuffer);

    vcos_semaphore_wait(&this->comp->sema);
    // insert at end of the list, so we process buffers in
    // the same order
    list = this->comp->in_list;
    while(list && list->pAppPrivate)
    {
        list = (OMX_BUFFERHEADERTYPE *)list->pAppPrivate;
    }

    if(!list)
    {
        this->comp->in_list = pBuffer;
    }
    else
    {
        list->pAppPrivate = pBuffer;
    }

    pBuffer->pAppPrivate = NULL;
    vcos_semaphore_post(&this->comp->sema);

    vcos_event_flags_set(&this->comp->event, ILCLIENT_EMPTY_BUFFER_DONE, VCOS_OR);

    if (this->comp->client->empty_buffer_done_callback)
    this->comp->client->empty_buffer_done_callback(this->comp->client->empty_buffer_done_callback_data, this->comp);

    return OMX_ErrorNone;
}
OMX_ERRORTYPE ILComponent::handleEmptyBufferDoneError(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    vc_assert(0);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ILComponent::handleFillBufferDoneError(OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
    vc_assert(0);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ILComponent::handleFillBufferDone(OMX_OUT OMX_HANDLETYPE hComponent, OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_BUFFERHEADERTYPE * list;

    this->client->debugOutput("%s: fill buffer done %p", this->comp->name, pBuffer);

    vcos_semaphore_wait(&this->comp->sema);
    // insert at end of the list, so we process buffers in
    // the correct order
    list = this->comp->out_list;
    while(list && list->pAppPrivate)
    {
        list = (OMX_BUFFERHEADERTYPE *)list->pAppPrivate;
    }

    if(!list)
    {
        this->comp->out_list = pBuffer;
    }
    else
    {
        list->pAppPrivate = pBuffer;
    }

    pBuffer->pAppPrivate = NULL;
    vcos_semaphore_post(&this->comp->sema);

    vcos_event_flags_set(&this->comp->event, ILCLIENT_FILL_BUFFER_DONE, VCOS_OR);

    if (this->comp->client->fill_buffer_done_callback)
    this->comp->client->fill_buffer_done_callback(this->comp->client->fill_buffer_done_callback_data, this->comp);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE ILComponent::handleEvent(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData)
{
    ILEVENT_T *event;
    OMX_ERRORTYPE error = OMX_ErrorNone;

    this->client->lockEvents();

    // go through the events on this component and remove any duplicates in the
    // existing list, since the client probably doesn't need them.  it's better
    // than asserting when we run out.
    event = this->comp->list;
    while (event != nullptr)
    {
        ILEVENT_T **list = &(event->next);
        while (*list != nullptr)
        {
            if ((*list)->eEvent == event->eEvent &&
                (*list)->nData1 == event->nData1 &&
                (*list)->nData2 == event->nData2)
            {
                // remove this duplicate
                ILEVENT_T *rem = *list;
                this->client->debugOutput("%s: removing %d/%d/%d", this->comp->name, event->eEvent, event->nData1, event->nData2);
                *list = rem->next;
                rem->eEvent = (OMX_EVENTTYPE)-1;
                rem->next = this->comp->client->event_list;
                this->comp->client->event_list = rem;
            }
            else
            {
                list = &((*list)->next);
            }
        }

        event = event->next;
    }

    vc_assert(this->comp->client->event_list);
    event = this->comp->client->event_list;

    switch (eEvent)
    {
        case OMX_EventCmdComplete:
            switch (nData1)
            {
                case OMX_CommandStateSet:
                    this->client->debugOutput("%s: callback state changed (%s)", this->comp->name, states[nData2]);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_STATE_CHANGED, VCOS_OR);
                    break;
                case OMX_CommandPortDisable:
                    this->client->debugOutput("%s: callback port disable %d", this->comp->name, nData2);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_PORT_DISABLED, VCOS_OR);
                    break;
                case OMX_CommandPortEnable:
                    this->client->debugOutput("%s: callback port enable %d", this->comp->name, nData2);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_PORT_ENABLED, VCOS_OR);
                    break;
                case OMX_CommandFlush:
                    this->client->debugOutput("%s: callback port flush %d", this->comp->name, nData2);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_PORT_FLUSH, VCOS_OR);
                    break;
                case OMX_CommandMarkBuffer:
                    this->client->debugOutput("%s: callback mark buffer %d", this->comp->name, nData2);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_MARKED_BUFFER, VCOS_OR);
                    break;
                default:
                    vc_assert(0);
            }
            break;
        case OMX_EventError:
        {
            // check if this component failed a command, and we have to notify another command
            // of this failure
            if (nData2 == 1 && this->comp->related != NULL)
                vcos_event_flags_set(&this->comp->related->event, ILCLIENT_EVENT_ERROR, VCOS_OR);

            error = (OMX_ERRORTYPE)nData1;
            switch (error)
            {
                case OMX_ErrorPortUnpopulated:
                    if (this->comp->error_mask & ILCLIENT_ERROR_UNPOPULATED)
                    {
                        this->client->debugOutput("%s: ignore error: port unpopulated (%d)", this->comp->name, nData2);
                        event = NULL;
                        break;
                    }
                    this->client->debugOutput("%s: port unpopulated %x (%d)", this->comp->name, error, nData2);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                    break;
                case OMX_ErrorSameState:
                    if (this->comp->error_mask & ILCLIENT_ERROR_SAMESTATE)
                    {
                        this->client->debugOutput("%s: ignore error: same state (%d)", this->comp->name, nData2);
                        event = NULL;
                        break;
                    }
                    this->client->debugOutput("%s: same state %x (%d)", this->comp->name, error, nData2);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                    break;
                case OMX_ErrorBadParameter:
                    if (this->comp->error_mask & ILCLIENT_ERROR_BADPARAMETER)
                    {
                        this->client->debugOutput("%s: ignore error: bad parameter (%d)", this->comp->name, nData2);
                        event = NULL;
                        break;
                    }
                    this->client->debugOutput("%s: bad parameter %x (%d)", this->comp->name, error, nData2);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                    break;
                case OMX_ErrorIncorrectStateTransition:
                    this->client->debugOutput("%s: incorrect state transition %x (%d)", this->comp->name, error, nData2);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                    break;
                case OMX_ErrorBadPortIndex:
                    this->client->debugOutput("%s: bad port index %x (%d)", this->comp->name, error, nData2);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                    break;
                case OMX_ErrorStreamCorrupt:
                    this->client->debugOutput("%s: stream corrupt %x (%d)", this->comp->name, error, nData2);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                    break;
                case OMX_ErrorInsufficientResources:
                    this->client->debugOutput("%s: insufficient resources %x (%d)", this->comp->name, error, nData2);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                    break;
                case OMX_ErrorUnsupportedSetting:
                    this->client->debugOutput("%s: unsupported setting %x (%d)", this->comp->name, error, nData2);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                    break;
                case OMX_ErrorOverflow:
                    this->client->debugOutput("%s: overflow %x (%d)", this->comp->name, error, nData2);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                    break;
                case OMX_ErrorDiskFull:
                    this->client->debugOutput("%s: disk full %x (%d)", this->comp->name, error, nData2);
                    //we do not set the error
                    break;
                case OMX_ErrorMaxFileSize:
                    this->client->debugOutput("%s: max file size %x (%d)", this->comp->name, error, nData2);
                    //we do not set the error
                    break;
                case OMX_ErrorDrmUnauthorised:
                    this->client->debugOutput("%s: drm file is unauthorised %x (%d)", this->comp->name, error, nData2);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                    break;
                case OMX_ErrorDrmExpired:
                    this->client->debugOutput("%s: drm file has expired %x (%d)", this->comp->name, error, nData2);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                    break;
                case OMX_ErrorDrmGeneral:
                    this->client->debugOutput("%s: drm library error %x (%d)", this->comp->name, error, nData2);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                    break;
                default:
                    vc_assert(0);
                    this->client->debugOutput("%s: unexpected error %x (%d)", this->comp->name, error, nData2);
                    vcos_event_flags_set(&this->comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR);
                    break;
            }
        }
            break;
        case OMX_EventBufferFlag:
            this->client->debugOutput("%s: buffer flag %d/%x", this->comp->name, nData1, nData2);
            if (nData2 & OMX_BUFFERFLAG_EOS)
            {
                vcos_event_flags_set(&this->comp->event, ILCLIENT_BUFFER_FLAG_EOS, VCOS_OR);
                nData2 = OMX_BUFFERFLAG_EOS;
            }
            else
                vc_assert(0);
            break;
        case OMX_EventPortSettingsChanged:
            this->client->debugOutput("%s: port settings changed %d", this->comp->name, nData1);
            vcos_event_flags_set(&this->comp->event, ILCLIENT_PARAMETER_CHANGED, VCOS_OR);
            break;
        case OMX_EventMark:
            this->client->debugOutput("%s: buffer mark %p", this->comp->name, pEventData);
            vcos_event_flags_set(&this->comp->event, ILCLIENT_BUFFER_MARK, VCOS_OR);
            break;
        case OMX_EventParamOrConfigChanged:
            this->client->debugOutput("%s: param/config 0x%X on port %d changed", this->comp->name, nData2, nData1);
            vcos_event_flags_set(&this->comp->event, ILCLIENT_CONFIG_CHANGED, VCOS_OR);
            break;
        default:
            vc_assert(0);
            break;
    }

    if (event)
    {
        // fill in details
        event->eEvent = eEvent;
        event->nData1 = nData1;
        event->nData2 = nData2;
        event->pEventData = pEventData;

        // remove from top of spare list
        this->comp->client->event_list = this->comp->client->event_list->next;

        // put at head of component event queue
        event->next = this->comp->list;
        this->comp->list = event;
    }
    this->client->unlockEvents();

    // now call any callbacks without the event lock so the client can
    // remove the event in context
    switch (eEvent)
    {
        case OMX_EventError:
            if (event && this->comp->client->error_callback)
            {
                this->comp->client->error_callback(this->comp->client->error_callback_data, this->comp, error);
            }
            break;
        case OMX_EventBufferFlag:
            if ((nData2 & OMX_BUFFERFLAG_EOS) && this->comp->client->eos_callback)
            {
                this->comp->client->eos_callback(this->comp->client->eos_callback_data, this->comp, nData1);
            }
            break;
        case OMX_EventPortSettingsChanged:
            if (this->comp->client->port_settings_callback)
            {
                this->comp->client->port_settings_callback(this->comp->client->port_settings_callback_data, this->comp,
                                                           nData1);
            }
            break;
        case OMX_EventParamOrConfigChanged:
            if (this->comp->client->configchanged_callback)
            {
                this->comp->client->configchanged_callback(this->comp->client->configchanged_callback_data, this->comp,
                                                           nData2);
            }
            break;
        default:
            // ignore
            break;
    }

    return OMX_ErrorNone;
}

ILComponent::ILComponent(ILClient * client, const std::string _componentName, const ILCLIENT_CREATE_FLAGS_T _flags):
    client(client),
    componentName(_componentName),
    flags(_flags)
{
    OMX_CALLBACKTYPE callbacks;
    OMX_ERRORTYPE error;
    char component_name[128];
    int32_t status;

    this->comp = (COMPONENT_T *)vcos_malloc(sizeof(COMPONENT_T), "il:comp");
    if(!this->comp)
    {
        throw(ILComponentException(std::string("Failed to create component")));
    }

    memset(this->comp, 0, sizeof(COMPONENT_T));

    #define COMP_PREFIX "OMX.broadcom."

    status = vcos_event_flags_create(&(this->comp)->event,"il:comp");
    vc_assert(status == VCOS_SUCCESS);
    status = vcos_semaphore_create(&(this->comp)->sema, "il:comp", 1);
    vc_assert(status == VCOS_SUCCESS);
    this->comp->client = client->getClient();

    vcos_snprintf(this->comp->name, sizeof(this->comp->name), "cl:%s", this->componentName.c_str());
    vcos_snprintf(this->comp->bufname, sizeof(this->comp->bufname), "cl:%s buffer", this->componentName.c_str());
    vcos_snprintf(component_name, sizeof(component_name), "%s%s", COMP_PREFIX, this->componentName.c_str());

    this->comp->flags = this->flags;

    callbacks.EventHandler = &ILComponent::eventHandler;
    callbacks.EmptyBufferDone = this->comp->flags & ILCLIENT_ENABLE_INPUT_BUFFERS ? ILComponent::emptyBufferDone : ILComponent::emptyBufferDoneError;
    callbacks.FillBufferDone = this->comp->flags & ILCLIENT_ENABLE_OUTPUT_BUFFERS ? ILComponent::fillBufferDone : ILComponent::fillBufferDoneError;

    error = OMX_GetHandle(&(this->comp)->comp, component_name, this, &callbacks);

    if (error == OMX_ErrorNone)
    {
        OMX_UUIDTYPE uid;
        char name[128];
        OMX_VERSIONTYPE compVersion, specVersion;

        if(OMX_GetComponentVersion(this->comp->comp, name, &compVersion, &specVersion, &uid) == OMX_ErrorNone)
        {
            char *p = (char *) uid + strlen(COMP_PREFIX);

            vcos_snprintf(this->comp->name, sizeof(comp->name), "cl:%s", p);
            this->comp->name[sizeof(comp->name)-1] = 0;
            vcos_snprintf(comp->bufname, sizeof(comp->bufname), "cl:%s buffer", p);
            this->comp->bufname[sizeof(comp->bufname)-1] = 0;
        }

        if(this->comp->flags & (ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_OUTPUT_ZERO_BUFFERS))
        {
            OMX_PORT_PARAM_TYPE ports;
            OMX_INDEXTYPE types[] = {OMX_IndexParamAudioInit, OMX_IndexParamVideoInit, OMX_IndexParamImageInit, OMX_IndexParamOtherInit};
            int i;

            ports.nSize = sizeof(OMX_PORT_PARAM_TYPE);
            ports.nVersion.nVersion = OMX_VERSION;
            for(i=0; i<4; i++)
            {
                OMX_ERRORTYPE error = OMX_GetParameter(this->comp->comp, types[i], &ports);
                if(error == OMX_ErrorNone)
                {
                    uint32_t j;

                    this->inputPort = ports.nStartPortNumber;
                    this->outputPort = this->inputPort + 1;

                    for(j = 0; j < ports.nPorts; j++)
                    {
                        if(this->comp->flags & ILCLIENT_DISABLE_ALL_PORTS)
                        {
                            this->disablePort(ports.nStartPortNumber + j);
                        }

                        if(this->comp->flags & ILCLIENT_OUTPUT_ZERO_BUFFERS)
                        {
                            OMX_PARAM_PORTDEFINITIONTYPE portdef;
                            portdef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
                            portdef.nVersion.nVersion = OMX_VERSION;
                            portdef.nPortIndex = ports.nStartPortNumber + j;
                            if(OMX_GetParameter(this->comp->comp, OMX_IndexParamPortDefinition, &portdef) == OMX_ErrorNone)
                            {
                                if(portdef.eDir == OMX_DirOutput && portdef.nBufferCountActual > 0)
                                {
                                    this->setOMXParameter(OMX_IndexParamPortDefinition, &portdef);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        vcos_event_flags_delete(&this->comp->event);
        vcos_semaphore_delete(&this->comp->sema);
        vcos_free(this->comp);
        this->comp = NULL;
        char buffer [100];
        snprintf(buffer, 100, "Failed to allocate buffers %x", error);
        throw(ILComponentException(std::string(buffer)));
    }
}

ILComponent::~ILComponent()
{
    if (this->comp != nullptr)
    {
        OMX_FreeHandle(this->comp->comp);
        vcos_event_flags_delete(&this->comp->event);
        vcos_semaphore_delete(&this->comp->sema);
        vcos_free(this->comp);
        this->comp = nullptr;
    }
}

void ILComponent::disablePort(int portIndex)
{
    OMX_ERRORTYPE error;
    error = OMX_SendCommand(this->comp->comp, OMX_CommandPortDisable, portIndex, NULL);
    vc_assert(error == OMX_ErrorNone);
    if(this->waitForCommandComplete(OMX_CommandPortDisable, portIndex) < 0)
    {
        vc_assert(0);
    }
}

int ILComponent::waitForCommandComplete(OMX_COMMANDTYPE command, OMX_U32 nData2)
{
    return this->waitForCommandCompleteDual(command, nData2, nullptr);
}

int ILComponent::waitForCommandCompleteDual(OMX_COMMANDTYPE command, OMX_U32 nData2, COMPONENT_T *other)
{
    OMX_U32 mask = ILCLIENT_EVENT_ERROR;
    int ret = 0;

    switch(command) {
        case OMX_CommandStateSet:
            mask |= ILCLIENT_STATE_CHANGED;
            break;
        case OMX_CommandPortDisable:
            mask |= ILCLIENT_PORT_DISABLED;
            break;
        case OMX_CommandPortEnable:
            mask |= ILCLIENT_PORT_ENABLED;
            break;
        default:
            return -1;
    }

    if(other)
    {
        other->related = this->comp;
    }

    while(true)
    {
        ILEVENT_T *cur, *prev = nullptr;
        VCOS_UNSIGNED set;

        this->client->lockEvents();

        cur = this->comp->list;
        while(cur &&
              !(cur->eEvent == OMX_EventCmdComplete && cur->nData1 == command && cur->nData2 == nData2) &&
              !(cur->eEvent == OMX_EventError && cur->nData2 == 1))
        {
            prev = cur;
            cur = cur->next;
        }

        if(cur)
        {
            if(prev == nullptr)
            {
                this->comp->list = cur->next;
            }
            else
            {
                prev->next = cur->next;
            }

            // work out whether this was a success or a fail event
            ret = cur->eEvent == OMX_EventCmdComplete || cur->nData1 == OMX_ErrorSameState ? 0 : -1;

            if(cur->eEvent == OMX_EventError)
                vcos_event_flags_get(&comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR_CONSUME, 0, &set);

            // add back into spare list
            cur->next = this->comp->client->event_list;
            this->comp->client->event_list = cur;
            cur->eEvent = (OMX_EVENTTYPE)-1; // mark as unused

            this->client->unlockEvents();
            break;
        }
        else if(other != nullptr)
        {
            // check the other component for an error event that terminates a command
            cur = other->list;
            while(cur && !(cur->eEvent == OMX_EventError && cur->nData2 == 1))
            {
                cur = cur->next;
            }

            if(cur)
            {
                // we don't remove the event in this case, since the user
                // can confirm that this event errored by calling wait_for_command on the
                // other component

                ret = -2;
                this->client->unlockEvents();
                break;
            }
        }

        this->client->unlockEvents();

        vcos_event_flags_get(&this->comp->event, mask, VCOS_OR_CONSUME, VCOS_SUSPEND, &set);
    }

    if(other)
        other->related = nullptr;

    return ret;
}

ILTunnel * ILComponent::tunnelTo(int sourcePort, ILComponent * sink, int sinkPort, unsigned int portStream, int timeout)
{
    ILTunnel * tunnel = new ILTunnel();
    tunnel->sourceComponent = this;
    tunnel->sourcePort = sourcePort;
    tunnel->sinkComponent = sink;
    tunnel->sinkPort = sinkPort;

    OMX_ERRORTYPE error;
    OMX_PARAM_U32TYPE param;
    OMX_STATETYPE state;
    int32_t status;
    int enable_error;

    // source component must at least be idle, not loaded
    error = OMX_GetState(this->comp->comp, &state);
    vc_assert(error == OMX_ErrorNone);
    if (state == OMX_StateLoaded && this->changeState(OMX_StateIdle) < 0)
    {
        throw ILComponentException(std::string("Unable to change component state"));
    }

    // wait for the port parameter changed from the source port
    if(timeout)
    {
        status = this->waitForEvent(OMX_EventPortSettingsChanged,
                                         sourcePort, 0, -1, 1,
                                         ILCLIENT_PARAMETER_CHANGED | ILCLIENT_EVENT_ERROR, timeout);

        if (status < 0)
        {
            throw ILComponentException(std::string("ilclient: timed out waiting for port settings changed on port %d"));
        }
    }

    // disable ports
    this->disableTunnel(tunnel);

    // if this source port uses port streams, we need to select one of them before proceeding
    // if getparameter causes an error that's fine, nothing needs selecting
    param.nSize = sizeof(OMX_PARAM_U32TYPE);
    param.nVersion.nVersion = OMX_VERSION;
    param.nPortIndex = sourcePort;
    if (OMX_GetParameter(this->comp->comp, OMX_IndexParamNumAvailableStreams, &param) == OMX_ErrorNone)
    {
        if (param.nU32 == 0)
        {
            // no streams available
            // leave the source port disabled, and return a failure
            throw ILComponentException(std::string("No streams available"));
        }
        if (param.nU32 <= portStream)
        {
            // requested stream not available
            // no streams available
            // leave the source port disabled, and return a failure
            throw ILComponentException(std::string("Requested stream not available"));
        }

        param.nU32 = portStream;
        error = this->setOMXParameter(OMX_IndexParamActiveStream, &param);
        vc_assert(error == OMX_ErrorNone);
    }

    // now create the tunnel
    error = OMX_SetupTunnel(this->comp->comp, sourcePort, sink->getComp()->comp, sinkPort);

    enable_error = 0;

    if (error != OMX_ErrorNone || (enable_error = this->enableTunnel(tunnel)) < 0)
    {
        // probably format not compatible
        error = OMX_SetupTunnel(this->comp->comp, sourcePort, NULL, 0);
        vc_assert(error == OMX_ErrorNone);
        error = OMX_SetupTunnel(sink->getComp()->comp, sinkPort, NULL, 0);
        vc_assert(error == OMX_ErrorNone);

        if(enable_error)
        {
            //Clean up the errors. This does risk removing an error that was nothing to do with this tunnel :-/
            sink->removeEvent(OMX_EventError, 0, 1, 0, 1);
            this->removeEvent(OMX_EventError, 0, 1, 0, 1);
        }

        throw ILComponentException(std::string("Could not setup/enable tunnel"));
    }

    return tunnel;
}

int ILComponent::changeState(OMX_STATETYPE state)
{
    OMX_ERRORTYPE error;
    error = OMX_SendCommand(this->comp->comp, OMX_CommandStateSet, state, NULL);
    vc_assert(error == OMX_ErrorNone);
    if(this->waitForCommandComplete(OMX_CommandStateSet, state) < 0)
    {
        this->client->debugOutput("ilclient: could not change component state to %d", state);
        this->removeEvent(OMX_EventError, 0, 1, 0, 1);
        return -1;
    }
    return 0;
}
int ILComponent::removeEvent(OMX_EVENTTYPE eEvent, OMX_U32 nData1, int ignore1, OMX_IN OMX_U32 nData2, int ignore2)
{
    ILEVENT_T *cur, *prev;
    uint32_t set;
    this->client->lockEvents();

    cur = this->comp->list;
    prev = nullptr;

    while (cur && !(cur->eEvent == eEvent && (ignore1 || cur->nData1 == nData1) && (ignore2 || cur->nData2 == nData2)))
    {
        prev = cur;
        cur = cur->next;
    }

    if (cur == nullptr)
    {
        this->client->unlockEvents();
        return -1;
    }

    if (prev == nullptr)
    {
        this->comp->list = cur->next;
    }
    else
    {
        prev->next = cur->next;
    }

    // add back into spare list
    cur->next = this->comp->client->event_list;
    this->comp->client->event_list = cur;
    cur->eEvent = (OMX_EVENTTYPE)-1; // mark as unused

    // if we're removing an OMX_EventError or OMX_EventParamOrConfigChanged event, then clear the error bit from the eventgroup,
    // since the user might have been notified through the error callback, and then
    // can't clear the event bit - this will then cause problems the next time they
    // wait for an error.
    if(eEvent == OMX_EventError)
        vcos_event_flags_get(&this->comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR_CONSUME, 0, &set);
    else if(eEvent == OMX_EventParamOrConfigChanged)
        vcos_event_flags_get(&this->comp->event, ILCLIENT_CONFIG_CHANGED, VCOS_OR_CONSUME, 0, &set);

    this->client->unlockEvents();
    return 0;
}

COMPONENT_T * ILComponent::getComp()
{
    return this->comp;
}

int ILComponent::waitForEvent(OMX_EVENTTYPE event, OMX_U32 nData1, int ignore1, OMX_IN OMX_U32 nData2, int ignore2, int event_flag, int suspend)
{
    int32_t status;
    uint32_t set;

    while (this->removeEvent(event, nData1, ignore1, nData2, ignore2) < 0)
    {
        // if we want to be notified of errors, check the list for an error now
        // before blocking, the event flag may have been cleared already.
        if(event_flag & ILCLIENT_EVENT_ERROR)
        {
            ILEVENT_T *cur;
            this->client->lockEvents();
            cur = comp->list;
            while(cur && cur->eEvent != OMX_EventError)
            {
                cur = cur->next;
            }

            if(cur)
            {
                // clear error flag
                vcos_event_flags_get(&comp->event, ILCLIENT_EVENT_ERROR, VCOS_OR_CONSUME, 0, &set);
                this->client->unlockEvents();
                return -2;
            }

            this->client->unlockEvents();
        }
        // check for config change event if we are asked to be notified of that
        if(event_flag & ILCLIENT_CONFIG_CHANGED)
        {
            ILEVENT_T *cur;
            this->client->lockEvents();
            cur = comp->list;
            while(cur && cur->eEvent != OMX_EventParamOrConfigChanged)
            {
                cur = cur->next;
            }

            this->client->unlockEvents();

            if(cur)
            {
                return this->removeEvent(event, nData1, ignore1, nData2, ignore2) == 0 ? 0 : -3;
            }
        }

        status = vcos_event_flags_get(&comp->event, event_flag, VCOS_OR_CONSUME,
                                      suspend, &set);
        if (status != 0)
        {
            return -1;
        }
        else if (set & ILCLIENT_EVENT_ERROR)
        {
            return -2;
        }
        else if (set & ILCLIENT_CONFIG_CHANGED)
        {
            return this->removeEvent(event, nData1, ignore1, nData2, ignore2) == 0 ? 0 : -3;
        }
    }

    return 0;
}
int ILComponent::enableTunnel(ILTunnel * tunnel)
{
    OMX_STATETYPE state;
    OMX_ERRORTYPE error;

    this->client->debugOutput("ilclient: enable tunnel from %x/%d to %x/%d",
                          tunnel->sourceComponent->getComp(), tunnel->sourcePort,
                          tunnel->sinkComponent->getComp(), tunnel->sinkPort);

    error = OMX_SendCommand(tunnel->sourceComponent->getComp()->comp, OMX_CommandPortEnable, tunnel->sourcePort, NULL);
    vc_assert(error == OMX_ErrorNone);

    error = OMX_SendCommand(tunnel->sinkComponent->getComp()->comp, OMX_CommandPortEnable, tunnel->sinkPort, NULL);
    vc_assert(error == OMX_ErrorNone);

    // to complete, the sink component can't be in loaded state
    error = OMX_GetState(tunnel->sinkComponent->getComp()->comp, &state);
    vc_assert(error == OMX_ErrorNone);
    if (state == OMX_StateLoaded)
    {
        int ret = 0;

        if(tunnel->sinkComponent->waitForCommandComplete(OMX_CommandPortEnable, tunnel->sinkPort) != 0 ||
           OMX_SendCommand(tunnel->sinkComponent->getComp()->comp, OMX_CommandStateSet, OMX_StateIdle, NULL) != OMX_ErrorNone ||
           (ret = tunnel->sinkComponent->waitForCommandCompleteDual(OMX_CommandStateSet, OMX_StateIdle, tunnel->sourceComponent->getComp())) < 0)
        {
            if(ret == -2)
            {
                // the error was reported fom the source component: clear this error and disable the sink component
                tunnel->sourceComponent->waitForCommandComplete(OMX_CommandPortEnable, tunnel->sourcePort);
                tunnel->sinkComponent->disablePort(tunnel->sinkPort);
            }

            tunnel->sourceComponent->disablePort(tunnel->sourcePort);
            throw ILComponentException(std::string("ilclient: could not change component state to IDLE"));
        }
    }
    else
    {
        if (tunnel->sinkComponent->waitForCommandComplete(OMX_CommandPortEnable, tunnel->sinkPort) != 0)
        {
            //Oops failed to enable the sink port
            tunnel->sourceComponent->disablePort(tunnel->sourcePort);
            tunnel->sourceComponent->waitForEvent(OMX_EventCmdComplete,
                                                  OMX_CommandPortEnable, 0, tunnel->sourcePort, 0,
                                                  ILCLIENT_PORT_ENABLED | ILCLIENT_EVENT_ERROR, VCOS_EVENT_FLAGS_SUSPEND);
            throw ILComponentException(std::string("Could not change sink port port to enabled"));
        }
    }

    if(tunnel->sourceComponent->waitForCommandComplete(OMX_CommandPortEnable, tunnel->sourcePort) != 0)
    {
        tunnel->sinkComponent->disablePort(tunnel->sinkPort);
        throw ILComponentException(std::string("Could not change source port to enabled"));
    }

    return 0;
}
int ILComponent::disableTunnel(ILTunnel * tunnel)
{
    OMX_ERRORTYPE error;

    if(tunnel->sourceComponent->getComp() == nullptr || tunnel->sinkComponent->getComp() == nullptr)
        return 0;

    tunnel->sourceComponent->getComp()->error_mask |= ILCLIENT_ERROR_UNPOPULATED;
    tunnel->sinkComponent->getComp()->error_mask |= ILCLIENT_ERROR_UNPOPULATED;

    error = OMX_SendCommand(tunnel->sourceComponent->getComp()->comp, OMX_CommandPortDisable, tunnel->sourcePort, NULL);
    vc_assert(error == OMX_ErrorNone);

    error = OMX_SendCommand(tunnel->sinkComponent->getComp()->comp, OMX_CommandPortDisable, tunnel->sinkPort, NULL);
    vc_assert(error == OMX_ErrorNone);

    if(tunnel->sourceComponent->waitForCommandComplete(OMX_CommandPortDisable, tunnel->sourcePort) < 0)
        vc_assert(0);

    if(tunnel->sinkComponent->waitForCommandComplete(OMX_CommandPortDisable, tunnel->sinkPort) < 0)
        vc_assert(0);

    tunnel->sourceComponent->getComp()->error_mask &= ~ILCLIENT_ERROR_UNPOPULATED;
    tunnel->sinkComponent->getComp()->error_mask &= ~ILCLIENT_ERROR_UNPOPULATED;

    return 0;
}

OMX_ERRORTYPE ILComponent::setOMXParameter(OMX_INDEXTYPE paramIndex, void * paramStruct)
{
    return OMX_SetParameter(this->comp->comp, paramIndex, paramStruct);
}

OMX_ERRORTYPE ILComponent::getOMXParameter(OMX_INDEXTYPE paramIndex, void * paramStruct)
{
    return OMX_GetParameter(this->comp->comp, paramIndex, paramStruct);
}

OMX_ERRORTYPE ILComponent::setOMXConfig(OMX_INDEXTYPE configIndex, void * configStruct)
{
    return OMX_SetConfig(this->comp->comp, configIndex, configStruct);
}

int ILComponent::enablePortBuffers(int portIndex, ILCLIENT_MALLOC_T ilclient_malloc, ILCLIENT_FREE_T ilclient_free, void * priv)
{
    OMX_ERRORTYPE error;
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    OMX_BUFFERHEADERTYPE *list = nullptr, **end = &list;
    OMX_STATETYPE state;
    int i;

    memset(&portdef, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    portdef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = portIndex;

    // work out buffer requirements, check port is in the right state
    error = OMX_GetParameter(this->comp->comp, OMX_IndexParamPortDefinition, &portdef);
    if(error != OMX_ErrorNone || portdef.bEnabled != OMX_FALSE || portdef.nBufferCountActual == 0 || portdef.nBufferSize == 0)
        return -1;

    // check component is in the right state to accept buffers
    error = OMX_GetState(this->comp->comp, &state);
    if (error != OMX_ErrorNone || !(state == OMX_StateIdle || state == OMX_StateExecuting || state == OMX_StatePause))
        return -1;

    // send the command
    error = OMX_SendCommand(this->comp->comp, OMX_CommandPortEnable, portIndex, NULL);
    vc_assert(error == OMX_ErrorNone);

    for (i=0; i != portdef.nBufferCountActual; i++)
    {
        unsigned char *buf;
        if(ilclient_malloc)
        {
            buf = (unsigned char *) ilclient_malloc(priv, portdef.nBufferSize, portdef.nBufferAlignment, this->comp->bufname);
        }
        else
        {
            buf = (unsigned char *) vcos_malloc_aligned(portdef.nBufferSize, portdef.nBufferAlignment, this->comp->bufname);
        }

        if(!buf)
        {
            break;
        }

        error = OMX_UseBuffer(this->comp->comp, end, portIndex, NULL, portdef.nBufferSize, buf);
        if(error != OMX_ErrorNone)
        {
            if(ilclient_free)
                ilclient_free(priv, buf);
            else
            vcos_free(buf);

            break;
        }
        end = (OMX_BUFFERHEADERTYPE **) &((*end)->pAppPrivate);
    }

    // queue these buffers
    vcos_semaphore_wait(&comp->sema);

    if(portdef.eDir == OMX_DirInput)
    {
        *end = comp->in_list;
        comp->in_list = list;
    }
    else
    {
        *end = comp->out_list;
        comp->out_list = list;
    }

    vcos_semaphore_post(&comp->sema);

    if(i != portdef.nBufferCountActual ||
       this->waitForCommandComplete(OMX_CommandPortEnable, portIndex) < 0)
    {
        this->disablePortBuffers(portIndex, NULL, ilclient_free, priv);

        // at this point the first command might have terminated with an error, which means that
        // the port is disabled before the disable_port_buffers function is called, so we're left
        // with the error bit set and an error event in the queue.  Clear these now if they exist.
        this->removeEvent(OMX_EventError, 0, 1, 1, 0);

        return -1;
    }

    // success
    return 0;
}

int ILComponent::disablePortBuffers(int portIndex, OMX_BUFFERHEADERTYPE *bufferList, ILCLIENT_FREE_T ilclient_free, void *priv)
{
    OMX_ERRORTYPE error;
    OMX_BUFFERHEADERTYPE *list = bufferList;
    OMX_BUFFERHEADERTYPE **head, *clist, *prev;
    OMX_PARAM_PORTDEFINITIONTYPE portdef;
    int num;

    // get the buffers off the relevant queue
    memset(&portdef, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    portdef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    portdef.nVersion.nVersion = OMX_VERSION;
    portdef.nPortIndex = portIndex;

    // work out buffer requirements, check port is in the right state
    error = OMX_GetParameter(this->comp->comp, OMX_IndexParamPortDefinition, &portdef);
    if(error != OMX_ErrorNone || portdef.bEnabled != OMX_TRUE || portdef.nBufferCountActual == 0 || portdef.nBufferSize == 0)
    {
        return error;
    }

    num = portdef.nBufferCountActual;

    error = OMX_SendCommand(this->comp->comp, OMX_CommandPortDisable, portIndex, nullptr);
    vc_assert(error == OMX_ErrorNone);

    while(num > 0)
    {
        VCOS_UNSIGNED set;

        if(list == nullptr)
        {
            vcos_semaphore_wait(&comp->sema);

            // take buffers for this port off the relevant queue
            head = portdef.eDir == OMX_DirInput ? &comp->in_list : &comp->out_list;
            clist = *head;
            prev = nullptr;

            while(clist)
            {
                if((portdef.eDir == OMX_DirInput ? clist->nInputPortIndex : clist->nOutputPortIndex) == portIndex)
                {
                    OMX_BUFFERHEADERTYPE *pBuffer = clist;

                    if(!prev)
                    {
                        clist = *head = (OMX_BUFFERHEADERTYPE *) pBuffer->pAppPrivate;
                    }
                    else
                    {
                        clist = (OMX_BUFFERHEADERTYPE *) pBuffer->pAppPrivate;
                    }
                    prev->pAppPrivate = (OMX_PTR) clist;

                    pBuffer->pAppPrivate = list;
                    list = pBuffer;
                }
                else
                {
                    prev = clist;
                    clist = (OMX_BUFFERHEADERTYPE *) &(clist->pAppPrivate);
                }
            }

            vcos_semaphore_post(&comp->sema);
        }

        while(list)
        {
            void *buf = list->pBuffer;
            OMX_BUFFERHEADERTYPE *next = (OMX_BUFFERHEADERTYPE *) list->pAppPrivate;

            error = OMX_FreeBuffer(comp->comp, portIndex, list);
            vc_assert(error == OMX_ErrorNone);

            if(ilclient_free)
                ilclient_free(priv, buf);
            else
            vcos_free(buf);

            num--;
            list = next;
        }

        if(num)
        {
            OMX_U32 mask = ILCLIENT_PORT_DISABLED | ILCLIENT_EVENT_ERROR;
            mask |= (portdef.eDir == OMX_DirInput ? ILCLIENT_EMPTY_BUFFER_DONE : ILCLIENT_FILL_BUFFER_DONE);

            // also wait for command complete/error in case we didn't have all the buffers allocated
            vcos_event_flags_get(&comp->event, mask, VCOS_OR_CONSUME, -1, &set);

            if((set & ILCLIENT_EVENT_ERROR) && this->removeEvent(OMX_EventError, 0, 1, 1, 0) >= 0)
                return 0;

            if((set & ILCLIENT_PORT_DISABLED) && this->removeEvent(OMX_EventCmdComplete, OMX_CommandPortDisable, 0, portIndex, 0) >= 0)
                return 0;
        }
    }

    if(this->waitForCommandComplete(OMX_CommandPortDisable, portIndex) < 0)
        vc_assert(0);

    return 0;
}

OMX_BUFFERHEADERTYPE * ILComponent::getInputBuffer(int portIndex, int block)
{
    OMX_BUFFERHEADERTYPE *ret = nullptr, *prev = nullptr;

    do {
        VCOS_UNSIGNED set;

        vcos_semaphore_wait(&comp->sema);
        ret = comp->in_list;
        while(ret != nullptr && ret->nInputPortIndex != portIndex)
        {
            prev = ret;
            ret = (OMX_BUFFERHEADERTYPE *)ret->pAppPrivate;
        }

        if(ret)
        {
            if(prev == nullptr)
            {
                comp->in_list = (OMX_BUFFERHEADERTYPE *)ret->pAppPrivate;
            }
            else
            {
                prev->pAppPrivate = ret->pAppPrivate;
            }

            ret->pAppPrivate = nullptr;
        }
        vcos_semaphore_post(&comp->sema);

        if(block && !ret)
            vcos_event_flags_get(&comp->event, ILCLIENT_EMPTY_BUFFER_DONE, VCOS_OR_CONSUME, -1, &set);

    } while(block && !ret);

    return ret;
}
OMX_ERRORTYPE ILComponent::emptyBuffer(OMX_BUFFERHEADERTYPE *buf)
{
    return OMX_EmptyThisBuffer(this->comp->comp, buf);
}