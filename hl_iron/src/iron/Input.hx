package iron;

@:hlNative("iron")
class Input {
    public static function keyboardDown(key:hl.Bytes):Bool {
        return false;
    }

    public static function keyboardStarted(key:hl.Bytes):Bool {
        return false;
    }

    public static function keyboardReleased(key:hl.Bytes):Bool {
        return false;
    }

    public static function mouseDown(button:hl.Bytes):Bool {
        return false;
    }

    public static function mouseStarted(button:hl.Bytes):Bool {
        return false;
    }

    public static function getMouseX():Single {
        return 0.0;
    }

    public static function getMouseY():Single {
        return 0.0;
    }
}