#include "libomx/NativePlayer.h"

int main (int argc, char **argv)
{
    //NativePlayer * n = new NativePlayer(std::string("http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_30fps_normal.mp4"));
    //NativePlayer * n = new NativePlayer(std::string("./test.h264"));
    //NativePlayer * n = new NativePlayer(std::string("http://movies.casperpanel.com/epicloop.mp4"));
    NativePlayer * n = new NativePlayer(std::string("trump.mp4"));

    int delay = 5;
    do
    {
        printf("Starting in %d\n", delay);
        fflush(stdout);
        delay--;
        usleep(1000000);
    }
    while(delay > 0);

    n->play();
    sleep(10);
    printf("Pausing\n");
    n->pause();
    fflush(stdout);
    sleep(5);
    printf("Resuming\n");
    fflush(stdout);
    n->play();
    sleep(5);
    printf("Speeding up\n");
    fflush(stdout);
    n->setSpeed(1.1);
    sleep(5);
    printf("Slowing Down\n");
    fflush(stdout);
    n->setSpeed(0.9);
    sleep(5);
    printf("Stopping, cleanup");
    fflush(stdout);
    delete n;
    printf("done?");
    fflush(stdout);
}