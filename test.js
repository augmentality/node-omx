const omx = require('./index');

let run = 0;
setInterval(() => {
    run++;
    console.log("RUN " + String(run));
    const p = new omx.Player();
    p.open('/augmentality/storage/briefing.mp4').then(() => {

        console.log('File Loaded. State: ' + p.state);
        p.play();
        setTimeout(() => {
            p.stop();
            subs.unsubscribe();
        }, 5000);

    });
}, 10000);
