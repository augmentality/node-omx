#include "VideoThread.h"

VideoThread::VideoThread(ILClient * pClient, ClockComponent * pClock, FFSource * src):
    client(pClient), clock(pClock)
{
    this->vdc = new VideoDecodeComponent(this->client);
    this->vdr = new VideoRenderComponent(this->client);
    this->sched = new VideoSchedulerComponent(this->client);
    this->clockSchedTunnel = this->clock->tunnelTo(80, this->sched, 12, 0, 0);
    this->vdc->changeState(OMX_StateIdle);
    this->vdc->setVideoCompressionFormat(src->codingType, (OMX_COLOR_FORMATTYPE)0, src->fpsrate / src->fpsscale, src->width, src->height);
    this->vdc->enablePortBuffers(130, nullptr, nullptr, nullptr);
}

void VideoThread::start()
{
    this->videoThread = std::thread(&VideoThread::videoThreadFunc, this);
}

VideoBlock * VideoThread::dequeue()
{
    std::unique_lock<std::mutex> lk(videoQueueMutex);
    if (videoQueue.empty())
    {
        while (!playbackComplete && videoQueue.empty())
        {
            videoReady.wait_for(lk,  std::chrono::milliseconds(100), [&]()
            {
                return !videoQueue.empty();
            });
        }
    }
    if (playbackComplete || videoQueue.empty())
    {
        return nullptr;
    }
    else
    {
        VideoBlock * b = videoQueue.front();
        videoQueue.pop();
        if (videoQueue.size() < 200 && !bufferReady)
        {
            std::unique_lock<std::mutex> lk(readyForDataMutex);
            bufferReady = true;
            readyForData.notify_one();
        }
        return b;
    }
}
void VideoThread::waitForBuffer()
{
    std::unique_lock<std::mutex> lk(this->readyForDataMutex);
    if (bufferReady)
    {
        return;
    }
    while (!playbackComplete && !bufferReady)
    {
        readyForData.wait_for(lk,  std::chrono::milliseconds(100), [&]()
        {
            return bufferReady;
        });
    }
}

void VideoThread::videoThreadFunc()
{
    this->vdc->changeState(OMX_StateExecuting);
    OMX_BUFFERHEADERTYPE *buf = nullptr;
    int port_settings_changed = 0;
    int audio_port_settings_changed = 0;
    int first_packet = 1;
    unsigned int data_len = 0;
    while (!playbackComplete)
    {
        VideoBlock * block = dequeue();
        if (block != nullptr)
        {
            int readBytes = 0;
            int pSize = block->dataSize;
            bool error = false;
            while (pSize != 0)
            {
                buf = vdc->getInputBuffer(130, 1);
                if (buf == nullptr)
                    break;
                buf->nFlags = 0;
                buf->nOffset = 0;
                buf->nFilledLen = (pSize > buf->nAllocLen) ? buf->nAllocLen : pSize;
                memcpy(buf->pBuffer, block->data + readBytes, buf->nFilledLen);
                pSize -= buf->nFilledLen;
                readBytes += buf->nFilledLen;
                data_len += buf->nFilledLen;
                if (port_settings_changed == 0 &&
                    ((data_len > 0 && vdc->removeEvent(OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
                     (data_len == 0 && vdc->waitForEvent(OMX_EventPortSettingsChanged, 131, 0, 0, 1,
                                                         ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) ==
                                       0)))
                {
                    port_settings_changed = 1;

                    decodeTunnel = vdc->tunnelTo(131, sched, 10, 0, 0);
                    schedTunnel = sched->tunnelTo(11, vdr, 90, 0, 1000);
                    sched->changeState(OMX_StateExecuting);
                    vdr->changeState(OMX_StateExecuting);
                }

                if (first_packet)
                {
                    buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
                    first_packet = 0;
                }
                else
                {
                    if (block->pts == DVD_NOPTS_VALUE && block->dts == DVD_NOPTS_VALUE)
                    {
                        buf->nFlags |= OMX_BUFFERFLAG_TIME_UNKNOWN;
                    }
                    else if (block->pts == DVD_NOPTS_VALUE)
                    {
                        buf->nFlags |= OMX_BUFFERFLAG_TIME_IS_DTS;
                    }
                }
                data_len = 0;

                static uint64_t baseTime = 0;
                static uint64_t lastTime = 0;


                uint64_t timestamp = (uint64_t)(
                        block->pts != DVD_NOPTS_VALUE ? block->pts : block->dts != DVD_NOPTS_VALUE ? block->dts : 0);

                if (block->looped)
                {
                    baseTime += lastTime;
                }
                lastTime = timestamp;
                timestamp += baseTime;

                if (!(buf->nFlags & OMX_BUFFERFLAG_TIME_UNKNOWN))
                {
                    buf->nTimeStamp = ToOMXTime(timestamp);
                }
                if (vdc->emptyBuffer(buf) != OMX_ErrorNone)
                {
                    error = true;
                    break;
                }

            }
            delete[] block->data;
            delete block;
            if (error)
            {
                break;
            }
            if (buf == nullptr)
            {
                return;
            }
        }
    }

    this->vdc->changeState(OMX_StateIdle);
}
void VideoThread::addData(VideoBlock * vb)
{
    waitForBuffer();
    {
        std::unique_lock<std::mutex> lk(videoQueueMutex);
        videoQueue.push(vb);
        if (videoQueue.size() > 200)
        {
            bufferReady = false;
        }
        videoReady.notify_one();
    }
}
VideoThread::~VideoThread()
{
    this->playbackComplete = true;
    this->videoThread.join();

    if (this->decodeTunnel != nullptr)
    {
        delete this->decodeTunnel;
        this->decodeTunnel = nullptr;
    }
    if (this->schedTunnel != nullptr)
    {
        delete this->schedTunnel;
        this->schedTunnel = nullptr;
    }

    if (this->clockSchedTunnel != nullptr)
    {
        delete this->clockSchedTunnel;
    }
    if (this->vdr != nullptr)
    {
        delete this->vdr;
        this->vdr = nullptr;
    }
    if (this->vdc != nullptr)
    {
        delete this->vdc;
        this->vdc = nullptr;
    }
    if (this->sched != nullptr)
    {
        delete this->sched;
        this->sched = nullptr;
    }
}