#include "VideoSchedulerComponent.h"

VideoSchedulerComponent::VideoSchedulerComponent(ILClient *client): ILComponent(client, std::string("video_scheduler"), (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS))
{

}
