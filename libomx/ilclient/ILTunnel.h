#pragma once
#include "ilc.h"
class ILComponent;

struct ILTunnel
{
    ILComponent * sourceComponent;
    int sourcePort;
    ILComponent * sinkComponent;
    int sinkPort;

    ~ILTunnel();
};