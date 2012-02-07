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

import flash.geom.Point;
import flash.geom.Rectangle;

import flash.net.Socket;

import flash.text.TextField;
import flash.text.TextFieldAutoSize;
import flash.text.TextFormat;

import flash.ui.ContextMenu;
import flash.ui.ContextMenuItem;
import flash.ui.Keyboard;

import flash.utils.ByteArray;

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
        for (ii = 0; ii < _height; ii++) {
            text += line;
        }
        _field.text = text;
        _field.setTextFormat(_unlitFormat);
        _field.x = (loaderInfo.width - _field.width) / 2;
        _field.y = (loaderInfo.height - _field.height) / 2;

        // create and initialize the combined contents array
        _contents = new Array(_width * _height);
        for (ii = 0; ii < _contents.length; ii++) {
            _contents[ii] = 0x20;
        }

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
            _socket.writeUnsignedInt(0x57544750); // "WTGP"
            _socket.writeUnsignedInt(0x00000001);
            writeHexString(getCookie("sessionId", "0000000000000000"));
            writeHexString(getCookie("sessionToken", "00000000000000000000000000000000"));
            _socket.writeShort(_width);
            _socket.writeShort(_height);
        });
        _socket.addEventListener(IOErrorEvent.IO_ERROR, function (event :IOErrorEvent) :void {
            fatalError("Couldn't connect to server.  Please wait a while, then reload the page.");
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
        _socket.writeShort(event.type == KeyboardEvent.KEY_DOWN ? KEY_PRESSED : KEY_RELEASED);
        _socket.writeUnsignedInt(getQtKeyCode(event));
        _socket.flush();
    }

    /**
     * Handles incoming socket data.
     */
    protected function readMessages (event :ProgressEvent) :void
    {
        while (true) {
            if (_messageLength == 0) {
                if (_socket.bytesAvailable < 2) {
                    return;
                }
                _messageLength = _socket.readUnsignedShort();

            } else {
                if (_socket.bytesAvailable < _messageLength) {
                    return;
                }
                var bytes :ByteArray = new ByteArray();
                _socket.readBytes(bytes, 0, _messageLength);
                _messageLength = 0;

                bytes.position = 0;
                decodeMessage(bytes);
            }
        }
        updateDisplay();
    }

    /**
     * Decodes the message contained in the provided byte array.
     */
    protected function decodeMessage (bytes :ByteArray) :void
    {
        var type :int = bytes.readUnsignedShort();
        switch (type) {
            case ADD_WINDOW:
                addWindow(new Window(bytes.readInt(), bytes.readInt(),
                    readRectangle(bytes), bytes.readInt()));
                break;

            case REMOVE_WINDOW:
                removeWindow(bytes.readInt());
                break;

            case UPDATE_WINDOW:
                updateWindow(bytes.readInt(), bytes.readInt(),
                    readRectangle(bytes), bytes.readInt());
                break;

            case SET_CONTENTS:
                var id :int = bytes.readInt();
                var bounds :Rectangle = readRectangle(bytes);
                var contents :Array = new Array(bounds.width * bounds.height);
                for (var ii :int = 0; ii < contents.length; ii++) {
                    contents[ii] = bytes.readInt();
                }
                setWindowContents(id, bounds, contents);
                break;

            case MOVE_CONTENTS:
                moveWindowContents(bytes.readInt(), readRectangle(bytes),
                    new Point(bytes.readInt(), bytes.readInt()), bytes.readInt());
                break;

            case SET_SESSION:
                setCookie("sessionId", readHexString(bytes, 8));
                setCookie("sessionToken", readHexString(bytes, 16));
                break;

            case COMPOUND:
                for (var count :int = bytes.readInt(); count >= 0; count--) {
                    decodeMessage(bytes);
                }
                break;

            default:
                trace("Unknown message type.", type);
        }
    }

    /**
     * Reads and returns a rectangle from the provided array.
     */
    protected function readRectangle (bytes :ByteArray) :Rectangle
    {
        return new Rectangle(bytes.readInt(), bytes.readInt(), bytes.readInt(), bytes.readInt());
    }

    /**
     * Reads the specified number of bytes from the provided array and returns them as a hex
     * string.
     */
    protected function readHexString (bytes :ByteArray, length :int) :String
    {
        var result :String = "";
        for (var ii :int = 0; ii < length; ii++) {
            // make sure each byte becomes two characters
            var value :uint = bytes.readUnsignedByte();
            result += (value / 16).toString(16) + (value % 16).toString(16);
        }
        return result;
    }

    /**
     * Writes the specified hex string to the socket as binary data.
     */
    protected function writeHexString (string :String) :void
    {
        for (var ii :int = 0; ii < string.length; ii += 2) {
            _socket.writeByte(parseInt(string.substr(ii, 2), 16));
        }
    }

    /**
     * Creates a new centered window with the supplied text, indicating a fatal error
     * (unable to connect to or disconnected from the server).
     */
    protected function fatalError (text :String) :void
    {
        text = " " + text + " ";
        var bounds :Rectangle = new Rectangle(
            int((_width - text.length - 2) / 2), int(_height/2), text.length + 2, 3);
        var window :Window = new Window(int.MAX_VALUE, int.MAX_VALUE, bounds, "#".charCodeAt(0));
        for (var ii :int = 0; ii < text.length; ii++) {
            window.contents[text.length + 3 + ii] = int(text.charCodeAt(ii));
        }
        addWindow(window);
        updateDisplay();
    }

    /**
     * Adds a window to the list.
     */
    protected function addWindow (window :Window) :void
    {
        addDirtyRegion(window.bounds);
        for (var ii :int = _windows.length - 1; ii >= 0; ii--) {
            if (window.layer >= _windows[ii].layer) {
                _windows.splice(ii + 1, 0, window);
                return;
            }
        }
        _windows.splice(0, 0, window);
    }

    /**
     * Removes the window with the specified id from the list.
     */
    protected function removeWindow (id :int) :void
    {
        var idx :int = getWindowIndex(id);
        if (idx != -1) {
            addDirtyRegion(_windows[idx].bounds);
            _windows.splice(idx, 1);
        }
    }

    /**
     * Updates the identified window.
     */
    protected function updateWindow (id :int, layer :int, bounds :Rectangle, fill :int) :void
    {
        var idx :int = getWindowIndex(id);
        if (idx == -1) {
            return;
        }
        var window :Window = _windows[idx];
        if (window.layer != layer) {
            // remove and reinsert
            _windows.splice(idx, 1);
            window.layer = layer;
            addWindow(window);
        }
        addDirtyRegion(window.bounds);
        if (!window.bounds.equals(bounds)) {
            addDirtyRegion(bounds);
            if (!window.bounds.size.equals(bounds.size)) {
                window.resizeContents(bounds.size, fill);
            }
            window.bounds.copyFrom(bounds);
        }
    }

    /**
     * Sets part of a window's contents.
     */
    protected function setWindowContents (id :int, bounds :Rectangle, contents :Array) :void
    {
        var window :Window = getWindow(id);
        if (window != null) {
            addDirtyRegion(new Rectangle(window.bounds.x + bounds.x, window.bounds.y + bounds.y,
                bounds.width, bounds.height));
            window.setContents(bounds, contents);
        }
    }

    /**
     * Moves part of a window's contents.
     */
    protected function moveWindowContents (
        id :int, source :Rectangle, dest :Point, fill :int) :void
    {
        var window :Window = getWindow(id);
        if (window != null) {
            addDirtyRegion(new Rectangle(window.bounds.x + source.x, window.bounds.y + source.y,
                source.width, source.height));
            addDirtyRegion(new Rectangle(window.bounds.x + dest.x, window.bounds.y + dest.y,
                source.width, source.height));
            window.moveContents(source, dest, fill);
        }
    }

    /**
     * Returns a reference to the window with the specified id, or null if not found.
     */
    protected function getWindow (id :int) :Window
    {
        var idx :int = getWindowIndex(id);
        return (idx == -1) ? null : _windows[idx];
    }

    /**
     * Returns the index of the window with the specified id, or -1 if not found.
     */
    protected function getWindowIndex (id :int) :int
    {
        for (var ii :int = 0; ii < _windows.length; ii++) {
            if (_windows[ii].id == id) {
                return ii;
            }
        }
        trace("Window not found.", id);
        return -1;
    }

    /**
     * Adds a region to the rectangle that will need updating.
     */
    protected function addDirtyRegion (region :Rectangle) :void
    {
        _dirty = _dirty.union(region);
    }

    /**
     * Updates the portion of the display covered by the dirty region.
     */
    protected function updateDisplay () :void
    {
        // get the intersection of the screen bounds and the dirty region
        var bounds :Rectangle = _dirty.intersection(new Rectangle(0, 0, _width, _height));
        _dirty.setEmpty();
        if (bounds.isEmpty()) {
            return;
        }

        // first, combine the contents of all intersecting windows
        var combined :Array = new Array(bounds.width * bounds.height);
        for (var ii :int = 0; ii < _windows.length; ii++) {
            var window :Window = _windows[ii];
            var isect :Rectangle = window.bounds.intersection(bounds);
            trace(isect);
            if (isect.isEmpty()) {
                continue;
            }
            for (var yy :int = isect.top; yy < isect.bottom; yy++) {
                for (var xx :int = isect.left; xx < isect.right; xx++) {
                    var value :int = window.contents[
                        (yy - window.bounds.y)*window.bounds.width + (xx - window.bounds.x)];
                    if (value != 0x0) { // zero = "transparent"
                        combined[(yy - bounds.y)*bounds.width + (xx - bounds.x)] = value;
                    }
                }
            }
        }

        // now update the field/highlights with the new contents
        for (yy = bounds.top; yy < bounds.bottom; yy++) {
            for (xx = bounds.left; xx < bounds.right; xx++) {
                var obj :Object = combined[(yy - bounds.y) * bounds.width + (xx - bounds.x)];
                var nvalue :int = (obj == null) ? 0x20 : int(obj);
                var idx :int = yy*_width + xx;
                var ovalue :int = _contents[idx];
                if (ovalue == nvalue) {
                    continue;
                }
                _contents[idx] = nvalue;

                var tidx :int = yy*(_width + 1) + xx;
                var nchar :int = nvalue & 0xFFFF;
                if ((ovalue & 0xFFFF) != nchar) {
                    _field.replaceText(tidx, tidx + 1, String.fromCharCode(nchar));
                }
                var nlit :int = (nvalue & HIGHLIGHT_FLAG);
                if ((ovalue & HIGHLIGHT_FLAG) != nlit) {
                    if (nlit == HIGHLIGHT_FLAG) {
                        // the char boundaries might not be valid yet, so we compute the location
                        // assuming the text is centered within the field
                        var highlight :Bitmap = new Bitmap(_highlightData);
                        addChildAt(_highlights[idx] = highlight, 0);
                        var cbounds :Rectangle = _field.getCharBoundaries(0);
                        highlight.x = _field.x + cbounds.width*xx +
                            (_field.width - cbounds.width*_width)/2;
                        highlight.y = _field.y + cbounds.height*yy +
                            (_field.height - cbounds.height*_height)/2;
                       _field.setTextFormat(_litFormat, tidx, tidx + 1);
                    } else {
                        removeChild(_highlights[idx]);
                        _highlights[idx] = null;
                        _field.setTextFormat(_unlitFormat, tidx, tidx + 1);
                    }
                }
            }
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

    /** The contents of our field as integers. */
    protected var _contents :Array;

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

    /** The length of the current server message. */
    protected var _messageLength :int = 0;

    /** The list of windows (sorted by layer). */
    protected var _windows :Array = [ ];

    /** The current dirty region. */
    protected var _dirty :Rectangle = new Rectangle();

    /** Flag indicating that the character should be highlighted. */
    protected static var HIGHLIGHT_FLAG :int = 0x10000;

    /** Outgoing message: key pressed. */
    protected static var KEY_PRESSED :int = 1;

    /** Outgoing message: key released. */
    protected static var KEY_RELEASED :int = 2;

    /** Incoming message: add window. */
    protected static var ADD_WINDOW :int = 1;

    /** Incoming message: remove window. */
    protected static var REMOVE_WINDOW :int = 2;

    /** Incoming message: update window. */
    protected static var UPDATE_WINDOW :int = 3;

    /** Incoming message: set contents. */
    protected static var SET_CONTENTS :int = 4;

    /** Incoming message: move contents. */
    protected static var MOVE_CONTENTS :int = 5;

    /** Incoming message: set session id/token. */
    protected static var SET_SESSION :int = 6;

    /** Incoming message: compound. */
    protected static var COMPOUND :int = 7;
}
}

import flash.geom.Point;
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
    public function Window (id :int, layer :int, bounds :Rectangle, fill :int = 0x20)
    {
        this.id = id;
        this.layer = layer;
        this.bounds = bounds.clone();

        var size :int = bounds.width * bounds.height;
        contents = new Array(size);
        for (var ii :int = 0; ii < size; ii++) {
            contents[ii] = fill;
        }
    }

    /**
     * Resizes the contents to the specified dimensions.
     */
    public function resizeContents (size :Point, fill :int = 0x20) :void
    {
        var osize :Point = bounds.size;
        var ocontents :Array = contents;
        contents = new Array(size.x * size.y);
        for (var yy :int = 0; yy < size.y; yy++) {
            for (var xx :int = 0; xx < size.x; xx++) {
                contents[yy*size.x + xx] = (xx < osize.x && yy < osize.y) ?
                    ocontents[yy*osize.x + xx] : fill;
            }
        }
    }

    /**
     * Sets part of the window's contents.
     */
    public function setContents (rect :Rectangle, values :Array) :void
    {
        for (var yy :int = 0; yy < rect.height; yy++) {
            for (var xx :int = 0; xx < rect.width; xx++) {
                contents[(rect.y + yy)*bounds.width + (rect.x + xx)] = values[yy*rect.width + xx];
            }
        }
    }

    /**
     * Moves part of a window's contents.
     */
    public function moveContents (source :Rectangle, dest :Point, fill :int = 0x20) :void
    {
        // copy the source to a temporary buffer and clear it
        var values :Array = new Array(source.width * source.height);
        for (var yy :int = 0; yy < source.height; yy++) {
            for (var xx :int = 0; xx < source.width; xx++) {
                var cidx :int = (source.y + yy)*bounds.width + (source.x + xx);
                values[yy*source.width + xx] = contents[cidx];
                contents[cidx] = fill;
            }
        }

        // then set it in the main one
        setContents(new Rectangle(dest.x, dest.y, source.width, source.height), values);
    }
}
