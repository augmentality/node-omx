"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var cmake = require("node-cmake");
var omx = cmake('node_omx');
var Player = /** @class */ (function () {
    function Player() {
        this.state = 0;
        this.url = '';
        this.p = new omx.Player();
    }
    Player.prototype.open = function (url) {
        if (this.state !== 0) {
            throw new Error('URL already open. Stop first or create a new instance.');
        }
        this.url = url;
        this.p.loadURL(url);
        this.state = 1;
    };
    Player.prototype.play = function () {
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
    };
    Player.prototype.stop = function () {
        if (this.state === 0 || this.p === null) {
            throw new Error('No file loaded');
        }
        this.p.stop();
        this.p = null;
        this.state = 0;
    };
    Player.prototype.pause = function () {
        if (this.state === 0 || this.p === null) {
            throw new Error('No file loaded');
        }
        if (this.state !== 2) {
            throw new Error('File not playing');
        }
        this.p.pause();
        this.state = 3;
    };
    Player.prototype.setSpeed = function (factor) {
        if (this.state === 0 || this.p === null) {
            throw new Error('No file loaded');
        }
        this.p.setSpeed(factor);
    };
    Player.prototype.setLoop = function (loop) {
        if (this.state === 0 || this.p === null) {
            throw new Error('No file loaded');
        }
        this.p.setLoop(loop);
    };
    Player.prototype.getTime = function () {
        if (this.state === 0 || this.p === null) {
            throw new Error('No file loaded');
        }
        return this.p.getTime();
    };
    return Player;
}());
exports.Player = Player;
//# sourceMappingURL=index.js.map