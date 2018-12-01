#include "libomx/NativePlayer.h"

int main (int argc, char **argv)
{
    //NativePlayer * n = new NativePlayer(std::string("http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_30fps_normal.mp4"));
    NativePlayer * n = new NativePlayer(std::string("./test.h264"));
    n->play();
    while(1)
    {
        usleep(10000);
    }
}