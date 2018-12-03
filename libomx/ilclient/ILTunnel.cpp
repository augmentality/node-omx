#include "ILTunnel.h"
#include "ILComponent.h"

ILTunnel::~ILTunnel()
{
    OMX_SendCommand(sourceComponent->getComp()->comp, OMX_CommandPortDisable, sourcePort, NULL);
    OMX_SendCommand(sinkComponent->getComp()->comp, OMX_CommandPortDisable, sinkPort, NULL);
    sourceComponent->waitForCommandComplete(OMX_CommandPortDisable, sourcePort);
    sinkComponent->waitForCommandComplete(OMX_CommandPortDisable, sinkPort);
}
