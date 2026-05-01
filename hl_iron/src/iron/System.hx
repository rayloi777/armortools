package iron;

@:hlNative("iron")
class System {
    public static function getWidth():Int {
        return 0;
    }

    public static function getHeight():Int {
        return 0;
    }

    public static function getTime():Float {
        return 0.0;
    }

    public static function setRenderCallback(cb:Void->Void):Void {}

    public static function setUpdateCallback(cb:Void->Void):Void {}

    public static function exit():Void {}
}