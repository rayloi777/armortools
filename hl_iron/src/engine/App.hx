package engine;

import iron.System;
import iron.Gpu;
import iron.Input;

class App {
    static function main():Void {
        System.setRenderCallback(draw);
        System.setUpdateCallback(update);
    }

    static function update():Void {
        if (Input.keyboardDown("escape")) {
            System.exit();
        }
    }

    static function draw():Void {
        Gpu.beginDefault(0xff6495ed, 1.0);
        Gpu.end();
    }
}
