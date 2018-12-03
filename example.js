const omx = require('./index.js');
const readline = require('readline');

readline.emitKeypressEvents(process.stdin);

const n = new omx.Player();
n.loadURL('http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_30fps_normal.mp4');
n.play();
let speed = 1.0;
setInterval(() =>
{
    console.log('Playback position: ' + n.getTime());
}, 1000);
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
