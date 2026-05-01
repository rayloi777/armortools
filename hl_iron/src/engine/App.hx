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
        // No-op for now — keyboard input will be added after MVP verifies rendering works
    }

    static function draw():Void {
        Gpu.beginDefault(0xff6495ed, 1.0);
        Gpu.end();
    }
}