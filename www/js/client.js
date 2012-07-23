//
// $Id$

/** The 2D graphics context for the canvas. */
var ctx;

/** The dimensions of each character in pixels. */
var charWidth, charHeight;

/** The width and height of the display in characters. */
var width, height;

/** The pixel offset of the character grid within the display. */
var offsetX, offsetY;

/** The contents of the display as integers. */
var contents;

/** Cached character bitmaps mapped by value. */
var bitmapData = Object();

/** The socket through which we communicate with the server. */
var socket;

/** The list of windows (sorted by layer). */
var windows = new Array();

/** The current dirty rectangle. */
var dirty;

/**
 * Entry point: called on load to initialize the client.
 */
function init ()
{
    var canvas = document.getElementById("canvas");
    ctx = canvas.getContext("2d");
    ctx.font = "12px Courier";
    ctx.fillStyle = "#00FF00";
    ctx.textAlign = "left";
    ctx.textBaseline = "top";
    charWidth = ctx.measureText("A").width;
    charHeight = 12;
    width = Math.floor(canvas.width / charWidth);
    height = Math.floor(canvas.height / charHeight);
    offsetX = Math.round((canvas.width % charWidth) / 2);
    offsetY = Math.round((canvas.height % charHeight) / 2);
    dirty = new Rectangle(0, 0, width, height);

    // create and initialize the combined contents array
    contents = new Array(width * height);
    for (var ii = 0; ii < contents.length; ii++) {
        contents[ii] = 0x20;
    }

    document.onkeydown = function (event) {
    };
    document.onkeyup = function (event) {
    };

    if ("WebSocket" in window) {
        socket = new WebSocket("ws://localhost:8080/client");
    } else if ("MozWebSocket" in window) {
        socket = new MozWebSocket("ws://localhost:8080/client");
    }
    socket.onopen = function () {
    };
    socket.onerror = function () {
        fatalError("No connection to server.  Please wait a moment, " +
            "then reload the page.");
    };
    socket.onmessage = function (event) {
    };
}

/**
 * Creates a new centered window with the supplied text, indicating a fatal error (unable to
 * connect to or disconnected from the server).
 */
function fatalError (text)
{
    addClientWindow(Number.MAX_VALUE, text);
}

/**
 * Creates a new centered window with the supplied id (which also acts as the layer) and text.
 */
function addClientWindow (id, text)
{
    text = " " + text + " ";
    var bounds = new Rectangle(
        Math.floor((width - text.length - 2) / 2), Math.floor(height/2), text.length + 2, 3);
    var window = new Window(id, id, bounds, "#".charCodeAt(0));
    for (var ii = 0; ii < text.length; ii++) {
        window.contents[text.length + 3 + ii] = text.charCodeAt(ii);
    }
    addWindow(window);
    updateDisplay();
}

/**
 * Adds a window to the list.
 */
function addWindow (window)
{
    addDirtyRegion(window.bounds);
    for (var ii = windows.length - 1; ii >= 0; ii--) {
        if (window.layer >= windows[ii].layer) {
            windows.splice(ii + 1, 0, window);
            return;
        }
    }
    windows.splice(0, 0, window);
}

/**
 * Updates the identified window, adding it if it isn't present.
 */
function updateWindow (id, layer, bounds, fill)
{
    var idx = getWindowIndex(id);
    if (idx == -1) {
        addWindow(new Window(id, layer, bounds, fill));
        return;
    }
    var window = windows[idx];
    if (window.layer != layer) {
        // remove and reinsert
        windows.splice(idx, 1);
        window.layer = layer;
        addWindow(window);
    }
    addDirtyRegion(window.bounds);
    if (!window.bounds.equals(bounds)) {
        addDirtyRegion(bounds);
        if (!window.bounds.size().equals(bounds.size())) {
            window.resizeContents(bounds.size(), fill);
        }
        window.bounds.copyFrom(bounds);
    }
}

/**
 * Removes the window with the specified id from the list.
 */
function removeWindow (id)
{
    var idx = getWindowIndex(id);
    if (idx != -1) {
        addDirtyRegion(windows[idx].bounds);
        windows.splice(idx, 1);
    }
}

/**
 * Sets part of a window's contents.
 */
function setWindowContents (id, bounds, contents)
{
    var window = getWindow(id);
    if (window != null) {
        addDirtyRegion(new Rectangle(window.bounds.x + bounds.x, window.bounds.y + bounds.y,
            bounds.width, bounds.height));
        window.setContents(bounds, contents);
    }
}

/**
 * Moves part of a window's contents.
 */
