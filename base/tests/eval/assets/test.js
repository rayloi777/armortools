// Eval Test - QuickJS Script
// Implements init(), update(dt), render(dt) lifecycle functions

let pos_x = 0;
let pos_y = 0;
let time = 0;
let initialized = false;

function init() {
    pos_x = 320;
    pos_y = 240;
    time = 0;
    initialized = true;
}

function update(dt) {
    time = time + dt;
    pos_x = 320 + Math.cos(time * 2.0) * 150;
    pos_y = 240 + Math.sin(time * 3.0) * 100;
}

function render(dt) {
    return {
        x: pos_x,
        y: pos_y,
        size: 30 + Math.sin(time * 5.0) * 10,
        color: 0xffe94560
    };
}