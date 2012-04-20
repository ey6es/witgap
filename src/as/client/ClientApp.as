//
// $Id$

package {

import flash.display.Bitmap;
import flash.display.BitmapData;
import flash.display.DisplayObject;
import flash.display.Shape;
import flash.display.SimpleButton;
import flash.display.Sprite;

import flash.events.ContextMenuEvent;
import flash.events.Event;
import flash.events.EventDispatcher;
import flash.events.FocusEvent;
import flash.events.IOErrorEvent;
import flash.events.KeyboardEvent;
import flash.events.MouseEvent;
import flash.events.ProgressEvent;
import flash.events.SecurityErrorEvent;
import flash.events.TimerEvent;

import flash.external.ExternalInterface;

import flash.filters.BevelFilter;
import flash.filters.BitmapFilterType;

import flash.geom.Matrix;
import flash.geom.Point;
import flash.geom.Rectangle;

import flash.net.Socket;

import flash.text.TextField;
import flash.text.TextFieldAutoSize;
import flash.text.TextFieldType;
import flash.text.TextFormat;

import flash.ui.ContextMenu;
import flash.ui.ContextMenuItem;
import flash.ui.KeyLocation;
import flash.ui.Keyboard;

import flash.utils.ByteArray;
import flash.utils.IDataInput;
import flash.utils.IDataOutput;
import flash.utils.Timer;

import com.hurlant.crypto.prng.Random;
import com.hurlant.crypto.rsa.RSAKey;
import com.hurlant.crypto.symmetric.AESKey;
import com.hurlant.crypto.symmetric.CBCMode;
import com.hurlant.util.Hex;

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
     * Notifies the application that it's about to be unloaded.
     */
    public function willBeUnloaded () :void
    {
        if (!_socket.connected) {
            return;
        }
        var out :IDataOutput = startMessage();
        out.writeByte(WINDOW_CLOSED_MSG);
        endMessage(out);
        _socket.close();
    }

    /**
     * Initializes the app.
     */
    protected function init (event :Event) :void
    {
        // create the text field used to render characters
        _charField = new TextField();
        _charField.text = " ";
        _normalFormat = new TextFormat();
        _normalFormat.color = 0x00FF00;
        _reverseFormat = new TextFormat();
        _reverseFormat.color = 0x000000;
        _dimFormat = new TextFormat();
        _dimFormat.color = 0x008000;
        _normalFormat.font = _reverseFormat.font = _dimFormat.font = "_typewriter";
        _charField.setTextFormat(_normalFormat);

        // get the size of a character and use it to determine the char width/height
        var bounds :Rectangle = _charField.getCharBoundaries(0);
        _charWidth = bounds.width;
        _charHeight = bounds.height;
        _width = loaderInfo.width / _charWidth;
        _height = loaderInfo.height / _charHeight;

        // create and add the big ol' bitmap with all the characters
        var bitmapData :BitmapData = new BitmapData(
            _charWidth*_width, _charHeight*_height, false, 0x0);
        _bitmap = new Bitmap(bitmapData);
        _bitmap.x = (loaderInfo.width - (_charWidth * _width)) / 2;
        _bitmap.y = (loaderInfo.height - (_charHeight * _height)) / 2;
        addChild(_bitmap);

        // create and initialize the combined contents array
        _contents = new Array(_width * _height);
        for (var ii :int = 0; ii < _contents.length; ii++) {
            _contents[ii] = 0x20;
        }

        // add the context menu to change colors
        contextMenu = new ContextMenu();
        contextMenu.hideBuiltInItems();
        contextMenu.customItems = [ new ContextMenuItem("Change Colors") ];
        contextMenu.customItems[0].addEventListener(ContextMenuEvent.MENU_ITEM_SELECT,
                function (event :ContextMenuEvent) :void {
            showColorPicker();
        });
        setColors(getForeground(), getBackground());

        // add the debug regions toggle if we're in development
        if (DEBUG) {
            contextMenu.customItems[1] = new ContextMenuItem("Toggle Debug Regions");
            contextMenu.customItems[1].addEventListener(ContextMenuEvent.MENU_ITEM_SELECT,
                    function (event :ContextMenuEvent) :void {
                _debugRegions = !_debugRegions;
            });
        }

        // listen for mouse and key events
        var mousePad :Sprite = new Sprite();
        addChild(mousePad);
        mousePad.graphics.beginFill(0x0, 0.0);
        mousePad.graphics.drawRect(0, 0, _bitmap.width, _bitmap.height);
        mousePad.graphics.endFill();
        mousePad.x = _bitmap.x;
        mousePad.y = _bitmap.y;
        mousePad.addEventListener(MouseEvent.MOUSE_DOWN, sendMouseMessage);
        mousePad.addEventListener(MouseEvent.MOUSE_UP, sendMouseMessage);
        addEventListener(KeyboardEvent.KEY_DOWN, sendKeyMessage);
        addEventListener(KeyboardEvent.KEY_UP, sendKeyMessage);

        // request focus and listen for changes
        stage.focus = this;
        addEventListener(FocusEvent.FOCUS_OUT, function (event :FocusEvent) :void {
            addClientWindow(int.MAX_VALUE - 1, "Click to restore input focus.");

            // attempt to reacquire
            if (stage.focus == null) {
                stage.focus = _bitmap.parent;
            }
        });
        addEventListener(FocusEvent.FOCUS_IN, function (event :FocusEvent) :void {
            removeWindow(int.MAX_VALUE - 1, false);
            updateDisplay();
        });

        // allow the JavaScript context to call willBeUnloaded
        ExternalInterface.addCallback("willBeUnloaded", willBeUnloaded);

        // create the socket
        _socket = new Socket();
        _socket.addEventListener(Event.CONNECT, function (event :Event) :void {
            // create our encryption key/iv and encrypt using the public key
            var random :Random = new Random();
            var secret :ByteArray = new ByteArray(), iv :ByteArray = new ByteArray();
            random.nextBytes(secret, 16);
            random.nextBytes(iv, 16);
            _ectx = new CBCMode(new AESKey(secret));
            _ectx.IV = iv;
            _dctx = new CBCMode(new AESKey(secret));
            _dctx.IV = iv;

            var rsaKey :RSAKey = RSAKey.parsePublicKey(
                loaderInfo.parameters["public_key"], "10001");
            var combined :ByteArray = new ByteArray(), encrypted :ByteArray = new ByteArray();
            combined.writeBytes(secret);
            combined.writeBytes(iv);
            rsaKey.encrypt(combined, encrypted, 32);

            // write the preamble
            _socket.writeUnsignedInt(PROTOCOL_MAGIC);
            _socket.writeUnsignedInt(PROTOCOL_VERSION);

            var remainder :ByteArray = new ByteArray();
            remainder.writeShort(_width);
            remainder.writeShort(_height);
            remainder.writeBytes(encrypted);

            encrypted = new ByteArray();
            encrypted.writeUTF(ExternalInterface.call("eval", "location.search"));
            encrypted.writeUTF(ExternalInterface.call("eval", "document.cookie"));
            _ectx.encrypt(encrypted);
            remainder.writeBytes(encrypted);

            _socket.writeUnsignedInt(remainder.length);
            _socket.writeBytes(remainder);
        });
        var errorHandler :Function = function (event :Event) :void {
            fatalError("No connection to server.  Please wait a moment, then reload the page.");
        };
        _socket.addEventListener(Event.CLOSE, errorHandler);
        _socket.addEventListener(IOErrorEvent.IO_ERROR, errorHandler);
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
     * Shows the color picker display.
     */
    protected function showColorPicker () :void
    {
        // no two pickers at once
        contextMenu.customItems[0].enabled = false;

        var picker :Sprite = new Sprite();

        var bevel :BevelFilter = new BevelFilter();
        bevel.distance = 0.5;

        picker.graphics.beginFill(0x808080);
        picker.graphics.drawRect(0, 0, 300, 270);
        picker.graphics.endFill();

        picker.filters = [ bevel ];

        addChild(picker);

        picker.x = (width - picker.width) / 2;
        picker.y = (height - picker.height) / 2;

        var fmt :TextFormat = new TextFormat();
        fmt.font = "_typewriter";

        // we block key event propagation to prevent it from reaching the server
        var blockKeys :Function = function (obj :EventDispatcher) :void {
            obj.addEventListener(KeyboardEvent.KEY_DOWN, function (event :KeyboardEvent) :void {
                event.stopPropagation();
            });
            obj.addEventListener(KeyboardEvent.KEY_UP, function (event :KeyboardEvent) :void {
                event.stopPropagation();
            });
        };

        // create the OK button that closes the window
        var createState :Function = function (color :uint, down :Boolean) :Sprite {
            var state :Sprite = new Sprite();
            var tf :TextField = new TextField();
            tf.autoSize = TextFieldAutoSize.LEFT;
            tf.text = "  OK  ";
            tf.setTextFormat(fmt);
            state.addChild(tf);
            state.graphics.beginFill(color);
            state.graphics.drawRect(0, 0, tf.width, tf.height);
            state.graphics.endFill();
            if (!down) {
                state.filters = [ bevel ];
            }
            return state;
        };
        var over :Sprite = createState(0xC0C0C0, false);
        var ok :SimpleButton = new SimpleButton(createState(0xB0B0B0, false), over,
            createState(0x707070, true), over);
        blockKeys(ok);
        ok.addEventListener(MouseEvent.CLICK, function (event :MouseEvent) :void {
            stage.focus = picker.parent;
            removeChild(picker);
            contextMenu.customItems[0].enabled = true;
        });
        picker.addChild(ok);
        ok.x = (picker.width - ok.width) / 2;
        ok.y = picker.height - ok.height*3/2;

        // create the foreground and background labels
        var createLabel :Function = function (text :String) :TextField {
            var label :TextField = new TextField();
            label.autoSize = TextFieldAutoSize.LEFT;
            label.text = text;
            label.setTextFormat(fmt);
            label.selectable = false;
            return label;
        };
        var fglabel :TextField = createLabel("Foreground:");
        picker.addChild(fglabel);
        var bglabel :TextField = createLabel("Background:");
        picker.addChild(bglabel);

        // create the foreground and background text fields
        var createField :Function = function (text :String) :TextField {
            var field :TextField = new TextField();
            field.width = fglabel.width * 6.75 / fglabel.text.length;
            field.height = fglabel.height;
            field.text = text;
            field.setTextFormat(fmt);
            field.maxChars = 6;
            field.restrict = "A-F0-9";
            field.background = true;
            field.type = TextFieldType.INPUT;
            field.defaultTextFormat = fmt;
            blockKeys(field);

            var bevel :BevelFilter = new BevelFilter();
            bevel.distance = 0.5;
            bevel.angle = 225;
            bevel.type = BitmapFilterType.OUTER;
            field.filters = [ bevel ];

            return field;
        };

        var foreground :TextField = createField(getForeground());
        picker.addChild(foreground);

        var background :TextField = createField(getBackground());
        picker.addChild(background);

        var changer :Function = function (event :Event) :void {
            setColors(foreground.text, background.text);
        };
        foreground.addEventListener(Event.CHANGE, changer);
        background.addEventListener(Event.CHANGE, changer);

        fglabel.x = 10;
        fglabel.y = 10;
        foreground.x = 10 + fglabel.width + 3;
        foreground.y = fglabel.y;

        background.x = picker.width - 10 - background.width;
        background.y = foreground.y;
        bglabel.x = background.x - bglabel.width - 3;
        bglabel.y = fglabel.y;

        stage.focus = foreground;

        // create swatches to select web colors
        var webColors :Array = [
            0x000000, 0x000080, 0x008000, 0x008080,
            0x800000, 0x800080, 0x808000, 0xC0C0C0,
            0x808080, 0x0000FF, 0x00FF00, 0x00FFFF,
            0xFF0000, 0xFF00FF, 0xFFFF00, 0xFFFFFF ];
        var createSwatch :Function = function (color :uint, field :TextField) :Sprite {
            var swatch :Sprite = new Sprite();
            swatch.buttonMode = true;

            var label :TextField = new TextField();
            label.autoSize = TextFieldAutoSize.LEFT;
            label.text = padColor(color.toString(16).toUpperCase());
            label.setTextFormat(fmt);
            label.background = true;
            label.border = true;
            label.selectable = false;
            label.mouseEnabled = false;
            swatch.addChild(label);
            blockKeys(swatch);

            var updater :Function = function (event :Event) :void {
                if (field === foreground) {
                    label.backgroundColor = parseInt(background.text, 16);
                    label.textColor = color;
                } else {
                    label.backgroundColor = color;
                    label.textColor = parseInt(foreground.text, 16);
                }
            };
            updater(null);

            swatch.addEventListener(MouseEvent.CLICK, function (event :MouseEvent) :void {
                field.text = label.text;
                field.dispatchEvent(new Event(Event.CHANGE));
            });
            (field == foreground ? background : foreground).addEventListener(
                Event.CHANGE, updater);

            return swatch;
        };
        var createSwatches :Function = function (ox :int, oy :int, field :TextField) :void {
            for (var ii :int = 0; ii < 8; ii++) {
                for (var jj :int = 0; jj < 2; jj++) {
                    var swatch :Sprite = createSwatch(webColors[ii*2 + jj], field);
                    picker.addChild(swatch);
                    swatch.x = ox +
                        (fglabel.width + foreground.width + 3 - 2*swatch.width - 10)/2 +
                            jj*(swatch.width + 10);
                    swatch.y = oy + ii*(swatch.height + 5);
                }
            }
        };
        createSwatches(fglabel.x, fglabel.y + fglabel.height + 10, foreground);
        createSwatches(bglabel.x, bglabel.y + bglabel.height + 10, background);
    }

    /**
     * Sets the foreground and background colors.
     */
    protected function setColors (foreground :String, background :String) :void
    {
        setCookie("foreground", foreground = padColor(foreground));
        setCookie("background", background = padColor(background));

        var fg :uint = parseInt(foreground, 16);
        var bg :uint = parseInt(background, 16);

        _normalFormat.color = fg;
        _reverseFormat.color = stage.color = bg;

        // the dim color is halfway between foreground and background
        var r :uint = ((fg >> 16) + (bg >> 16)) / 2;
        var g :uint = (((fg >> 8) & 0xFF) + ((bg >> 8) & 0xFF)) / 2;
        var b :uint = ((fg & 0xFF) + (bg & 0xFF)) / 2;
        _dimFormat.color = (r << 16) | (g << 8) | b;

        // dispose of all cached bitmaps and refresh
        for (var key :* in _bitmapData) {
            _bitmapData[uint(key)].dispose();
        }
        _bitmapData = new Object();

        // invalidate the contents, make everything dirty, refresh
        for (var ii :int = 0; ii < _contents.length; ii++) {
            _contents[ii] = -1;
        }
        _dirty.setTo(0, 0, _width, _height);
        updateDisplay();
    }

    /**
     * Pads a color out to the full six digits.
     */
    protected function padColor (str :String) :String
    {
        while (str.length < 6) {
            str = "0" + str;
        }
        return str;
    }

    /**
     * Sets a cookie value in the containing context.
     */
    protected function setCookie (name :String, value :String) :void
    {
        ExternalInterface.call("setCookie", name, value);
    }

    /**
     * Retrieves the foreground color preference as a string.
     */
    protected function getForeground () :String
    {
        return getCookie("foreground", "00FF00");
    }

    /**
     * Retrieves the background color preference as a string.
     */
    protected function getBackground () :String
    {
        return getCookie("background", "000000");
    }

    /**
     * Retrieves a cookie value from the containing context.
     */
    protected function getCookie (name :String, def :String) :String
    {
        var values :Array = ExternalInterface.call("eval", "document.cookie").split(";");
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
     * Handles mouse events.
     */
    protected function sendMouseMessage (event :MouseEvent) :void
    {
        if (!_socket.connected) {
            return;
        }
        var x :int = (event.stageX - _bitmap.x) / _charWidth;
        var y :int = (event.stageY - _bitmap.y) / _charHeight;
        if (x < 0 || y < 0 || x >= _width || y >= _height) {
            return;
        }
        var out :IDataOutput = startMessage();
        out.writeByte(event.type == MouseEvent.MOUSE_DOWN ?
            MOUSE_PRESSED_MSG : MOUSE_RELEASED_MSG);
        out.writeShort(x);
        out.writeShort(y);
        endMessage(out);
    }

    /**
     * Handles key events.
     */
    protected function sendKeyMessage (event :KeyboardEvent) :void
    {
        if (!_socket.connected) {
            return;
        }
        var out :IDataOutput = startMessage();
        var numpad :Boolean = (event.keyLocation == KeyLocation.NUM_PAD);
        out.writeByte(event.type == KeyboardEvent.KEY_DOWN ?
            (numpad ? KEY_PRESSED_NUMPAD_MSG : KEY_PRESSED_MSG) :
            (numpad ? KEY_RELEASED_NUMPAD_MSG : KEY_RELEASED_MSG));
        out.writeUnsignedInt(getQtKeyCode(event));
        out.writeShort(event.charCode);
        endMessage(out);
    }

    /**
     * Handles incoming socket data.
     */
    protected function readMessages (event :ProgressEvent) :void
    {
        var decoded :Boolean = false;

        while (_socket.connected) {
            if (_messageLength == 0) {
                if (_socket.bytesAvailable < 2) {
                    break;
                }
                _messageLength = _socket.readUnsignedShort();

            } else {
                if (_socket.bytesAvailable < _messageLength) {
                    return;
                }
                var bytes :ByteArray = new ByteArray();
                _socket.readBytes(bytes, 0, _messageLength);
                if (_crypto) {
                    _dctx.decrypt(bytes);
                }
                _messageLength = 0;

                bytes.position = 0;
                decodeMessage(bytes);
                decoded = true;
            }
        }
        if (decoded) {
            updateDisplay();
        }
    }

    /**
     * Decodes the message contained in the provided byte array.
     */
    protected function decodeMessage (bytes :ByteArray) :void
    {
        var type :int = bytes.readUnsignedByte();
        switch (type) {
            case UPDATE_WINDOW_MSG:
                updateWindow(bytes.readInt(), bytes.readInt(),
                    readRectangle(bytes), bytes.readInt());
                break;

            case REMOVE_WINDOW_MSG:
                removeWindow(bytes.readInt());
                break;

            case SET_CONTENTS_MSG:
                var id :int = bytes.readInt();
                var bounds :Rectangle = readRectangle(bytes);
                var contents :Array = new Array(bounds.width * bounds.height);
                for (var ii :int = 0; ii < contents.length; ii++) {
                    contents[ii] = bytes.readInt();
                }
                setWindowContents(id, bounds, contents);
                break;

            case MOVE_CONTENTS_MSG:
                moveWindowContents(bytes.readInt(), readRectangle(bytes),
                    new Point(bytes.readShort(), bytes.readShort()), bytes.readInt());
                break;

            case SET_COOKIE_MSG:
                setCookie(bytes.readUTF(), bytes.readUTFBytes(bytes.bytesAvailable));
                break;

            case CLOSE_MSG:
                fatalError(bytes.readUTFBytes(bytes.bytesAvailable));
                _socket.close();
                break;

            case TOGGLE_CRYPTO_MSG:
                // respond immediately in the affirmative and toggle
                var out :IDataOutput = startMessage();
                out.writeByte(CRYPTO_TOGGLED_MSG);
                endMessage(out);
                _crypto = !_crypto;
                break;

            case COMPOUND_MSG:
                while (bytes.bytesAvailable > 0) {
                    var mbytes :ByteArray = new ByteArray();
                    bytes.readBytes(mbytes, 0, bytes.readUnsignedShort());
                    mbytes.position = 0;
                    decodeMessage(mbytes);
                }
                break;

            default:
                trace("Unknown message type.", type);
        }
    }

    /**
     * Starts a message.
     *
     * @return the output stream to write to.
     */
    protected function startMessage () :IDataOutput
    {
        return _crypto ? new ByteArray() : _socket;
    }

    /**
     * Finishes a message.
     *
     * @param out the output stream returned from {@link #startMessage}.
     */
    protected function endMessage (out :IDataOutput) :void
    {
        if (_crypto) {
            var bytes :ByteArray = ByteArray(out);
            _ectx.encrypt(bytes);
            _socket.writeBytes(bytes);
        }
        _socket.flush();
    }

    /**
     * Reads and returns a rectangle from the provided array.
     */
    protected function readRectangle (bytes :ByteArray) :Rectangle
    {
        return new Rectangle(bytes.readShort(), bytes.readShort(),
            bytes.readShort(), bytes.readShort());
    }

    /**
     * Creates a new centered window with the supplied text, indicating a fatal error
     * (unable to connect to or disconnected from the server).
     */
    protected function fatalError (text :String) :void
    {
        addClientWindow(int.MAX_VALUE, text);
    }

    /**
     * Creates a new centered window with the supplied id (which also acts as the layer)
     * and text.
     */
    protected function addClientWindow (id :int, text :String) :void
    {
        text = " " + text + " ";
        var bounds :Rectangle = new Rectangle(
            int((_width - text.length - 2) / 2), int(_height/2), text.length + 2, 3);
        var window :Window = new Window(id, id, bounds, "#".charCodeAt(0));
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
     * Updates the identified window, adding it if it isn't present.
     */
    protected function updateWindow (id :int, layer :int, bounds :Rectangle, fill :int) :void
    {
        var idx :int = getWindowIndex(id, false);
        if (idx == -1) {
            addWindow(new Window(id, layer, bounds, fill));
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
     * Removes the window with the specified id from the list.
     *
     * @param warn if true, trace a warning if the window wasn't found.
     */
    protected function removeWindow (id :int, warn :Boolean = true) :void
    {
        var idx :int = getWindowIndex(id, warn);
        if (idx != -1) {
            addDirtyRegion(_windows[idx].bounds);
            _windows.splice(idx, 1);
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
                bounds.width, bounds.height), 0x0000FF);
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
                source.width, source.height), 0xFF0000);
            addDirtyRegion(new Rectangle(window.bounds.x + dest.x, window.bounds.y + dest.y,
                source.width, source.height), 0x00FF00);
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
     *
     * @param warn if true, trace a warning if the window wasn't found.
     */
    protected function getWindowIndex (id :int, warn :Boolean = true) :int
    {
        for (var ii :int = 0; ii < _windows.length; ii++) {
            if (_windows[ii].id == id) {
                return ii;
            }
        }
        if (warn) {
            trace("Window not found.", id);
        }
        return -1;
    }

    /**
     * Adds a region to the rectangle that will need updating.
     */
    protected function addDirtyRegion (region :Rectangle, color :uint = 0xFFFFFF) :void
    {
        _dirty = _dirty.union(region);

        if (_debugRegions) {
            // add the outline of the region
            var shape :Shape = new Shape();
            shape.graphics.lineStyle(1, color);
            shape.graphics.drawRect(0, 0, _charWidth*region.width, _charHeight*region.height);
            shape.x = _bitmap.x + _charWidth*region.x;
            shape.y = _bitmap.y + _charHeight*region.y;
            addChild(shape);

            // remove it after a second
            var timer :Timer = new Timer(1000, 1);
            timer.addEventListener(TimerEvent.TIMER_COMPLETE, function (event :TimerEvent) :void {
                removeChild(shape);
            });
            timer.start();
        }
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

        // now update the bitmap with the new contents
        _bitmap.bitmapData.lock();
        var srect :Rectangle = new Rectangle(0, 0, _charWidth, _charHeight);
        var dpoint :Point = new Point();
        var drect :Rectangle = new Rectangle(0, 0, _charWidth, _charHeight);
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

                if (nvalue == 0x20) {
                    drect.x = xx*_charWidth;
                    drect.y = yy*_charHeight;
                    _bitmap.bitmapData.fillRect(drect, stage.color);

                } else {
                    dpoint.x = xx*_charWidth;
                    dpoint.y = yy*_charHeight;
                    _bitmap.bitmapData.copyPixels(getBitmapData(nvalue), srect, dpoint);
                }
            }
        }
        _bitmap.bitmapData.unlock();
    }

    /**
     * Returns through the cache the bitmap corresponding to the specified value.
     */
    protected function getBitmapData (value :int) :BitmapData
    {
        var data :BitmapData = _bitmapData[value];
        if (data == null) {
            _charField.text = String.fromCharCode(value & 0xFFFF);
            var bg :uint;
            if ((value & REVERSE_FLAG) == 0) {
                bg = _reverseFormat.color as uint;
                _charField.setTextFormat((value & DIM_FLAG) == 0 ? _normalFormat : _dimFormat);
            } else {
                bg = _normalFormat.color as uint;
                _charField.setTextFormat((value & DIM_FLAG) == 0 ? _reverseFormat : _dimFormat);
            }
            _bitmapData[value] = data = new BitmapData(_charWidth, _charHeight, false, bg);
            var bounds :Rectangle = _charField.getCharBoundaries(0);
            var matrix :Matrix = new Matrix();
            matrix.translate(-bounds.x, -bounds.y);
            data.draw(_charField, matrix);
        }
        return data;
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

    /** The contents of the display as integers. */
    protected var _contents :Array;

    /** Formats for normal, reverse, and dim text. */
    protected var _normalFormat :TextFormat, _reverseFormat :TextFormat, _dimFormat :TextFormat;

    /** The field we reuse to create character bitmaps. */
    protected var _charField :TextField;

    /** Big ol' bitmap. */
    protected var _bitmap :Bitmap;

    /** The cached bitmap data for each character. */
    protected var _bitmapData :Object = new Object();

    /** The width and height of the display in characters. */
    protected var _width :int, _height :int;

    /** The width and height of each character. */
    protected var _charWidth :int, _charHeight :int;

    /** The socket via which we communicate with the server. */
    protected var _socket :Socket;

    /** The contexts that we use for encryption and decryption. */
    protected var _ectx :CBCMode, _dctx :CBCMode;

    /** Whether or not to encrypt incoming/outgoing messages. */
    protected var _crypto :Boolean = false;

    /** The length of the current server message. */
    protected var _messageLength :int = 0;

    /** The list of windows (sorted by layer). */
    protected var _windows :Array = [ ];

    /** The current dirty region. */
    protected var _dirty :Rectangle = new Rectangle();

    /** Whether or not to show the redraw regions. */
    protected var _debugRegions :Boolean = false;

    /** Whether or not we're running in debug mode. */
    protected static var DEBUG :Boolean = true;

    /** Flag indicating that the character should be displayed in reverse. */
    protected static var REVERSE_FLAG :int = 0x10000;

    /** Flag indicating that the character should be displayed half-bright. */
    protected static var DIM_FLAG :int = 0x20000;

    /** The magic number that identifies the protocol. */
    protected static var PROTOCOL_MAGIC :int = 0x57544750; // "WTGP"

    /** The protocol version. */
    protected static var PROTOCOL_VERSION :int = 0x00000001;

    /** Outgoing message: mouse pressed. */
    protected static var MOUSE_PRESSED_MSG :int = 0;

    /** Outgoing message: mouse released. */
    protected static var MOUSE_RELEASED_MSG :int = 1;

    /** Outgoing message: key pressed. */
    protected static var KEY_PRESSED_MSG :int = 2;

    /** Outgoing message: key pressed on the number pad. */
    protected static var KEY_PRESSED_NUMPAD_MSG :int = 3;

    /** Outgoing message: key released. */
    protected static var KEY_RELEASED_MSG :int = 4;

    /** Outgoing message: key released on the number pad. */
    protected static var KEY_RELEASED_NUMPAD_MSG :int = 5;

    /** Outgoing message: window closed. */
    protected static var WINDOW_CLOSED_MSG :int = 6;

    /** Outgoing message: encryption toggled. */
    protected static var CRYPTO_TOGGLED_MSG :int = 7;

    /** Incoming message: add or update window. */
    protected static var UPDATE_WINDOW_MSG :int = 0;

    /** Incoming message: remove window. */
    protected static var REMOVE_WINDOW_MSG :int = 1;

    /** Incoming message: set contents. */
    protected static var SET_CONTENTS_MSG :int = 2;

    /** Incoming message: move contents. */
    protected static var MOVE_CONTENTS_MSG :int = 3;

    /** Incoming message: set cookie. */
    protected static var SET_COOKIE_MSG :int = 4;

    /** Incoming message: close. */
    protected static var CLOSE_MSG :int = 5;

    /** Incoming message: toggle encryption. */
    protected static var TOGGLE_CRYPTO_MSG :int = 6;

    /** Incoming message: compound message follows. */
    protected static var COMPOUND_MSG :int = 7;
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
