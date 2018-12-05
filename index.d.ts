import { Subject } from 'rxjs/internal/Subject';
import { PlaybackState } from "./PlaybackState";
export declare class Player {
    private p;
    private url;
    private state;
    onPlaybackState: Subject<PlaybackState>;
    constructor();
    open(url: string): Promise<void>;
    play(): void;
    stop(): void;
    pause(): void;
    setSpeed(factor: number): void;
    setLoop(loop: boolean): void;
    getTime(): number;
}