function moveWindowContents (id, src, dest, fill)
{
    var window = getWindow(id);
    if (window != null) {
        addDirtyRegion(new Rectangle(window.bounds.x + src.x, window.bounds.y + src.y,
            src.width, src.height));
        addDirtyRegion(new Rectangle(window.bounds.x + dest.x, window.bounds.y + dest.y,
            src.width, src.height));
        window.moveContents(src, dest, fill);
    }
}

/**
 * Returns a reference to the window with the specified id, or null if not found.
 */
function getWindow (id)
{
    var idx = getWindowIndex(id);
    return (idx == -1) ? null : windows[idx];
}

/**
 * Returns the index of the window with the specified id, or -1 if not found.
 */
function getWindowIndex (id)
{
    for (var ii = 0; ii < windows.length; ii++) {
        if (windows[ii].id == id) {
            return ii;
        }
    }
    return -1;
}

/**
 * Adds a region to the rectangle that will need updating.
 */
function addDirtyRegion (region)
{
    dirty = dirty.union(region);
}

/**
 * Updates the portion of the display covered by the dirty region.
 */
function updateDisplay ()
{
    // get the intersection of the screen bounds and the dirty region
    var bounds = dirty.intersection(new Rectangle(0, 0, width, height));
    dirty.setEmpty();
    if (bounds.isEmpty()) {
        return;
    }

    // first, combine the contents of all intersecting windows
    var combined = new Array(bounds.width * bounds.height);
    for (var ii = 0; ii < windows.length; ii++) {
        var window = windows[ii];
        var isect = window.bounds.intersection(bounds);
        if (isect.isEmpty()) {
            continue;
        }
        for (var yy = isect.y, yymax = isect.y + isect.height; yy < yymax; yy++) {
            for (var xx = isect.x, xxmax = isect.x + isect.width; xx < xxmax; xx++) {
                var value = window.contents[
                    (yy - window.bounds.y)*window.bounds.width + (xx - window.bounds.x)];
                if (value != 0x0) { // zero = "transparent"
                    combined[(yy - bounds.y)*bounds.width + (xx - bounds.x)] = value;
                }
            }
        }
    }

    // now update the canvas with the new contents
    for (yy = bounds.y, yymax = bounds.y + bounds.height; yy < yymax; yy++) {
        for (xx = bounds.x, xxmax = bounds.x + bounds.width; xx < xxmax; xx++) {
            var obj = combined[(yy - bounds.y) * bounds.width + (xx - bounds.x)];
            var nvalue = (obj == null) ? 0x20 : obj;
            var idx = yy*width + xx;
            var ovalue = contents[idx];
            if (ovalue == nvalue) {
                continue;
            }
            drawCharacter(xx, yy, nvalue);
            contents[idx] = nvalue;
        }
    }
}

/**
 * Draws a character at the specified (character) coordinates.
 */
function drawCharacter (x, y, value)
{
    var px = offsetX + x*charWidth;
    var py = offsetY + y*charHeight;
    var data = bitmapData[value];
    if (data) {
        ctx.putImageData(data, px, py);
        return;
    }
    ctx.clearRect(px, py, charWidth, charHeight);
    ctx.fillText(String.fromCharCode(value & 0xFFFF), px, py);
    bitmapData[value] = ctx.getImageData(px, py, charWidth, charHeight);
}

/**
 * Sets the value of the cookie with the given key.
 */
function setCookie (key, value)
{
    var expires = new Date();
    expires.setFullYear(expires.getFullYear() + 5);
    document.cookie = key + "=" + escape(value) + "; expires=" + expires.toUTCString();
}

/**
 * Returns the value of the cookie with the specified key, or the given default if not set.
 */
function getCookie (key, def)
{
    var cookie = document.cookie;
    var values = cookie.split(";");
    for (ii = 0; ii < values.length; ii++) {
        var value = values[ii];
        var idx = value.indexOf("=");
        if (value.substring(0, idx).replace(/\s+/, "") == key) {
            return unescape(value.substring(idx + 1));
        }
    }
    return def;
}

/**
 * Returns the Qt key code corresponding to the given keyboard event.
 */
