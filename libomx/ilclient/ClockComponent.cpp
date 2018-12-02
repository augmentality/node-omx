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