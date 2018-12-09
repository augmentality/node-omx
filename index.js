"use strict";
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
Object.defineProperty(exports, "__esModule", { value: true });
const cmake = require("node-cmake");
const Subject_1 = require("rxjs/internal/Subject");
const omx = cmake('node_omx');
class Player {
    constructor() {
        this.onPlaybackState = new Subject_1.Subject();
        this.state = 0;
        this.url = '';
        this.p = new omx.Player();
    }
    open(url) {
        this.url = url;
        if (this.state !== 0) {
            throw new Error('URL already open. Stop first or create a new instance.');
        }
        return new Promise((resolve, reject) => {
            try {
                this.p.loadURL(url, () => {
                    this.state = 1;
                    resolve();
                }, (state) => {
                    this.onPlaybackState.next(state);
                });
            }
            catch (err) {
            }
        });
    }
    play() {
        if (this.state === 0 || this.p === null) {
            throw new Error('No file loaded');
        }
        if (this.state === 2) {
            throw new Error('Already playing');
        }
        if (this.p === null) {
            this.p = new omx.Player(this.url);
        }
        this.p.play();
        this.state = 2;
    }
    stop() {
        if (this.state === 0 || this.p === null) {
            throw new Error('No file loaded');
        }
        this.p.stop();
        this.p = null;
        this.state = 0;
    }
    pause() {
        if (this.state === 0 || this.p === null) {
            throw new Error('No file loaded');
        }
        if (this.state !== 2) {
            throw new Error('File not playing');
        }
        this.p.pause();
        this.state = 3;
    }
    setSpeed(factor) {
        if (this.state === 0 || this.p === null) {
            throw new Error('No file loaded');
        }
        this.p.setSpeed(factor);
    }
    setLoop(loop) {
        if (this.state === 0 || this.p === null) {
            throw new Error('No file loaded');
        }
        this.p.setLoop(loop);
    }
    getTime() {
        if (this.state === 0 || this.p === null) {
            throw new Error('No file loaded');
        }
        return this.p.getTime();
    }
}
exports.Player = Player;
//# sourceMappingURL=index.js.map