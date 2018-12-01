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
                        throw(ILClientException(std::string("Unable to initialise client")));
                }
                unsigned int err = OMX_Init();
                if (err != OMX_ErrorNone)
                {
                        this->destroy();
                        char buffer [100];
                        snprintf(buffer, 100, "Unable to initialize OMX %x", err);
                        throw(ILClientException(std::string(buffer)));
                }
        }

        ~ILClient()
        {
                this->destroy();
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
                this->client = (ILCLIENT_T *) vcos_malloc(sizeof(ILCLIENT_T), "ilclient");
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

