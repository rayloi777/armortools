package iron;

@:hlNative("iron")
class Input {
    public static function keyboardDown(key:String):Bool {
        return false;
    }

    public static function keyboardStarted(key:String):Bool {
        return false;
    }

    public static function keyboardReleased(key:String):Bool {
        return false;
    }

    public static function mouseDown(button:String):Bool {
        return false;
    }

    public static function mouseStarted(button:String):Bool {
        return false;
    }

    public static function getMouseX():Single {
        return 0.0;
    }

    public static function getMouseY():Single {
        return 0.0;
    }
}