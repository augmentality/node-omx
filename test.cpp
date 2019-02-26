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
    for (int x = 0; x < 3; x++)
    {
        printf("Loading player"); fflush(stdout);
        NativePlayer * n = new NativePlayer(std::string("/augmentality/storage/briefing.mp4"), [&]()
        {
            printf("Playback Completed");fflush(stdout);
        });

        int delay = 2;
        do
        {
            printf("Starting in %d\n", delay); fflush(stdout);
            fflush(stdout);
            delay--;
            usleep(1000000);
        } while (delay > 0);

        n->play();

        printf("Playing for 5 seconds"); fflush(stdout);
        sleep(5);
        printf("Stopping");fflush(stdout);
        delete n;
        sleep(5);
        printf("Waiting a bit.."); fflush(stdout);
    }
    printf("All done"); fflush(stdout);
    return 0;
}
