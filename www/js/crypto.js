//
// $Id$

function Random ()
{
    this.i = 0;
    this.j = 0;
    this.S = new ByteArray();

    for (var ii = 0; ii < 256; ii++) {
        this.S.set(ii, ii);
    }

    var key = new ByteArray();
    for (var ii = 0; ii < 256; ii += 2) {
        var t = Math.floor(Math.random() * 65536);
        key.set(ii, t >> 8);
        key.set(ii + 1, t & 0xFF);
    }

	var x = new Date().getTime();
	key.set(0, key.get(0) ^ (x & 0xFF));
	key.set(1, key.get(1) ^ ((x >> 8) & 0xFF));
	key.set(2, key.get(2) ^ ((x >> 16) & 0xFF));
	key.set(3, key.get(3) ^ ((x >> 24) & 0xFF));

    var j = 0;
    for (var ii = 0; ii < 256; ii++) {
        var t = this.S.get(ii);
        j = (j + t + key.get(ii % key.size)) & 0xFF;
		this.S.set(ii, this.S.get(j));
		this.S.set(j, t);
    }
}

Random.prototype.nextBytes = function (buffer, len)
{
    while (len--) {
        buffer.writeByte(this.nextByte());
	}
}

Random.prototype.nextByte = function ()
{
    this.i = (this.i + 1) & 0xFF;
    this.j = (this.j + this.S.get(this.i)) & 0xFF;
    var t = this.S.get(this.i);
    this.S.set(this.i, this.S.get(this.j));
    this.S.set(this.j, t);
    return this.S.get((t + this.S.get(this.i)) & 0xFF);
}
