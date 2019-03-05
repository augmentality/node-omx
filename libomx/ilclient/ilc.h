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

Some parts of this file Copyright (c) 2012, Broadcom Europe Ltd:

Copyright (c) 2012, Broadcom Europe Ltd
All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once

#include "IL/OMX_Broadcom.h"
#include "interface/vcos/vcos.h"
#include "interface/vcos/vcos_logging.h"
#include "interface/vmcs_host/vchost.h"

static inline OMX_TICKS ToOMXTime(int64_t pts)
{
    OMX_TICKS ticks;
    ticks.nLowPart = pts;
    ticks.nHighPart = pts >> 32;
    return ticks;
}
static inline int64_t FromOMXTime(OMX_TICKS ticks)
{
    int64_t pts = ticks.nLowPart | ((uint64_t)(ticks.nHighPart) << 32);
    return pts;
}
/**
 * The <DFN>ILCLIENT_T</DFN> structure encapsulates the state needed for the IL
 * Client API.  It contains a set of callback functions used to
 * communicate with the user.  It also includes a linked list of free
 * event structures.
 ***********************************************************/
typedef struct _ILCLIENT_T ILCLIENT_T;


/**
 * Each <DFN>ILEVENT_T</DFN> structure stores the result of an <DFN>EventHandler</DFN>
 * callback from a component, storing the event message type and any
 * parameters returned.
 ***********************************************************/
typedef struct _ILEVENT_T ILEVENT_T;


#define VCOS_LOG_CATEGORY (&ilclient_log_category)

#ifndef ILCLIENT_THREAD_DEFAULT_STACK_SIZE
    #define ILCLIENT_THREAD_DEFAULT_STACK_SIZE   (6<<10)
#endif

static VCOS_LOG_CAT_T ilclient_log_category;

#define NUM_EVENTS 100

typedef enum {
    ILCLIENT_FLAGS_NONE            = 0x0, /**< Used if no flags are
                                            set. */

    ILCLIENT_ENABLE_INPUT_BUFFERS  = 0x1, /**< If set we allow the
                                            client to communicate with
                                            input ports via buffer
                                            communication, rather than
                                            tunneling with another
                                            component. */

    ILCLIENT_ENABLE_OUTPUT_BUFFERS = 0x2, /**< If set we allow the
                                            client to communicate with
                                            output ports via buffer
                                            communication, rather than
                                            tunneling with another
                                            component. */

    ILCLIENT_DISABLE_ALL_PORTS     = 0x4, /**< If set we disable all
                                            ports on creation. */

    ILCLIENT_HOST_COMPONENT        = 0x8, /**< Create a host component.
                                            The default host ilcore
                                            only can create host components
                                            by being locally hosted
                                            so should only be used for testing
                                            purposes. */

    ILCLIENT_OUTPUT_ZERO_BUFFERS   = 0x10 /**< All output ports will have
                                            nBufferCountActual set to zero,
                                            if supported by the component. */
} ILCLIENT_CREATE_FLAGS_T;

struct _COMPONENT_T {
        OMX_HANDLETYPE comp;
        ILCLIENT_CREATE_FLAGS_T flags;
        VCOS_SEMAPHORE_T sema;
        VCOS_EVENT_FLAGS_T event;
        struct _COMPONENT_T *related;
        OMX_BUFFERHEADERTYPE *out_list;
        OMX_BUFFERHEADERTYPE *in_list;
        char name[32];
        char bufname[32];
        unsigned int error_mask;
        unsigned int priv;
        ILEVENT_T *list;
        ILCLIENT_T *client;
};

typedef struct _ILCLIENT_T ILCLIENT_T;
typedef struct _ILEVENT_T ILEVENT_T;
typedef struct _COMPONENT_T COMPONENT_T;


typedef void (*ILCLIENT_CALLBACK_T)(void *userdata, COMPONENT_T *comp, OMX_U32 data);
typedef void (*ILCLIENT_BUFFER_CALLBACK_T)(void *data, COMPONENT_T *comp);
typedef void *(*ILCLIENT_MALLOC_T)(void *userdata, VCOS_UNSIGNED size, VCOS_UNSIGNED align, const char *description);
typedef void (*ILCLIENT_FREE_T)(void *userdata, void *pointer);

