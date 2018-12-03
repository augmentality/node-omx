#include "libomx/NativePlayer.h"

int main (int argc, char **argv)
{
    NativePlayer * n = new NativePlayer(std::string("http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_30fps_normal.mp4"));

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


    // while that is playing, load another
    NativePlayer * n2 = new NativePlayer(std::string("http://movies.casperdns.com/sintel.mp4"));

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
    printf("Stopping, cleanup\n");
    fflush(stdout);
    delete n;
    printf("Starting second vid\n");
    fflush(stdout);
    n2->play();
    sleep(10);
    printf("Killing second video\n");
    fflush(stdout);
    delete n2;
    printf("DONE");
    fflush(stdout);
}