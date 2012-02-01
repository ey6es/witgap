//
// $Id$

package {

import flash.display.Sprite;

import flash.events.Event;
import flash.events.IOErrorEvent;
import flash.events.ProgressEvent;
import flash.events.SecurityErrorEvent;

import flash.geom.Rectangle;

import flash.net.Socket;

import flash.text.TextField;
import flash.text.TextFieldAutoSize;
import flash.text.TextFormat;

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
        _field.multiline = true;
        var format :TextFormat = new TextFormat();
        format.font = "_typewriter";
        _field.setTextFormat(format);

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
        _field.setTextFormat(format);
        _field.x = (loaderInfo.width - _field.width) / 2;
        _field.y = (loaderInfo.height - _field.height) / 2;

        // create the socket
        _socket = new Socket();
        _socket.addEventListener(Event.CONNECT, function (event :Event) :void {
            // write the magic number and version
            _socket.writeUnsignedInt(0x57544750);
            _socket.writeUnsignedInt(0x00000001);
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
    }

    /** Our gigantic text field. */
    protected var _field :TextField;

    /** The width and height of the display in characters. */
    protected var _width :int, _height :int;

    /** The socket via which we communicate with the server. */
    protected var _socket :Socket;
}
}