/******************************************************************************
Static data and types used only in this file.
******************************************************************************/

struct _ILEVENT_T {
    OMX_EVENTTYPE eEvent;
    OMX_U32 nData1;
    OMX_U32 nData2;
    OMX_PTR pEventData;
    struct _ILEVENT_T *next;
};

#define NUM_EVENTS 100
struct _ILCLIENT_T {
    ILEVENT_T *event_list;
    VCOS_SEMAPHORE_T event_sema;
    ILEVENT_T event_rep[NUM_EVENTS];

    ILCLIENT_CALLBACK_T port_settings_callback;
    void *port_settings_callback_data;
    ILCLIENT_CALLBACK_T eos_callback;
    void *eos_callback_data;
    ILCLIENT_CALLBACK_T error_callback;
    void *error_callback_data;
    ILCLIENT_BUFFER_CALLBACK_T fill_buffer_done_callback;
    void *fill_buffer_done_callback_data;
    ILCLIENT_BUFFER_CALLBACK_T empty_buffer_done_callback;
    void *empty_buffer_done_callback_data;
    ILCLIENT_CALLBACK_T configchanged_callback;
    void *configchanged_callback_data;
};


#define random_wait()
static char *states[] = {"Invalid", "Loaded", "Idle", "Executing", "Pause", "WaitingForResources"};

typedef enum {
    ILCLIENT_ERROR_UNPOPULATED  = 0x1,
    ILCLIENT_ERROR_SAMESTATE    = 0x2,
    ILCLIENT_ERROR_BADPARAMETER = 0x4
} ILERROR_MASK_T;



/**
 * The event mask enumeration describes the possible events that the
 * user can ask to wait for when waiting for a particular event.
 ***********************************************************/
typedef enum {
    ILCLIENT_EMPTY_BUFFER_DONE  = 0x1,   /**< Set when a buffer is
                                           returned from an input
                                           port */

    ILCLIENT_FILL_BUFFER_DONE   = 0x2,   /**< Set when a buffer is
                                           returned from an output
                                           port */

    ILCLIENT_PORT_DISABLED      = 0x4,   /**< Set when a port indicates
                                           it has completed a disable
                                           command. */

    ILCLIENT_PORT_ENABLED       = 0x8,   /**< Set when a port indicates
                                           is has completed an enable
                                           command. */

    ILCLIENT_STATE_CHANGED      = 0x10,  /**< Set when a component
                                           indicates it has completed
                                           a state change command. */

    ILCLIENT_BUFFER_FLAG_EOS    = 0x20,  /**< Set when a port signals
                                           an EOS event. */

    ILCLIENT_PARAMETER_CHANGED  = 0x40,  /**< Set when a port signals a
                                           port settings changed
                                           event. */

    ILCLIENT_EVENT_ERROR        = 0x80,  /**< Set when a component
                                           indicates an error. */

    ILCLIENT_PORT_FLUSH         = 0x100, /**< Set when a port indicates
                                           is has completed a flush
                                           command. */

    ILCLIENT_MARKED_BUFFER      = 0x200, /**< Set when a port indicates
                                           it has marked a buffer. */

    ILCLIENT_BUFFER_MARK        = 0x400, /**< Set when a port indicates
                                           it has received a buffer
                                           mark. */

    ILCLIENT_CONFIG_CHANGED     = 0x800  /**< Set when a config parameter
                                           changed. */
} ILEVENT_MASK_T;



/**
 * \brief This structure represents a tunnel in the OpenMAX IL API.
 *
 * Some operations in this API act on a tunnel, so the tunnel state
 * structure (<DFN>TUNNEL_T</DFN>) is a convenient store of the source and sink
 * of the tunnel.  For each, a pointer to the relevant component state
 * structure and the port index is stored.
 ***********************************************************/
typedef struct {
    COMPONENT_T *source;  /**< The source component */
    int source_port;      /**< The output port index on the source component */
    COMPONENT_T *sink;    /**< The sink component */
    int sink_port;        /**< The input port index on the sink component */
} TUNNEL_T;

extern "C" {
    OMX_ERRORTYPE OMX_GetDebugInformation (OMX_OUT OMX_STRING debugInfo, OMX_INOUT OMX_S32 *pLen);
}
