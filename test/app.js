#!/usr/bin/env node

const path = require("path");
const app_path = path.resolve("..", "build", "src", "rtt", "strtt");

const spawn = require("child_process").spawn;

let test = 0;
let testCnt = 0;

console.log("current path:", __dirname);
console.log("application path:", app_path);
console.log("----------------");

const child = spawn(app_path, []);

child.stdout.on("data", (chunk) => {
    console.log("RTT:", chunk.toString());
    switch (test) {
        case 1:
            if (chunk.toString() === "echo") {
                console.log("PASS", test);
                test2();
            }
            break;
        case 2:
            if (chunk.toString() === "1234567890abcde") {
                console.log("PASS", test);
                test3();
            }
            break;
        case 3:
            if (chunk[0] === testCnt) {
                console.log("PASS", test, testCnt++);
                if (testCnt == 255) {
                    test3();
                } else {
                    test4();
                }

            }
            break;

        default:
            break;
    }
});

child.stderr.on("data", (data) => {
    console.error(`stderr: ${data}`);
});

child.on("close", (code) => {
    console.log(`child process exited with code: ${code}`);
});

child.on("error", (code) => {
    console.log(`child process error: ${code}`);
});

setTimeout(() => {
    test1();
}, 500);


function test1() {
    test = 1;
    child.stdin.write("echo");
}

function test2() {
    test = 2;
    child.stdin.write("1234567890abcde");
}

function test3() {
    test = 3;
    const buf = Buffer.from([testCnt]);
    child.stdin.write(buf);
}
function test4() {
    console.log("ALL PASS");
}