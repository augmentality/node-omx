#include "VideoRenderComponent.h"

VideoRenderComponent::VideoRenderComponent(ILClient *client): ILComponent(client, std::string("video_render"), (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS))
{

}
