#include "libomx/NativePlayer.h"

std::thread t;

int main (int argc, char **argv)
{
    NativePlayer * n;
    auto exitFunc = [&](){
        n->waitForCompletion();
        printf("Playback truly finished");
        delete n;
    };
    n = new NativePlayer(std::string("http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_30fps_normal.mp4"), [&](){
        printf("Playback Completed");
        t = std::thread(exitFunc);
    });

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
    sleep(30);
    t.join();
}
