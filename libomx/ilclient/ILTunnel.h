#pragma once

class ILComponent;

struct ILTunnel
{
    ILComponent * sourceComponent;
    int sourcePort;
    ILComponent * sinkComponent;
    int sinkPort;
};