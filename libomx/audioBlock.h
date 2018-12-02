#pragma once

struct AudioBlock
{
    int audioFormat;
    size_t dataSize;
    int sampleRate;
    int channels;
    uint8_t * data;
    int sampleCount;
    int streamCount;
    double pts;
    double dts;
    double duration;
    bool looped;
};
