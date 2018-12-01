#include "VideoDecodeComponent.h"

VideoDecodeComponent::VideoDecodeComponent(ILClient *client): ILComponent(client, std::string("video_decode"), (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS))
{

}

void VideoDecodeComponent::setVideoCompressionFormat(OMX_VIDEO_CODINGTYPE codec, OMX_COLOR_FORMATTYPE colorFormat, float fps, int width, int height)
{
    OMX_PARAM_PORTDEFINITIONTYPE portParam;
    memset(&(portParam), 0, sizeof(portParam));
    portParam.nPortIndex = this->inputPort;

    float fifo_size = (float)80*1024*60 / (1024*1024);

    OMX_GetParameter(this->comp->comp, OMX_IndexParamPortDefinition, &portParam);

    portParam.nPortIndex = this->inputPort;
    portParam.nBufferCountActual = 80*1024*60 / portParam.nBufferSize;

    portParam.format.video.nFrameWidth  = width;
    portParam.format.video.nFrameHeight = height;

    this->setOMXParameter(OMX_IndexParamPortDefinition, &portParam);

    OMX_VIDEO_PARAM_PORTFORMATTYPE format;
    memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
    format.nVersion.nVersion = OMX_VERSION;
    format.nPortIndex = 130;
    format.eCompressionFormat = codec;
    format.xFramerate = (long long)(1 << 16) * fps;
    if (setOMXParameter(OMX_IndexParamVideoPortFormat, &format) != OMX_ErrorNone)
    {
        throw ILComponentException(std::string("Unable to set video compression format"));
    }
}