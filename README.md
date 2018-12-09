# node-omx

[![npm version](https://badge.fury.io/js/%40augmentality%2Fnode-omx.svg)](https://badge.fury.io/js/%40caspertech%2Fnode-omx)
[![Known Vulnerabilities](https://snyk.io/test/npm/@augmentality/node-omx/badge.svg)](https://snyk.io/test/npm/@augmentality/node-omx)
[![Dependencies](https://david-dm.org/Augmentality/node-omx.svg)](https://david-dm.org/Augmentality/node-omx.svg)

> A Node.JS video player for the Raspberry Pi (and other devices using Broadcom's OpenMax-based Multi-Media Abstraction Layer API)

This is a compiled module for NodeJS which provides hardware-accelerated video and audio playback on the Pi. Everything is built-in, it doesn't shell out to other applications.

## Install

```bash
npm install --save @augmentality/node-omx
```

## Features

* Asynchronous loading, playback and control
* Instant start (once loaded)
* Network or local playback
* Seamless looping with a/v sync
* Pause / Resume
* Get playback time
* Callbacks for play state
* Seamless switching between videos with multiple instances
* Supports mp4, mkv, mov, or any container supported by ffmpeg

## Prerequisites

* Working C++ toolchain (apt-get install build-essential)
* ffmpeg (apt-get install libavcodec-dev)
* cmake (apt-get install cmake)

## Supported Platforms

Any device supporting Broadcom's OpenMax-based Multi-Media Abstraction Layer API. 

For convenience, on unsupported platforms, the module will still build and will output placeholder messages instead of playing media.

## Usage Example

Javascript

```javascript
const omx = require('@augmentality/node-omx');
const readline = require('readline');

readline.emitKeypressEvents(process.stdin);

let state = 0;
async function run()
{
    const n = new omx.Player();


    n.onPlaybackState.subscribe((pState) =>
    {
        state = pState;
        switch(state)
        {
            case 0:
                console.log('Stopped');
                break;
            case 1:
                console.log('File loaded');
                break;
            case 2:
                console.log('Playing');
                break;
            case 3:
                console.log('Paused');
                break;
        }
    });

    await n.open('http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_30fps_normal.mp4');
    n.play();
    n.setLoop(true); // Enable seamless looping

    setInterval(() =>
    {
        if (state > 0)
        {
            console.log('Playback position: ' + n.getTime());
        }
    }, 1000);

    let speed = 1.0;
    let paused = false;
    process.stdin.setRawMode(true);
    process.stdin.on('keypress', (str, key) =>
    {
        if (key.ctrl && key.name === 'q')
        {
            process.exit();
        }
        else
        {
            switch(key.name)
            {
                case 'up':
                    speed += 0.01;
                    console.log('Playback speed: ' + speed);
                    n.setSpeed(speed);
                    break;
                case 'down':
                    speed -= 0.01;
                    console.log('Playback speed: ' + speed);
                    n.setSpeed(speed);
                    break;
                case 'space':
                    if (!paused)
                    {
                        n.pause();
                        paused = true;
                    }
                    else
                    {
                        n.play();
                        paused = false;
                    }
                    break;
            }
        }
    });
}
run().then({});
```

## Caveats

* A read-ahead buffer is implemented to help smooth playback. If you disable looping, the reader may have already looped, in which case playback will end at the end of the next run.
* Only H264 video is supported at present.
* Looping is not possible when playing h264 annex-b (raw H264). Mux into a container if you need this.