function getQtKeyCode (event)
{
    switch (event.keyCode) {
        case 27: return 0x01000000;
        case 112: return 0x01000030;
        case 113: return 0x01000031;
        case 114: return 0x01000032;
        case 115: return 0x01000033;
        case 116: return 0x01000034;
        case 117: return 0x01000035;
        case 118: return 0x01000036;
        case 119: return 0x01000037;
        case 120: return 0x01000038;
        case 121: return 0x01000039;
        case 122: return 0x0100003a;
        case 123: return 0x0100003b;
        case 124: return 0x0100003c;
        case 125: return 0x0100003d;
        case 126: return 0x0100003e;
        case 44: return 0x01000009;
        case 145: return 0x01000026;
        case 19: return 0x01000008;

        case 192: return 0x60;
        case 48: return 0x30;
        case 49: return 0x31;
        case 50: return 0x32;
        case 51: return 0x33;
        case 52: return 0x34;
        case 53: return 0x35;
        case 54: return 0x36;
        case 55: return 0x37;
        case 56: return 0x38;
        case 57: return 0x39;
        case 189: return 0x2d;
        case 187: return 0x3d;
        case 8: return 0x01000003;
        case 45: return 0x01000006;
        case 36: return 0x01000010;
        case 33: return 0x01000016;
        case 111: return 0x2f;
        case 106: return 0x2a;
        case 109: return 0x2d;

        case 9: return 0x01000001;
        case 81: return 0x51;
        case 87: return 0x57;
        case 69: return 0x45;
        case 82: return 0x52;
        case 84: return 0x54;
        case 89: return 0x59;
        case 85: return 0x55;
        case 73: return 0x49;
        case 79: return 0x4f;
        case 80: return 0x50;
        case 219: return 0x5b;
        case 221: return 0x5d;
        case 220: return 0x5c;
        case 46: return 0x01000007;
        case 35: return 0x01000011;
        case 34: return 0x01000017;
        case 103: return 0x37;
        case 104: return 0x38;
        case 105: return 0x39;
        case 107: return 0x2b;

        case 20: return 0x01000024;
        case 65: return 0x41;
        case 83: return 0x53;
        case 68: return 0x44;
        case 70: return 0x46;
        case 71: return 0x47;
        case 72: return 0x48;
        case 74: return 0x4a;
        case 75: return 0x4b;
        case 76: return 0x4c;
        case 186: return 0x3b;
        case 222: return 0x27;
        case 13: return 0x01000004;
        case 100: return 0x34;
        case 101: return 0x35;
        case 102: return 0x36;

        case 16: return 0x01000020;
        case 90: return 0x5a;
        case 88: return 0x58;
        case 67: return 0x43;
        case 86: return 0x56;
        case 66: return 0x42;
        case 78: return 0x4e;
        case 77: return 0x4d;
        case 188: return 0x2c;
        case 190: return 0x2e;
        case 191: return 0x2f;
        case 38: return 0x01000013;
        case 97: return 0x31;
        case 98: return 0x32;
        case 99: return 0x33;

        case 17: return 0x01000021;
        case 224: return 0x01000021;
        case 18: return 0x01000023;
        case 32: return 0x20;
        case 93: return 0x01000055;
        case 37: return 0x01000012;
        case 40: return 0x01000015;
        case 39: return 0x01000014;

        default: return 0x01ffffff;
    }
}


/**
 * Constructor for managed windows.
 */
function Window (id, layer, bounds, fill)
{
    this.id = id;
    this.layer = layer;
    this.bounds = bounds.clone();

    var size = bounds.width * bounds.height;
    this.contents = new Array(size);
    for (var ii = 0; ii < size; ii++) {
        this.contents[ii] = fill;
    }
}

/**
 * Resizes the contents to the specified dimensions.
 */
Window.prototype.resizeContents = function (size, fill)
{
    var osize = this.bounds.size();
    var ocontents = this.contents;
    this.contents = new Array(size.x * size.y);
    for (var yy = 0; yy < size.y; yy++) {
        for (var xx = 0; xx < size.x; xx++) {
            this.contents[yy*size.x + xx] = (xx < osize.x && yy < osize.y) ?
                ocontents[yy*osize.x + xx] : fill;
        }
    }
};

/**
 * Sets part of the window's contents.
 */
Window.prototype.setContents = function (rect, values)
{
    for (var yy = 0; yy < rect.height; yy++) {
        for (var xx = 0; xx < rect.width; xx++) {
            this.contents[(rect.y + yy)*this.bounds.width + (rect.x + xx)] =
                values[yy*rect.width + xx];
        }
    }
}

/**
 * Moves part of a window's contents.
 */
Window.prototype.moveContents = function (src, dest, fill)
{
    // copy the source to a temporary buffer and clear it
    var values = new Array(src.width * src.height);
    for (var yy = 0; yy < src.height; yy++) {
        for (var xx = 0; xx < src.width; xx++) {
            var cidx = (src.y + yy)*this.bounds.width + (src.x + xx);
            values[yy*src.width + xx] = this.contents[cidx];
            this.contents[cidx] = fill;
        }
    }

    // then set it in the main one
    setContents(new Rectangle(dest.x, dest.y, src.width, src.height), values);
}
