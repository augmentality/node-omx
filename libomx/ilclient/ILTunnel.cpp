#include "ILTunnel.h"
#include "ILComponent.h"

ILTunnel::~ILTunnel()
{
    OMX_SendCommand(sourceComponent->getComp(), OMX_CommandPortDisable, sourcePort, NULL);
    OMX_SendCommand(sinkComponent->getComp(), OMX_CommandPortDisable, sinkPort, NULL);
    sourceComponent->waitForCommandComplete(OMX_CommandPortDisable, sourcePort);
    sinkComponent->waitForCommandComplete(OMX_CommandPortDisable, sinkPort);
}
