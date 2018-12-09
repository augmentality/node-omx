const omx = require('./index.js');
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
