//
// $Id$

package {

import flash.display.Bitmap;
import flash.display.BitmapData;
import flash.display.Sprite;

import flash.events.ContextMenuEvent;
import flash.events.Event;
import flash.events.IOErrorEvent;
import flash.events.KeyboardEvent;
import flash.events.ProgressEvent;
import flash.events.SecurityErrorEvent;

import flash.external.ExternalInterface;

import flash.geom.Rectangle;

import flash.net.Socket;

import flash.text.TextField;
import flash.text.TextFieldAutoSize;
import flash.text.TextFormat;

import flash.ui.ContextMenu;
import flash.ui.ContextMenuItem;
import flash.ui.Keyboard;

/**
 * The Witgap client application.
 */
[SWF(frameRate="60", width=800, height=600, backgroundColor="#000000")]
public class ClientApp extends Sprite {

    /**
     * Creates a new client app.
     */
    public function ClientApp ()
    {
        // initialize when we've got our dimensions
        loaderInfo.addEventListener(Event.INIT, init);
    }

    /**
     * Initializes the app.
     */
    protected function init (event :Event) :void
    {
        // create and add the text field display
        _field = new TextField();
        addChild(_field);
        _field.autoSize = TextFieldAutoSize.CENTER;
        _field.textColor = 0x00FF00;
        _field.text = " ";
        _field.selectable = false;
        _unlitFormat = new TextFormat();
        _litFormat = new TextFormat();
        _litFormat.color = 0x000000;
        _unlitFormat.font = _litFormat.font = "_typewriter";
        _field.setTextFormat(_unlitFormat);

        // get the size of a character and use it to determine the char width/height
        var bounds :Rectangle = _field.getCharBoundaries(0);
        _width = loaderInfo.width / bounds.width;
        _height = loaderInfo.height / bounds.height;
        var line :String = "", text :String = "";
        for (var ii :int = 0; ii < _width; ii++) {
            line += " ";
        }
        line += '\n';
        for (var jj :int = 0; jj < _height; jj++) {
            text += line;
        }
        _field.text = text;
        _field.setTextFormat(_unlitFormat);
        _field.x = (loaderInfo.width - _field.width) / 2;
        _field.y = (loaderInfo.height - _field.height) / 2;

        // create the highlight bitmap data and highlight array
        _highlightData = new BitmapData(bounds.width, bounds.height, false);
        _highlights = new Array(_width * _height);

        // add the context menu to change colors
        contextMenu = new ContextMenu();
        contextMenu.hideBuiltInItems();
        contextMenu.customItems = [ ];
        var captions :Array = [ "White", "Green", "Amber" ];
        var colors :Array = [ 0xFFFFFF, 0x00FF00, 0xFFAF00 ];
        var setColor :Function = function (caption :String) :void {
            setCookie("color", caption);
            for (var kk :int = 0; kk < captions.length; kk++) {
                if (captions[kk] == caption) {
                    _field.textColor = colors[kk];
                    _highlightData.fillRect(_highlightData.rect, colors[kk]);
                    contextMenu.customItems[kk].enabled = false;
                } else {
                    contextMenu.customItems[kk].enabled = true;
                }
            }
        };
        var updateColor :Function = function (event :ContextMenuEvent) :void {
            setColor(ContextMenuItem(event.target).caption);
        };
        for (var kk :int = 0; kk < captions.length; kk++) {
            var item :ContextMenuItem = new ContextMenuItem(captions[kk]);
            contextMenu.customItems.push(item);
            item.addEventListener(ContextMenuEvent.MENU_ITEM_SELECT, updateColor);
        }
        setColor(getCookie("color", "Green"));

        // listen for key events
        addEventListener(KeyboardEvent.KEY_DOWN, sendKeyMessage);
        addEventListener(KeyboardEvent.KEY_UP, sendKeyMessage);

        // create the socket
        _socket = new Socket();
        _socket.addEventListener(Event.CONNECT, function (event :Event) :void {
            // write the magic number, version, screen dimensions
            _socket.writeUnsignedInt(0x57544750);
            _socket.writeUnsignedInt(0x00000001);
            _socket.writeShort(_width);
            _socket.writeShort(_height);
        });
        _socket.addEventListener(IOErrorEvent.IO_ERROR, function (event :IOErrorEvent) :void {
            // TODO: display error

            var str :String = "Connect error";
            drawString(str, (_width - str.length) / 2, _height/2);
        });
        _socket.addEventListener(SecurityErrorEvent.SECURITY_ERROR,
                function (event :SecurityErrorEvent) :void {
            // we get this when we can't connect; ignore it
        });
        _socket.addEventListener(ProgressEvent.SOCKET_DATA, readMessages);

        // attempt to connect to the server
        var params :Object = loaderInfo.parameters;
        _socket.connect(String(params["server_host"]), int(params["server_port"]));
    }

    /**
     * Sets a cookie value in the containing context.
     */
    protected function setCookie (name :String, value :String) :void
    {
        ExternalInterface.call("setCookie", name + "=" + escape(value));
    }

    /**
     * Retrieves a cookie value from the containing context.
     */
    protected function getCookie (name :String, def :String) :String
    {
        var values :Array = ExternalInterface.call("getCookie").split(";");
        for (var ii :int = 0; ii < values.length; ii++) {
            var value :String = values[ii];
            var idx :int = value.indexOf("=");
            if (value.substring(0, idx).replace(/\s+/,"") == name) {
                return unescape(value.substring(idx + 1));
            }
        }
        return def;
    }

    /**
     * Handles key events.
     */
    protected function sendKeyMessage (event :KeyboardEvent) :void
    {
        if (!_socket.connected) {
            return;
        }
        _socket.writeShort(6);
        _socket.writeShort(event.type == KeyboardEvent.KEY_DOWN ? 0 : 1);
        _socket.writeUnsignedInt(getQtKeyCode(event));
    }

    /**
     * Handles incoming socket data.
     */
    protected function readMessages (event :ProgressEvent) :void
    {
    }

    /**
     * Draws a string at the specified x and y positions.
     */
    protected function drawString (str :String, x :int, y :int) :void
    {
        var begin :int = y*(_width + 1) + x;
        _field.replaceText(begin, begin + str.length, str);

        for (var ii :int = 0; ii < str.length; ii++) {
            setHighlight(x + ii, y, true);
        }
    }

    /**
     * Sets or clears the highlight at the specified location.
     */
    protected function setHighlight (x :int, y :int, lit :Boolean) :void
    {
        var idx :int = y*_width + x;
        var highlight :Bitmap = _highlights[idx];
        if ((highlight != null) == lit) {
            return;
        }
        var tidx :int = y*(_width + 1) + x;
        if (lit) {
            // the char boundaries might not be valid yet, so we compute the location assuming the
            // text is centered within the field
            addChildAt(_highlights[idx] = highlight = new Bitmap(_highlightData), 0);
            var bounds :Rectangle = _field.getCharBoundaries(0);
            highlight.x = _field.x + bounds.width*x + (_field.width - bounds.width*_width)/2;
            highlight.y = _field.y + bounds.height*y + (_field.height - bounds.height*_height)/2;
            _field.setTextFormat(_litFormat, tidx, tidx + 1);
        } else {
            removeChild(_highlights[idx]);
            _highlights[idx] = null;
            _field.setTextFormat(_unlitFormat, tidx, tidx + 1);
        }
    }

    /**
     * Updates the portion of the text field covered by the given bounds.
     */
    protected function updateField (bounds :Rectangle) :void
    {
        for (var ii :int = 0; ii < _windows.length; ii++) {
            var window :Window = _windows[ii];
            var isect :Rectangle = window.bounds.intersection(bounds);

        }
    }

    /**
     * Converts an ActionScript key code to a Qt one.
     */
    protected static function getQtKeyCode (event :KeyboardEvent) :uint
    {
        switch (event.keyCode) {
            case Keyboard.ESCAPE: return 0x01000000;
            case Keyboard.F1: return 0x01000030;
            case Keyboard.F2: return 0x01000031;
            case Keyboard.F3: return 0x01000032;
            case Keyboard.F4: return 0x01000033;
            case Keyboard.F5: return 0x01000034;
            case Keyboard.F6: return 0x01000035;
            case Keyboard.F7: return 0x01000036;
            case Keyboard.F8: return 0x01000037;
            case Keyboard.F9: return 0x01000038;
            case Keyboard.F10: return 0x01000039;
            case Keyboard.F11: return 0x0100003a;
            case Keyboard.F12: return 0x0100003b;
            case Keyboard.F13: return 0x0100003c;
            case Keyboard.F14: return 0x0100003d;
            case Keyboard.F15: return 0x0100003e;
            case Keyboard.PAUSE: return 0x01000008;

            case Keyboard.BACKQUOTE: return 0x60;
            case Keyboard.NUMBER_0: return 0x30;
            case Keyboard.NUMBER_1: return 0x31;
            case Keyboard.NUMBER_2: return 0x32;
            case Keyboard.NUMBER_3: return 0x33;
            case Keyboard.NUMBER_4: return 0x34;
            case Keyboard.NUMBER_5: return 0x35;
            case Keyboard.NUMBER_6: return 0x36;
            case Keyboard.NUMBER_7: return 0x37;
            case Keyboard.NUMBER_8: return 0x38;
            case Keyboard.NUMBER_9: return 0x39;
            case Keyboard.MINUS: return 0x2d;
            case Keyboard.EQUAL: return 0x3d;
            case Keyboard.BACKSPACE: return 0x01000003;
            case Keyboard.INSERT: return 0x01000006;
            case Keyboard.HOME: return 0x01000010;
            case Keyboard.PAGE_UP: return 0x01000016;
            case Keyboard.NUMPAD_DIVIDE: return 0x2f;
            case Keyboard.NUMPAD_MULTIPLY: return 0x2a;
            case Keyboard.NUMPAD_SUBTRACT: return 0x2d;

            case Keyboard.TAB: return 0x01000001;
            case Keyboard.Q: return 0x51;
            case Keyboard.W: return 0x57;
            case Keyboard.E: return 0x45;
            case Keyboard.R: return 0x52;
            case Keyboard.T: return 0x54;
            case Keyboard.Y: return 0x59;
            case Keyboard.U: return 0x55;
            case Keyboard.I: return 0x49;
            case Keyboard.O: return 0x4f;
            case Keyboard.P: return 0x50;
            case Keyboard.LEFTBRACKET: return 0x5b;
            case Keyboard.RIGHTBRACKET: return 0x5d;
            case Keyboard.BACKSLASH: return 0x5c;
            case Keyboard.DELETE: return 0x01000007;
            case Keyboard.END: return 0x01000011;
            case Keyboard.PAGE_DOWN: return 0x01000017;
            case Keyboard.NUMPAD_7: return 0x37;
            case Keyboard.NUMPAD_8: return 0x38;
            case Keyboard.NUMPAD_9: return 0x39;
            case Keyboard.NUMPAD_ADD: return 0x2b;

            case Keyboard.CAPS_LOCK: return 0x01000024;
            case Keyboard.A: return 0x41;
            case Keyboard.S: return 0x53;
            case Keyboard.D: return 0x44;
            case Keyboard.F: return 0x46;
            case Keyboard.G: return 0x47;
            case Keyboard.H: return 0x48;
            case Keyboard.J: return 0x4a;
            case Keyboard.K: return 0x4b;
            case Keyboard.L: return 0x4c;
            case Keyboard.SEMICOLON: return 0x3b;
            case Keyboard.QUOTE: return 0x27;
            case Keyboard.ENTER: return 0x01000004;
            case Keyboard.NUMPAD_4: return 0x34;
            case Keyboard.NUMPAD_5: return 0x35;
            case Keyboard.NUMPAD_6: return 0x36;

            case Keyboard.SHIFT: return 0x01000020;
            case Keyboard.Z: return 0x5a;
            case Keyboard.X: return 0x58;
            case Keyboard.C: return 0x43;
            case Keyboard.V: return 0x56;
            case Keyboard.B: return 0x42;
            case Keyboard.N: return 0x4e;
            case Keyboard.M: return 0x4d;
            case Keyboard.COMMA: return 0x2c;
            case Keyboard.PERIOD: return 0x2e;
            case Keyboard.SLASH: return 0x2f;
            case Keyboard.UP: return 0x01000013;
            case Keyboard.NUMPAD_1: return 0x31;
            case Keyboard.NUMPAD_2: return 0x32;
            case Keyboard.NUMPAD_3: return 0x33;
            case Keyboard.NUMPAD_ENTER: return 0x01000005;

            case Keyboard.CONTROL: return 0x01000021;
            case Keyboard.COMMAND: return 0x01000021;
            case Keyboard.ALTERNATE: return 0x01000023;
            case Keyboard.SPACE: return 0x20;
            case Keyboard.MENU: return 0x01000055;
            case Keyboard.LEFT: return 0x01000012;
            case Keyboard.DOWN: return 0x01000015;
            case Keyboard.RIGHT: return 0x01000014;

            case Keyboard.HELP: return 0x01000058;

            default: return 0x01ffffff; // Key_unknown
        }
    }

    /** Our gigantic text field. */
    protected var _field :TextField;

    /** Formats for unlit and lit text. */
    protected var _unlitFormat :TextFormat, _litFormat :TextFormat;

    /** The highlight bitmap data. */
    protected var _highlightData :BitmapData;

    /** The highlight bitmaps for each location (if active). */
    protected var _highlights :Array;

    /** The width and height of the display in characters. */
    protected var _width :int, _height :int;

    /** The socket via which we communicate with the server. */
    protected var _socket :Socket;

    /** The list of windows (sorted by layer). */
    protected var _windows :Array = [ ];
}
}

import flash.geom.Rectangle;

/**
 * A managed window.
 */
class Window
{
    /** The window identifier. */
    public var id :int;

    /** The window layer.  Higher layers render on top of lower ones. */
    public var layer :int;

    /** The window bounds. */
    public var bounds :Rectangle;

    /** The window contents. */
    public var contents :Array;

    /**
     * Creates a new, empty window.
     */
    public function Window (id :int, layer :int, bounds :Rectangle)
    {
        this.id = id;
        this.layer = layer;
        this.bounds = bounds.clone();

        var size :int = bounds.width * bounds.height;
        contents = new Array(size);
        for (var ii :int = 0; ii < size; ii++) {
            contents[ii] = 0x20;
        }
    }
}
