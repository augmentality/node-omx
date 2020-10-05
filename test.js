
 if (process.argv.length < 3)
 {
     console.log("Usage: node " + process.argv[1] + " path/to/test_file.mp4");
     process.exit(1);
 }

const omx = require('./index');

let run = 0;
setInterval(() => {
    run++;
    console.log("RUN " + String(run));
    const p = new omx.Player();
    p.open(process.argv[2]).then(() => {

        console.log('File Loaded. State: ' + p.state);
        p.play();
        setTimeout(() => {
            p.stop();

            if (run > 10)
            {
                console.log('Done 10 runs, exiting.');
                process.exit();
            }

        }, 5000);

    });
}, 10000);
