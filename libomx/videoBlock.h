#pragma once

struct VideoBlock
{
    size_t dataSize;
    uint8_t * data;
    double pts;
    double dts;
    double duration;
    bool looped;
};