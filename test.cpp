//
// node-omx - A media player for node.js on the Raspberry Pi
// Copyright (C) 2018 Augmentality Ltd <info@augmentality.uk>
//
// This file is part of node-omx.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; version 2.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
// Street, Fifth Floor, Boston, MA 02110-1301, USA.


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
