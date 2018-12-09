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
#include <cstring>
#include <stdio.h>
#include <string>
#include "ILClientException.h"
#include <bcm_host.h>

class ILClient
{
    static bool initDone;

public:
    ILClient()
    {
            if (!ILClient::initDone)
            {
                    ILClient::initDone = true;
                    bcm_host_init();
            }

            if (!this->init())
            {
                    throw (ILClientException(std::string("Unable to initialise client")));
            }
            unsigned int err = OMX_Init();
            if (err != OMX_ErrorNone)
            {
                    this->destroy();
                    char buffer[100];
                    snprintf(buffer, 100, "Unable to initialize OMX %x", err);
                    throw (ILClientException(std::string(buffer)));
            }
    }

    ~ILClient()
    {
            this->destroy();
            OMX_Deinit();
    }

    ILCLIENT_T * getClient()
    {
            return this->client;
    }

    void lockEvents()
    {
            vcos_semaphore_wait(&this->client->event_sema);
    }

    void unlockEvents()
    {
            vcos_semaphore_post(&this->client->event_sema);
    }

    void debugOutput(char * format, ...)
    {
            va_list args;

            va_start(args, format);
            vcos_vlog_info(format, args);
            va_end(args);
            fflush(stdout);
    }

private:

    bool init()
    {
            this->client = (ILCLIENT_T *)vcos_malloc(sizeof(ILCLIENT_T), "ilclient");
            int i;

            if (!this->client)
            {
                    return false;
            }

            vcos_log_set_level(VCOS_LOG_CATEGORY, VCOS_LOG_WARN);
            vcos_log_register("ilclient", VCOS_LOG_CATEGORY);

            memset(this->client, 0, sizeof(ILCLIENT_T));

            i = vcos_semaphore_create(&this->client->event_sema, "il:event", 1);
            vc_assert(i == VCOS_SUCCESS);

            this->lockEvents();
            this->client->event_list = nullptr;
            for (i = 0; i < NUM_EVENTS; i++)
            {
                    this->client->event_rep[i].eEvent = (OMX_EVENTTYPE) - 1; // mark as unused
                    this->client->event_rep[i].next = this->client->event_list;
                    this->client->event_list = this->client->event_rep + i;
            }
            this->unlockEvents();
            return true;
    }

    void destroy()
    {
            if (this->client)
            {
                    vcos_semaphore_delete(&this->client->event_sema);
                    vcos_free(this->client);
                    this->client = nullptr;
                    vcos_log_unregister(VCOS_LOG_CATEGORY);
            }
    }

    ILCLIENT_T * client = nullptr;
};

