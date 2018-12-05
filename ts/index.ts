import * as cmake from 'node-cmake'
import {Subject} from 'rxjs/internal/Subject';
import {PlaybackState} from "./PlaybackState";
const omx = cmake('node_omx');

export class Player
{
    private p: any;
    private url: string;
    private state: number;

    onPlaybackState: Subject<PlaybackState> = new Subject<PlaybackState>();

    constructor()
    {
        this.state = 0;
        this.url = '';
        this.p = new omx.Player();
    }

    open(url: string): Promise<void>
    {
        this.url = url;
        if (this.state !== 0)
        {
            throw new Error('URL already open. Stop first or create a new instance.');
        }
        return new Promise<void>((resolve, reject) =>
        {
            try
            {
                this.p.loadURL(url, () =>
                {
                    this.state = 1;
                    resolve();
                }, (state: PlaybackState) =>
                {
                    this.onPlaybackState.next(state);
                });
            }
            catch (err)
            {

            }
        });

    }

    play()
    {
        if (this.state === 0  || this.p === null)
        {
            throw new Error('No file loaded');
        }
        if (this.state === 2)
        {
            throw new Error('Already playing');
        }

        if (this.p === null)
        {
            this.p = new omx.Player(this.url);
        }
        this.p.play();
        this.state = 2;
    }

    stop()
    {
        if (this.state === 0  || this.p === null)
        {
            throw new Error('No file loaded');
        }
        this.p.stop();
        this.p = null;
        this.state = 0;
    }

    pause()
    {
        if (this.state === 0  || this.p === null)
        {
            throw new Error('No file loaded');
        }
        if (this.state !== 2)
        {
            throw new Error('File not playing');
        }
        this.p.pause();
        this.state = 3;
    }

    setSpeed(factor: number)
    {
        if (this.state === 0  || this.p === null)
        {
            throw new Error('No file loaded');
        }
        this.p.setSpeed(factor);
    }

    setLoop(loop: boolean)
    {
        if (this.state === 0  || this.p === null)
        {
            throw new Error('No file loaded');
        }
        this.p.setLoop(loop);
    }

    getTime(): number
    {
        if (this.state === 0 || this.p === null)
        {
            throw new Error('No file loaded');
        }
        return this.p.getTime();
    }
}