"use strict";
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