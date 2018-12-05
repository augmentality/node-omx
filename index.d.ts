export declare class Player {
    private p;
    private url;
    private state;
    constructor();
    open(url: string): void;
    play(): void;
    stop(): void;
    pause(): void;
    setSpeed(factor: number): void;
    setLoop(loop: boolean): void;
    getTime(): number;
}
