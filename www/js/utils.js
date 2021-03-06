//
// $Id$

// make sure console is defined
if (!console) {
    console = { log: function () { } };
}

/**
 * Constructor for ByteArray objects, after flash.utils.ByteArray.
 */
function ByteArray (buffer)
{
    if (buffer) {
        this.size = buffer.byteLength;
    } else {
        buffer = new ArrayBuffer(8);
        this.size = 0;
    }
    this.view = new Int8Array(buffer);
    this.position = 0;
}

ByteArray.prototype.get = function (position)
{
    return this.view[position] & 0xFF;
};

ByteArray.prototype.set = function (position, value)
{
    this.willWrite(position, 1);
    this.view[position] = value;
};

ByteArray.prototype.clear = function ()
{
    this.size = 0;
    this.position = 0;
};

ByteArray.prototype.bytesAvailable = function ()
{
    return (this.size - this.position);
};

ByteArray.prototype.compact = function ()
{
    return this.view.buffer.slice(0, this.size);
};

/**
 * Writes a byte value to the array.
 */
ByteArray.prototype.writeByte = function (value)
{
    this.willWrite(this.position, 1);
    this.view[this.position++] = value;
};

/**
 * Reads an unsigned byte value from the array.
 */
ByteArray.prototype.readUnsignedByte = function ()
{
    return this.view[this.position++] & 0xFF;
};

/**
 * Writes a short value to the array.
 */
ByteArray.prototype.writeShort = function (value)
{
    this.willWrite(this.position, 2);
    this.view[this.position++] = value >> 8;
    this.view[this.position++] = value & 0xFF;
};

/**
 * Reads a short value from the array.
 */
ByteArray.prototype.readShort = function ()
{
    return (this.view[this.position++] << 8) | (this.view[this.position++] & 0xFF);
};

/**
 * Reads an unsigned short value from the array.
 */
ByteArray.prototype.readUnsignedShort = function ()
{
    return ((this.view[this.position++] & 0xFF) << 8) | (this.view[this.position++] & 0xFF);
};

/**
 * Writes an unsigned int value to the array.
 */
ByteArray.prototype.writeUnsignedInt = function (value)
{
    this.willWrite(this.position, 4);
    this.view[this.position++] = value >> 24;
    this.view[this.position++] = (value >> 16) & 0xFF;
    this.view[this.position++] = (value >> 8) & 0xFF;
    this.view[this.position++] = value & 0xFF;
};

/**
 * Reads an int value from the array.
 */
ByteArray.prototype.readInt = function ()
{
    return (this.view[this.position++] << 24) |
        ((this.view[this.position++] & 0xFF) << 16) |
        ((this.view[this.position++] & 0xFF) << 8) |
        (this.view[this.position++] & 0xFF);
};

/**
 * Writes a byte array to the array.
 */
ByteArray.prototype.writeBytes = function (array, position, size)
{
    if (position == null) {
        position = 0;
    }
    if (size == null) {
        size = array.size;
    }
    this.willWrite(this.position, size);
    this.view.set(array.view.subarray(position, position + size), this.position);
    this.position += size;
};

/**
 * Reads a byte array from the array.
 */
ByteArray.prototype.readBytes = function (array, position, size)
{
    if (position == null) {
        position = 0;
    }
    if (size == null) {
        size = this.bytesAvailable();
    }
    array.willWrite(position, size);
    array.view.set(this.view.subarray(this.position, this.position + size), position);
    this.position += size;
};

/**
 * Writes a UTF string to the array.  See http://en.wikipedia.org/wiki/UTF-8.
 */
ByteArray.prototype.writeUTF = function (value)
{
    var array = new ByteArray();
    for (var ii = 0; ii < value.length; ii++) {
        var ch = value.charCodeAt(ii);
        if (ch <= 0x7F) {
            array.writeByte(ch);

        } else if (ch <= 0x7FF) {
            array.writeByte(0xC0 | (ch >> 6));
            array.writeByte(0x80 | (ch & 0x3F));

        } else if (ch <= 0xFFFF) {
            array.writeByte(0xE0 | (ch >> 12));
            array.writeByte(0x80 | ((ch >> 6) & 0x3F));
            array.writeByte(0x80 | (ch & 0x3F));

        } else { // ch <= 0x10FFFF
            array.writeByte(0xF0 | (ch >> 18));
            array.writeByte(0x80 | ((ch >> 12) & 0x3F));
            array.writeByte(0x80 | ((ch >> 6) & 0x3F));
            array.writeByte(0x80 | (ch & 0x3F));
        }
    }
    this.writeShort(array.size);
    this.writeBytes(array);
};

/**
 * Reads a UTF string from the array.
 */
ByteArray.prototype.readUTF = function ()
{
    return this.readUTFBytes(this.readUnsignedShort());
};

/**
 * Reads a UTF string from the array.
 */
ByteArray.prototype.readUTFBytes = function (len)
{
    var value = "";
    for (var ii = 0; ii < len; ii++) {
        var ch = this.readUnsignedByte();
        if (ch < 0x80) {
            value += String.fromCharCode(ch);

        } else if (ch < 0xE0) {
            value += String.fromCharCode(
                ((ch & 0x1F) << 6) |
                (this.readUnsignedByte() & 0x3F));

        } else if (ch < 0xF0) {
            value += String.fromCharCode(
                ((ch & 0x0F) << 12) |
                ((this.readUnsignedByte() & 0x3F) << 6) |
                (this.readUnsignedByte() & 0x3F));

        } else {
            value += String.fromCharCode(
                ((ch & 0x07) << 18) |
                ((this.readUnsignedByte() & 0x3F) << 12) |
                ((this.readUnsignedByte() & 0x3F) << 6) |
                (this.readUnsignedByte() & 0x3F));
        }
    }
    return value;
};

/**
 * Notes that we're about to write the specified number of bytes to the given position, expands
 * buffer and adjusts size as necessary.
 */
ByteArray.prototype.willWrite = function (position, size)
{
    var osize = this.size;
    this.size = Math.max(this.size, position + size);
    if (this.size <= this.view.byteLength) {
        return;
    }
    // double the size of the underlying buffer until it fits and copy from old to new
    var ncapacity = this.view.byteLength * 2;
    while (this.size > ncapacity) {
        ncapacity *= 2;
    }
    var oview = this.view;
    this.view = new Int8Array(ncapacity);
    this.view.set(oview.subarray(0, osize));
};

ByteArray.prototype.toString = function ()
{
    var str = "";
    for (var ii = 0; ii < this.size; ii++) {
        var num = this.get(ii).toString(16);
        str += (num.length == 1 ? "0" : "") + num;
    }
    return str;
};
