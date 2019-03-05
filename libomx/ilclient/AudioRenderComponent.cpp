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

#include "AudioRenderComponent.h"

AudioRenderComponent::AudioRenderComponent(ILClient *client): ILComponent(client, std::string("audio_render"), (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_ENABLE_INPUT_BUFFERS | ILCLIENT_DISABLE_ALL_PORTS))
{

}

void AudioRenderComponent::setPCMMode(int sampleRate, int channelCount, AVSampleFormat format)
{
    int bitDepth = 8;
    bool bitSigned = true;
    bool planar = false;
    switch(format)
    {
        case AVSampleFormat::AV_SAMPLE_FMT_U8:
            bitSigned = false;
            break;
        case AVSampleFormat::AV_SAMPLE_FMT_S16:
            bitDepth = 16;
            break;
        case AVSampleFormat::AV_SAMPLE_FMT_S32:
            bitDepth = 32;
            break;
        case AVSampleFormat::AV_SAMPLE_FMT_FLT:
            bitDepth = 32;
            break;
        case AVSampleFormat::AV_SAMPLE_FMT_DBL:
            bitDepth = 64;
            break;
        case AVSampleFormat::AV_SAMPLE_FMT_U8P:
            planar = true;
            bitSigned = false;
            break;
        case AVSampleFormat::AV_SAMPLE_FMT_S16P:
            planar = true;
            bitDepth = 16;
            bitSigned = true;
            break;
        case AVSampleFormat::AV_SAMPLE_FMT_S32P:
            planar = true;
            bitDepth = 32;
            break;
        case AVSampleFormat::AV_SAMPLE_FMT_FLTP:
            planar = true;
            bitDepth = 32;
            break;
        case AVSampleFormat::AV_SAMPLE_FMT_DBLP:
            planar = true;
            bitDepth = 64;
            break;
    }

    OMX_AUDIO_PARAM_PCMMODETYPE pcm;
    memset(&pcm, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
    pcm.nSize = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
    pcm.nVersion.nVersion = OMX_VERSION;
    pcm.nPortIndex = 100;
    pcm.nChannels = channelCount;
    pcm.eNumData = (bitSigned) ? OMX_NumericalDataSigned : OMX_NumericalDataUnsigned;
    pcm.eEndian = OMX_EndianLittle;
    pcm.nSamplingRate = sampleRate;
    pcm.bInterleaved = (planar) ? OMX_FALSE : OMX_TRUE;
    pcm.nBitPerSample = bitDepth;
    pcm.ePCMMode = OMX_AUDIO_PCMModeLinear;
    switch(channelCount)
    {
        case 1:
            pcm.eChannelMapping[0] = OMX_AUDIO_ChannelCF;
            break;
        case 3:
            pcm.eChannelMapping[2] = OMX_AUDIO_ChannelCF;
            pcm.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
            pcm.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
            break;
        case 8:
            pcm.eChannelMapping[7] = OMX_AUDIO_ChannelRS;
        case 7:
            pcm.eChannelMapping[6] = OMX_AUDIO_ChannelLS;
        case 6:
            pcm.eChannelMapping[5] = OMX_AUDIO_ChannelRR;
        case 5:
            pcm.eChannelMapping[4] = OMX_AUDIO_ChannelLR;
        case 4:
            pcm.eChannelMapping[3] = OMX_AUDIO_ChannelLFE;
            pcm.eChannelMapping[2] = OMX_AUDIO_ChannelCF;
        case 2:
            pcm.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
            pcm.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
            break;
    }
    this->setOMXParameter(OMX_IndexParamAudioPcm, &pcm);

}
void AudioRenderComponent::setAudioDest(const char *dest)
{
    OMX_CONFIG_BRCMAUDIODESTINATIONTYPE ar_dest;
    memset(&ar_dest, 0, sizeof(ar_dest));
    ar_dest.nSize = sizeof(OMX_CONFIG_BRCMAUDIODESTINATIONTYPE);
    ar_dest.nVersion.nVersion = OMX_VERSION;
    strcpy((char *)ar_dest.sName, dest);
    this->setOMXConfig(OMX_IndexConfigBrcmAudioDestination, &ar_dest);
}
void AudioRenderComponent::waitForEOS()
{
    this->waitForEvent(OMX_EventBufferFlag, 101, 0, OMX_BUFFERFLAG_EOS, 0, ILCLIENT_BUFFER_FLAG_EOS, -1);
}