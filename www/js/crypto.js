//
// $Id$

/*
 * Derived from as3crypto library (http://code.google.com/p/as3crypto/).  License:
 *
 * Copyright (c) 2007 Henri Torgemane
 * All Rights Reserved.
 *
 * BigInteger, RSA, Random and ARC4 are derivative works of the jsbn library
 * (http://www-cs-students.stanford.edu/~tjw/jsbn/)
 * The jsbn library is Copyright (c) 2003-2005  Tom Wu (tjw@cs.Stanford.EDU)
 *
 * MD5, SHA1, and SHA256 are derivative works (http://pajhome.org.uk/crypt/md5/)
 * Those are Copyright (c) 1998-2002 Paul Johnston & Contributors (paj@pajhome.org.uk)
 *
 * SHA256 is a derivative work of jsSHA2 (http://anmar.eu.org/projects/jssha2/)
 * jsSHA2 is Copyright (c) 2003-2004 Angel Marin (anmar@gmx.net)
 *
 * AESKey is a derivative work of aestable.c (http://www.geocities.com/malbrain/aestable_c.html)
 * aestable.c is Copyright (c) Karl Malbrain (malbrain@yahoo.com)
 *
 * BlowFishKey, DESKey and TripeDESKey are derivative works of the Bouncy Castle Crypto Package (http://www.bouncycastle.org)
 * Those are Copyright (c) 2000-2004 The Legion Of The Bouncy Castle
 *
 * Base64 is copyright (c) 2006 Steve Webster (http://dynamicflash.com/goodies/base64)
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer. Redistributions in binary form must
 * reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the author nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL TOM WU BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF
 * THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

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

function PKCS5 (blockSize)
{
    this.blockSize = blockSize;
}

PKCS5.prototype.pad = function (a)
{
	var c = this.blockSize - a.size % this.blockSize;
	for (var ii = 0; ii < c; ii++) {
	    a.set(a.size, c);
	}
}

PKCS5.prototype.unpad = function (a)
{
    var c = a.size % this.blockSize;
    if (c != 0) {
        throw new Error("PKCS#5::unpad: ByteArray.length isn't a multiple of the blockSize");
    }
    c = a.get(a.size - 1);
    for (var ii = c; ii > 0; ii--) {
        var v = a.get(a.size - 1);
        a.size--;
	    if (c != v) {
            throw new Error("PKCS#5:unpad: Invalid padding value. expected [" +
                c + "], found [" + v + "]");
	    }
	}
}

function CBCMode (key, iv)
{
    this.key = key;
    this.blockSize = key.getBlockSize();
    this.padding = new PKCS5(this.blockSize);
    this.lastIV = new ByteArray();
    this.lastIV.writeBytes(iv);
}

CBCMode.prototype.encrypt = function (src)
{
    this.padding.pad(src);
    var vector = this.lastIV;
    for (var ii = 0; ii < src.size; ii += this.blockSize) {
        for (var jj = 0; jj < this.blockSize; jj++) {
            src.set(ii + jj, src.get(ii + jj) ^ vector.get(jj));
        }
        this.key.encrypt(src, ii);
        vector.position = 0;
        vector.writeBytes(src, ii, this.blockSize);
    }
}

CBCMode.prototype.decrypt = function (src)
{
    var vector = this.lastIV;
    var tmp = new ByteArray();
    for (var ii = 0; ii < src.size; ii += this.blockSize) {
        tmp.position = 0;
        tmp.writeBytes(src, ii, this.blockSize);
        this.key.decrypt(src, ii);
        for (var jj = 0; jj < this.blockSize; jj++) {
            src.set(ii + jj, src.get(ii + jj) ^ vector.get(jj));
        }
        vector.position = 0;
        vector.writeBytes(tmp, 0, this.blockSize);
    }
    this.padding.unpad(src);
}

function AESKey (key)
{
    this.tmp = new ByteArray();
    this.state = new ByteArray();
    this.keyLength = key.size;
    this.key = new ByteArray();
    this.key.writeBytes(key);
    this.expandKey();
}

AESKey.prototype.Nb = 4;

AESKey.prototype.Sbox = [ // forward s-box
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16];

AESKey.prototype.InvSbox = [ // inverse s-box
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d];

AESKey.prototype.Xtime2Sbox = [ // combined Xtimes2[Sbox[]]
    0xc6, 0xf8, 0xee, 0xf6, 0xff, 0xd6, 0xde, 0x91, 0x60, 0x02, 0xce, 0x56, 0xe7, 0xb5, 0x4d, 0xec,
    0x8f, 0x1f, 0x89, 0xfa, 0xef, 0xb2, 0x8e, 0xfb, 0x41, 0xb3, 0x5f, 0x45, 0x23, 0x53, 0xe4, 0x9b,
    0x75, 0xe1, 0x3d, 0x4c, 0x6c, 0x7e, 0xf5, 0x83, 0x68, 0x51, 0xd1, 0xf9, 0xe2, 0xab, 0x62, 0x2a,
    0x08, 0x95, 0x46, 0x9d, 0x30, 0x37, 0x0a, 0x2f, 0x0e, 0x24, 0x1b, 0xdf, 0xcd, 0x4e, 0x7f, 0xea,
    0x12, 0x1d, 0x58, 0x34, 0x36, 0xdc, 0xb4, 0x5b, 0xa4, 0x76, 0xb7, 0x7d, 0x52, 0xdd, 0x5e, 0x13,
    0xa6, 0xb9, 0x00, 0xc1, 0x40, 0xe3, 0x79, 0xb6, 0xd4, 0x8d, 0x67, 0x72, 0x94, 0x98, 0xb0, 0x85,
    0xbb, 0xc5, 0x4f, 0xed, 0x86, 0x9a, 0x66, 0x11, 0x8a, 0xe9, 0x04, 0xfe, 0xa0, 0x78, 0x25, 0x4b,
    0xa2, 0x5d, 0x80, 0x05, 0x3f, 0x21, 0x70, 0xf1, 0x63, 0x77, 0xaf, 0x42, 0x20, 0xe5, 0xfd, 0xbf,
    0x81, 0x18, 0x26, 0xc3, 0xbe, 0x35, 0x88, 0x2e, 0x93, 0x55, 0xfc, 0x7a, 0xc8, 0xba, 0x32, 0xe6,
    0xc0, 0x19, 0x9e, 0xa3, 0x44, 0x54, 0x3b, 0x0b, 0x8c, 0xc7, 0x6b, 0x28, 0xa7, 0xbc, 0x16, 0xad,
    0xdb, 0x64, 0x74, 0x14, 0x92, 0x0c, 0x48, 0xb8, 0x9f, 0xbd, 0x43, 0xc4, 0x39, 0x31, 0xd3, 0xf2,
    0xd5, 0x8b, 0x6e, 0xda, 0x01, 0xb1, 0x9c, 0x49, 0xd8, 0xac, 0xf3, 0xcf, 0xca, 0xf4, 0x47, 0x10,
    0x6f, 0xf0, 0x4a, 0x5c, 0x38, 0x57, 0x73, 0x97, 0xcb, 0xa1, 0xe8, 0x3e, 0x96, 0x61, 0x0d, 0x0f,
    0xe0, 0x7c, 0x71, 0xcc, 0x90, 0x06, 0xf7, 0x1c, 0xc2, 0x6a, 0xae, 0x69, 0x17, 0x99, 0x3a, 0x27,
    0xd9, 0xeb, 0x2b, 0x22, 0xd2, 0xa9, 0x07, 0x33, 0x2d, 0x3c, 0x15, 0xc9, 0x87, 0xaa, 0x50, 0xa5,
    0x03, 0x59, 0x09, 0x1a, 0x65, 0xd7, 0x84, 0xd0, 0x82, 0x29, 0x5a, 0x1e, 0x7b, 0xa8, 0x6d, 0x2c];

AESKey.prototype.Xtime3Sbox = [ // combined Xtimes3[Sbox[]]
    0xa5, 0x84, 0x99, 0x8d, 0x0d, 0xbd, 0xb1, 0x54, 0x50, 0x03, 0xa9, 0x7d, 0x19, 0x62, 0xe6, 0x9a,
    0x45, 0x9d, 0x40, 0x87, 0x15, 0xeb, 0xc9, 0x0b, 0xec, 0x67, 0xfd, 0xea, 0xbf, 0xf7, 0x96, 0x5b,
    0xc2, 0x1c, 0xae, 0x6a, 0x5a, 0x41, 0x02, 0x4f, 0x5c, 0xf4, 0x34, 0x08, 0x93, 0x73, 0x53, 0x3f,
    0x0c, 0x52, 0x65, 0x5e, 0x28, 0xa1, 0x0f, 0xb5, 0x09, 0x36, 0x9b, 0x3d, 0x26, 0x69, 0xcd, 0x9f,
    0x1b, 0x9e, 0x74, 0x2e, 0x2d, 0xb2, 0xee, 0xfb, 0xf6, 0x4d, 0x61, 0xce, 0x7b, 0x3e, 0x71, 0x97,
    0xf5, 0x68, 0x00, 0x2c, 0x60, 0x1f, 0xc8, 0xed, 0xbe, 0x46, 0xd9, 0x4b, 0xde, 0xd4, 0xe8, 0x4a,
    0x6b, 0x2a, 0xe5, 0x16, 0xc5, 0xd7, 0x55, 0x94, 0xcf, 0x10, 0x06, 0x81, 0xf0, 0x44, 0xba, 0xe3,
    0xf3, 0xfe, 0xc0, 0x8a, 0xad, 0xbc, 0x48, 0x04, 0xdf, 0xc1, 0x75, 0x63, 0x30, 0x1a, 0x0e, 0x6d,
    0x4c, 0x14, 0x35, 0x2f, 0xe1, 0xa2, 0xcc, 0x39, 0x57, 0xf2, 0x82, 0x47, 0xac, 0xe7, 0x2b, 0x95,
    0xa0, 0x98, 0xd1, 0x7f, 0x66, 0x7e, 0xab, 0x83, 0xca, 0x29, 0xd3, 0x3c, 0x79, 0xe2, 0x1d, 0x76,
    0x3b, 0x56, 0x4e, 0x1e, 0xdb, 0x0a, 0x6c, 0xe4, 0x5d, 0x6e, 0xef, 0xa6, 0xa8, 0xa4, 0x37, 0x8b,
    0x32, 0x43, 0x59, 0xb7, 0x8c, 0x64, 0xd2, 0xe0, 0xb4, 0xfa, 0x07, 0x25, 0xaf, 0x8e, 0xe9, 0x18,
    0xd5, 0x88, 0x6f, 0x72, 0x24, 0xf1, 0xc7, 0x51, 0x23, 0x7c, 0x9c, 0x21, 0xdd, 0xdc, 0x86, 0x85,
    0x90, 0x42, 0xc4, 0xaa, 0xd8, 0x05, 0x01, 0x12, 0xa3, 0x5f, 0xf9, 0xd0, 0x91, 0x58, 0x27, 0xb9,
    0x38, 0x13, 0xb3, 0x33, 0xbb, 0x70, 0x89, 0xa7, 0xb6, 0x22, 0x92, 0x20, 0x49, 0xff, 0x78, 0x7a,
    0x8f, 0xf8, 0x80, 0x17, 0xda, 0x31, 0xc6, 0xb8, 0xc3, 0xb0, 0x77, 0x11, 0xcb, 0xfc, 0xd6, 0x3a];

AESKey.prototype.Xtime9 = [
    0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f, 0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
    0x90, 0x99, 0x82, 0x8b, 0xb4, 0xbd, 0xa6, 0xaf, 0xd8, 0xd1, 0xca, 0xc3, 0xfc, 0xf5, 0xee, 0xe7,
    0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04, 0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
    0xab, 0xa2, 0xb9, 0xb0, 0x8f, 0x86, 0x9d, 0x94, 0xe3, 0xea, 0xf1, 0xf8, 0xc7, 0xce, 0xd5, 0xdc,
    0x76, 0x7f, 0x64, 0x6d, 0x52, 0x5b, 0x40, 0x49, 0x3e, 0x37, 0x2c, 0x25, 0x1a, 0x13, 0x08, 0x01,
    0xe6, 0xef, 0xf4, 0xfd, 0xc2, 0xcb, 0xd0, 0xd9, 0xae, 0xa7, 0xbc, 0xb5, 0x8a, 0x83, 0x98, 0x91,
    0x4d, 0x44, 0x5f, 0x56, 0x69, 0x60, 0x7b, 0x72, 0x05, 0x0c, 0x17, 0x1e, 0x21, 0x28, 0x33, 0x3a,
    0xdd, 0xd4, 0xcf, 0xc6, 0xf9, 0xf0, 0xeb, 0xe2, 0x95, 0x9c, 0x87, 0x8e, 0xb1, 0xb8, 0xa3, 0xaa,
    0xec, 0xe5, 0xfe, 0xf7, 0xc8, 0xc1, 0xda, 0xd3, 0xa4, 0xad, 0xb6, 0xbf, 0x80, 0x89, 0x92, 0x9b,
    0x7c, 0x75, 0x6e, 0x67, 0x58, 0x51, 0x4a, 0x43, 0x34, 0x3d, 0x26, 0x2f, 0x10, 0x19, 0x02, 0x0b,
    0xd7, 0xde, 0xc5, 0xcc, 0xf3, 0xfa, 0xe1, 0xe8, 0x9f, 0x96, 0x8d, 0x84, 0xbb, 0xb2, 0xa9, 0xa0,
    0x47, 0x4e, 0x55, 0x5c, 0x63, 0x6a, 0x71, 0x78, 0x0f, 0x06, 0x1d, 0x14, 0x2b, 0x22, 0x39, 0x30,
    0x9a, 0x93, 0x88, 0x81, 0xbe, 0xb7, 0xac, 0xa5, 0xd2, 0xdb, 0xc0, 0xc9, 0xf6, 0xff, 0xe4, 0xed,
    0x0a, 0x03, 0x18, 0x11, 0x2e, 0x27, 0x3c, 0x35, 0x42, 0x4b, 0x50, 0x59, 0x66, 0x6f, 0x74, 0x7d,
    0xa1, 0xa8, 0xb3, 0xba, 0x85, 0x8c, 0x97, 0x9e, 0xe9, 0xe0, 0xfb, 0xf2, 0xcd, 0xc4, 0xdf, 0xd6,
    0x31, 0x38, 0x23, 0x2a, 0x15, 0x1c, 0x07, 0x0e, 0x79, 0x70, 0x6b, 0x62, 0x5d, 0x54, 0x4f, 0x46];

AESKey.prototype.XtimeB = [
    0x00, 0x0b, 0x16, 0x1d, 0x2c, 0x27, 0x3a, 0x31, 0x58, 0x53, 0x4e, 0x45, 0x74, 0x7f, 0x62, 0x69,
    0xb0, 0xbb, 0xa6, 0xad, 0x9c, 0x97, 0x8a, 0x81, 0xe8, 0xe3, 0xfe, 0xf5, 0xc4, 0xcf, 0xd2, 0xd9,
    0x7b, 0x70, 0x6d, 0x66, 0x57, 0x5c, 0x41, 0x4a, 0x23, 0x28, 0x35, 0x3e, 0x0f, 0x04, 0x19, 0x12,
    0xcb, 0xc0, 0xdd, 0xd6, 0xe7, 0xec, 0xf1, 0xfa, 0x93, 0x98, 0x85, 0x8e, 0xbf, 0xb4, 0xa9, 0xa2,
    0xf6, 0xfd, 0xe0, 0xeb, 0xda, 0xd1, 0xcc, 0xc7, 0xae, 0xa5, 0xb8, 0xb3, 0x82, 0x89, 0x94, 0x9f,
    0x46, 0x4d, 0x50, 0x5b, 0x6a, 0x61, 0x7c, 0x77, 0x1e, 0x15, 0x08, 0x03, 0x32, 0x39, 0x24, 0x2f,
    0x8d, 0x86, 0x9b, 0x90, 0xa1, 0xaa, 0xb7, 0xbc, 0xd5, 0xde, 0xc3, 0xc8, 0xf9, 0xf2, 0xef, 0xe4,
    0x3d, 0x36, 0x2b, 0x20, 0x11, 0x1a, 0x07, 0x0c, 0x65, 0x6e, 0x73, 0x78, 0x49, 0x42, 0x5f, 0x54,
    0xf7, 0xfc, 0xe1, 0xea, 0xdb, 0xd0, 0xcd, 0xc6, 0xaf, 0xa4, 0xb9, 0xb2, 0x83, 0x88, 0x95, 0x9e,
    0x47, 0x4c, 0x51, 0x5a, 0x6b, 0x60, 0x7d, 0x76, 0x1f, 0x14, 0x09, 0x02, 0x33, 0x38, 0x25, 0x2e,
    0x8c, 0x87, 0x9a, 0x91, 0xa0, 0xab, 0xb6, 0xbd, 0xd4, 0xdf, 0xc2, 0xc9, 0xf8, 0xf3, 0xee, 0xe5,
    0x3c, 0x37, 0x2a, 0x21, 0x10, 0x1b, 0x06, 0x0d, 0x64, 0x6f, 0x72, 0x79, 0x48, 0x43, 0x5e, 0x55,
    0x01, 0x0a, 0x17, 0x1c, 0x2d, 0x26, 0x3b, 0x30, 0x59, 0x52, 0x4f, 0x44, 0x75, 0x7e, 0x63, 0x68,
    0xb1, 0xba, 0xa7, 0xac, 0x9d, 0x96, 0x8b, 0x80, 0xe9, 0xe2, 0xff, 0xf4, 0xc5, 0xce, 0xd3, 0xd8,
    0x7a, 0x71, 0x6c, 0x67, 0x56, 0x5d, 0x40, 0x4b, 0x22, 0x29, 0x34, 0x3f, 0x0e, 0x05, 0x18, 0x13,
    0xca, 0xc1, 0xdc, 0xd7, 0xe6, 0xed, 0xf0, 0xfb, 0x92, 0x99, 0x84, 0x8f, 0xbe, 0xb5, 0xa8, 0xa3];

AESKey.prototype.XtimeD = [
    0x00, 0x0d, 0x1a, 0x17, 0x34, 0x39, 0x2e, 0x23, 0x68, 0x65, 0x72, 0x7f, 0x5c, 0x51, 0x46, 0x4b,
    0xd0, 0xdd, 0xca, 0xc7, 0xe4, 0xe9, 0xfe, 0xf3, 0xb8, 0xb5, 0xa2, 0xaf, 0x8c, 0x81, 0x96, 0x9b,
    0xbb, 0xb6, 0xa1, 0xac, 0x8f, 0x82, 0x95, 0x98, 0xd3, 0xde, 0xc9, 0xc4, 0xe7, 0xea, 0xfd, 0xf0,
    0x6b, 0x66, 0x71, 0x7c, 0x5f, 0x52, 0x45, 0x48, 0x03, 0x0e, 0x19, 0x14, 0x37, 0x3a, 0x2d, 0x20,
    0x6d, 0x60, 0x77, 0x7a, 0x59, 0x54, 0x43, 0x4e, 0x05, 0x08, 0x1f, 0x12, 0x31, 0x3c, 0x2b, 0x26,
    0xbd, 0xb0, 0xa7, 0xaa, 0x89, 0x84, 0x93, 0x9e, 0xd5, 0xd8, 0xcf, 0xc2, 0xe1, 0xec, 0xfb, 0xf6,
    0xd6, 0xdb, 0xcc, 0xc1, 0xe2, 0xef, 0xf8, 0xf5, 0xbe, 0xb3, 0xa4, 0xa9, 0x8a, 0x87, 0x90, 0x9d,
    0x06, 0x0b, 0x1c, 0x11, 0x32, 0x3f, 0x28, 0x25, 0x6e, 0x63, 0x74, 0x79, 0x5a, 0x57, 0x40, 0x4d,
    0xda, 0xd7, 0xc0, 0xcd, 0xee, 0xe3, 0xf4, 0xf9, 0xb2, 0xbf, 0xa8, 0xa5, 0x86, 0x8b, 0x9c, 0x91,
    0x0a, 0x07, 0x10, 0x1d, 0x3e, 0x33, 0x24, 0x29, 0x62, 0x6f, 0x78, 0x75, 0x56, 0x5b, 0x4c, 0x41,
    0x61, 0x6c, 0x7b, 0x76, 0x55, 0x58, 0x4f, 0x42, 0x09, 0x04, 0x13, 0x1e, 0x3d, 0x30, 0x27, 0x2a,
    0xb1, 0xbc, 0xab, 0xa6, 0x85, 0x88, 0x9f, 0x92, 0xd9, 0xd4, 0xc3, 0xce, 0xed, 0xe0, 0xf7, 0xfa,
    0xb7, 0xba, 0xad, 0xa0, 0x83, 0x8e, 0x99, 0x94, 0xdf, 0xd2, 0xc5, 0xc8, 0xeb, 0xe6, 0xf1, 0xfc,
    0x67, 0x6a, 0x7d, 0x70, 0x53, 0x5e, 0x49, 0x44, 0x0f, 0x02, 0x15, 0x18, 0x3b, 0x36, 0x21, 0x2c,
    0x0c, 0x01, 0x16, 0x1b, 0x38, 0x35, 0x22, 0x2f, 0x64, 0x69, 0x7e, 0x73, 0x50, 0x5d, 0x4a, 0x47,
    0xdc, 0xd1, 0xc6, 0xcb, 0xe8, 0xe5, 0xf2, 0xff, 0xb4, 0xb9, 0xae, 0xa3, 0x80, 0x8d, 0x9a, 0x97];

AESKey.prototype.XtimeE = [
    0x00, 0x0e, 0x1c, 0x12, 0x38, 0x36, 0x24, 0x2a, 0x70, 0x7e, 0x6c, 0x62, 0x48, 0x46, 0x54, 0x5a,
    0xe0, 0xee, 0xfc, 0xf2, 0xd8, 0xd6, 0xc4, 0xca, 0x90, 0x9e, 0x8c, 0x82, 0xa8, 0xa6, 0xb4, 0xba,
    0xdb, 0xd5, 0xc7, 0xc9, 0xe3, 0xed, 0xff, 0xf1, 0xab, 0xa5, 0xb7, 0xb9, 0x93, 0x9d, 0x8f, 0x81,
    0x3b, 0x35, 0x27, 0x29, 0x03, 0x0d, 0x1f, 0x11, 0x4b, 0x45, 0x57, 0x59, 0x73, 0x7d, 0x6f, 0x61,
    0xad, 0xa3, 0xb1, 0xbf, 0x95, 0x9b, 0x89, 0x87, 0xdd, 0xd3, 0xc1, 0xcf, 0xe5, 0xeb, 0xf9, 0xf7,
    0x4d, 0x43, 0x51, 0x5f, 0x75, 0x7b, 0x69, 0x67, 0x3d, 0x33, 0x21, 0x2f, 0x05, 0x0b, 0x19, 0x17,
    0x76, 0x78, 0x6a, 0x64, 0x4e, 0x40, 0x52, 0x5c, 0x06, 0x08, 0x1a, 0x14, 0x3e, 0x30, 0x22, 0x2c,
    0x96, 0x98, 0x8a, 0x84, 0xae, 0xa0, 0xb2, 0xbc, 0xe6, 0xe8, 0xfa, 0xf4, 0xde, 0xd0, 0xc2, 0xcc,
    0x41, 0x4f, 0x5d, 0x53, 0x79, 0x77, 0x65, 0x6b, 0x31, 0x3f, 0x2d, 0x23, 0x09, 0x07, 0x15, 0x1b,
    0xa1, 0xaf, 0xbd, 0xb3, 0x99, 0x97, 0x85, 0x8b, 0xd1, 0xdf, 0xcd, 0xc3, 0xe9, 0xe7, 0xf5, 0xfb,
    0x9a, 0x94, 0x86, 0x88, 0xa2, 0xac, 0xbe, 0xb0, 0xea, 0xe4, 0xf6, 0xf8, 0xd2, 0xdc, 0xce, 0xc0,
    0x7a, 0x74, 0x66, 0x68, 0x42, 0x4c, 0x5e, 0x50, 0x0a, 0x04, 0x16, 0x18, 0x32, 0x3c, 0x2e, 0x20,
    0xec, 0xe2, 0xf0, 0xfe, 0xd4, 0xda, 0xc8, 0xc6, 0x9c, 0x92, 0x80, 0x8e, 0xa4, 0xaa, 0xb8, 0xb6,
    0x0c, 0x02, 0x10, 0x1e, 0x34, 0x3a, 0x28, 0x26, 0x7c, 0x72, 0x60, 0x6e, 0x44, 0x4a, 0x58, 0x56,
    0x37, 0x39, 0x2b, 0x25, 0x0f, 0x01, 0x13, 0x1d, 0x47, 0x49, 0x5b, 0x55, 0x7f, 0x71, 0x63, 0x6d,
    0xd7, 0xd9, 0xcb, 0xc5, 0xef, 0xe1, 0xf3, 0xfd, 0xa7, 0xa9, 0xbb, 0xb5, 0x9f, 0x91, 0x83, 0x8d];

AESKey.prototype.Rcon = [ 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36 ];

AESKey.prototype.expandKey = function ()
{
    var tmp0, tmp1, tmp2, tmp3, tmp4;
    var Nk = this.key.size / 4;
    this.Nr = Nk + 6;

    for (var idx = Nk; idx < this.Nb * (this.Nr + 1); idx++) {
        tmp0 = this.key.get(4*idx - 4);
        tmp1 = this.key.get(4*idx - 3);
        tmp2 = this.key.get(4*idx - 2);
        tmp3 = this.key.get(4*idx - 1);
        if (!(idx % Nk)) {
            tmp4 = tmp3;
            tmp3 = this.Sbox[tmp0];
            tmp0 = this.Sbox[tmp1] ^ this.Rcon[idx / Nk];
            tmp1 = this.Sbox[tmp2];
            tmp2 = this.Sbox[tmp4];

        } else if (Nk > 6 && idx % Nk == 4) {
            tmp0 = this.Sbox[tmp0];
            tmp1 = this.Sbox[tmp1];
            tmp2 = this.Sbox[tmp2];
            tmp3 = this.Sbox[tmp3];
        }
        this.key.set(4*idx + 0, this.key.get(4*idx - 4*Nk + 0) ^ tmp0);
        this.key.set(4*idx + 1, this.key.get(4*idx - 4*Nk + 1) ^ tmp1);
        this.key.set(4*idx + 2, this.key.get(4*idx - 4*Nk + 2) ^ tmp2);
        this.key.set(4*idx + 3, this.key.get(4*idx - 4*Nk + 3) ^ tmp3);
    }
}

AESKey.prototype.getBlockSize = function ()
{
    return 16;
}

AESKey.prototype.encrypt = function (block, index)
{
    this.state.position = 0;
    this.state.writeBytes(block, index, this.Nb * 4);

    this.addRoundKey(this.key, 0);
    for (var round = 1; round < this.Nr + 1; round++) {
        if (round < this.Nr) {
            this.mixSubColumns();
        } else {
            this.shiftRows();
        }
        this.addRoundKey(this.key, round * this.Nb * 4);
    }

    block.position = index;
    block.writeBytes(this.state);
}

AESKey.prototype.decrypt = function (block, index)
{
    this.state.position = 0;
    this.state.writeBytes(block, index, this.Nb * 4);

    this.addRoundKey(this.key, this.Nr * this.Nb * 4);
    this.invShiftRows();
    for (var round = this.Nr; round--; ) {
        this.addRoundKey(this.key, round * this.Nb * 4);
        if (round) {
            this.invMixSubColumns();
        }
    }

    block.position = index;
    block.writeBytes(this.state);
}

AESKey.prototype.addRoundKey = function (key, offset)
{
    for (var idx = 0; idx < 16; idx++) {
        this.state.set(idx, this.state.get(idx) ^ key.get(idx + offset));
    }
}

AESKey.prototype.shiftRows = function ()
{
    // just substitute row 0
    this.state.set(0, this.Sbox[this.state.get(0)]);
    this.state.set(4, this.Sbox[this.state.get(4)]);
    this.state.set(8, this.Sbox[this.state.get(8)]);
    this.state.set(12, this.Sbox[this.state.get(12)]);

    // rotate row 1
    var tmp = this.Sbox[this.state.get(1)];
    this.state.set(1, this.Sbox[this.state.get(5)]);
    this.state.set(5, this.Sbox[this.state.get(9)]);
    this.state.set(9, this.Sbox[this.state.get(13)]);
    this.state.set(13, tmp);

    // rotate row 2
    tmp = this.Sbox[this.state.get(2)];
    this.state.set(2, this.Sbox[this.state.get(10)]);
    this.state.set(10, tmp);
    tmp = this.Sbox[this.state.get(6)];
    this.state.set(6, this.Sbox[this.state.get(14)]);
    this.state.set(14, tmp);

    // rotate row 3
    tmp = this.Sbox[this.state.get(15)];
    this.state.set(15, this.Sbox[this.state.get(11)]);
    this.state.set(11, this.Sbox[this.state.get(7)]);
    this.state.set(7, this.Sbox[this.state.get(3)]);
    this.state.set(3, tmp);
}

AESKey.prototype.invShiftRows = function ()
{
    // restore row 0
    this.state.set(0, this.InvSbox[this.state.get(0)]);
    this.state.set(4, this.InvSbox[this.state.get(4)]);
    this.state.set(8, this.InvSbox[this.state.get(8)]);
    this.state.set(12, this.InvSbox[this.state.get(12)]);

    // restore row 1
    var tmp = this.InvSbox[this.state.get(13)];
    this.state.set(13, this.InvSbox[this.state.get(9)]);
    this.state.set(9, this.InvSbox[this.state.get(5)]);
    this.state.set(5, this.InvSbox[this.state.get(1)]);
    this.state.set(1, tmp);

    // restore row 2
    tmp = this.InvSbox[this.state.get(2)];
    this.state.set(2, this.InvSbox[this.state.get(10)]);
    this.state.set(10, tmp);
    tmp = this.InvSbox[this.state.get(6)];
    this.state.set(6, this.InvSbox[this.state.get(14)]);
    this.state.set(14, tmp);

    // restore row 3
    tmp = this.InvSbox[this.state.get(3)];
    this.state.set(3, this.InvSbox[this.state.get(7)]);
    this.state.set(7, this.InvSbox[this.state.get(11)]);
    this.state.set(11, this.InvSbox[this.state.get(15)]);
    this.state.set(15, tmp);
}

AESKey.prototype.mixSubColumns = function ()
{
    this.tmp.size = 0;

    // mixing column 0
    this.tmp.set(0, this.Xtime2Sbox[this.state.get(0)] ^ this.Xtime3Sbox[this.state.get(5)] ^
        this.Sbox[this.state.get(10)] ^ this.Sbox[this.state.get(15)]);
    this.tmp.set(1, this.Sbox[this.state.get(0)] ^ this.Xtime2Sbox[this.state.get(5)] ^
        this.Xtime3Sbox[this.state.get(10)] ^ this.Sbox[this.state.get(15)]);
    this.tmp.set(2, this.Sbox[this.state.get(0)] ^ this.Sbox[this.state.get(5)] ^
        this.Xtime2Sbox[this.state.get(10)] ^ this.Xtime3Sbox[this.state.get(15)]);
    this.tmp.set(3, this.Xtime3Sbox[this.state.get(0)] ^ this.Sbox[this.state.get(5)] ^
        this.Sbox[this.state.get(10)] ^ this.Xtime2Sbox[this.state.get(15)]);

    // mixing column 1
    this.tmp.set(4, this.Xtime2Sbox[this.state.get(4)] ^ this.Xtime3Sbox[this.state.get(9)] ^
        this.Sbox[this.state.get(14)] ^ this.Sbox[this.state.get(3)]);
    this.tmp.set(5, this.Sbox[this.state.get(4)] ^ this.Xtime2Sbox[this.state.get(9)] ^
        this.Xtime3Sbox[this.state.get(14)] ^ this.Sbox[this.state.get(3)]);
    this.tmp.set(6, this.Sbox[this.state.get(4)] ^ this.Sbox[this.state.get(9)] ^
        this.Xtime2Sbox[this.state.get(14)] ^ this.Xtime3Sbox[this.state.get(3)]);
    this.tmp.set(7, this.Xtime3Sbox[this.state.get(4)] ^ this.Sbox[this.state.get(9)] ^
        this.Sbox[this.state.get(14)] ^ this.Xtime2Sbox[this.state.get(3)]);

    // mixing column 2
    this.tmp.set(8, this.Xtime2Sbox[this.state.get(8)] ^ this.Xtime3Sbox[this.state.get(13)] ^
        this.Sbox[this.state.get(2)] ^ this.Sbox[this.state.get(7)]);
    this.tmp.set(9, this.Sbox[this.state.get(8)] ^ this.Xtime2Sbox[this.state.get(13)] ^
        this.Xtime3Sbox[this.state.get(2)] ^ this.Sbox[this.state.get(7)]);
    this.tmp.set(10, this.Sbox[this.state.get(8)] ^ this.Sbox[this.state.get(13)] ^
        this.Xtime2Sbox[this.state.get(2)] ^ this.Xtime3Sbox[this.state.get(7)]);
    this.tmp.set(11, this.Xtime3Sbox[this.state.get(8)] ^ this.Sbox[this.state.get(13)] ^
        this.Sbox[this.state.get(2)] ^ this.Xtime2Sbox[this.state.get(7)]);

    // mixing column 3
    this.tmp.set(12, this.Xtime2Sbox[this.state.get(12)] ^ this.Xtime3Sbox[this.state.get(1)] ^
        this.Sbox[this.state.get(6)] ^ this.Sbox[this.state.get(11)]);
    this.tmp.set(13, this.Sbox[this.state.get(12)] ^ this.Xtime2Sbox[this.state.get(1)] ^
        this.Xtime3Sbox[this.state.get(6)] ^ this.Sbox[this.state.get(11)]);
    this.tmp.set(14, this.Sbox[this.state.get(12)] ^ this.Sbox[this.state.get(1)] ^
        this.Xtime2Sbox[this.state.get(6)] ^ this.Xtime3Sbox[this.state.get(11)]);
    this.tmp.set(15, this.Xtime3Sbox[this.state.get(12)] ^ this.Sbox[this.state.get(1)] ^
        this.Sbox[this.state.get(6)] ^ this.Xtime2Sbox[this.state.get(11)]);

    this.state.position = 0;
    this.state.writeBytes(this.tmp, 0, this.Nb * 4);
}

AESKey.prototype.invMixSubColumns = function ()
{
	this.tmp.size = 0;

	// restore column 0
    this.tmp.set(0, this.XtimeE[this.state.get(0)] ^ this.XtimeB[this.state.get(1)] ^
        this.XtimeD[this.state.get(2)] ^ this.Xtime9[this.state.get(3)]);
    this.tmp.set(5, this.Xtime9[this.state.get(0)] ^ this.XtimeE[this.state.get(1)] ^
        this.XtimeB[this.state.get(2)] ^ this.XtimeD[this.state.get(3)]);
    this.tmp.set(10, this.XtimeD[this.state.get(0)] ^ this.Xtime9[this.state.get(1)] ^
        this.XtimeE[this.state.get(2)] ^ this.XtimeB[this.state.get(3)]);
    this.tmp.set(15, this.XtimeB[this.state.get(0)] ^ this.XtimeD[this.state.get(1)] ^
        this.Xtime9[this.state.get(2)] ^ this.XtimeE[this.state.get(3)]);

	// restore column 1
    this.tmp.set(4, this.XtimeE[this.state.get(4)] ^ this.XtimeB[this.state.get(5)] ^
        this.XtimeD[this.state.get(6)] ^ this.Xtime9[this.state.get(7)]);
    this.tmp.set(9, this.Xtime9[this.state.get(4)] ^ this.XtimeE[this.state.get(5)] ^
        this.XtimeB[this.state.get(6)] ^ this.XtimeD[this.state.get(7)]);
    this.tmp.set(14, this.XtimeD[this.state.get(4)] ^ this.Xtime9[this.state.get(5)] ^
        this.XtimeE[this.state.get(6)] ^ this.XtimeB[this.state.get(7)]);
    this.tmp.set(3, this.XtimeB[this.state.get(4)] ^ this.XtimeD[this.state.get(5)] ^
        this.Xtime9[this.state.get(6)] ^ this.XtimeE[this.state.get(7)]);

	// restore column 2
    this.tmp.set(8, this.XtimeE[this.state.get(8)] ^ this.XtimeB[this.state.get(9)] ^
        this.XtimeD[this.state.get(10)] ^ this.Xtime9[this.state.get(11)]);
    this.tmp.set(13, this.Xtime9[this.state.get(8)] ^ this.XtimeE[this.state.get(9)] ^
        this.XtimeB[this.state.get(10)] ^ this.XtimeD[this.state.get(11)]);
    this.tmp.set(2, this.XtimeD[this.state.get(8)] ^ this.Xtime9[this.state.get(9)] ^
        this.XtimeE[this.state.get(10)] ^ this.XtimeB[this.state.get(11)]);
    this.tmp.set(7, this.XtimeB[this.state.get(8)] ^ this.XtimeD[this.state.get(9)] ^
        this.Xtime9[this.state.get(10)] ^ this.XtimeE[this.state.get(11)]);

	// restore column 3
    this.tmp.set(12, this.XtimeE[this.state.get(12)] ^ this.XtimeB[this.state.get(13)] ^
        this.XtimeD[this.state.get(14)] ^ this.Xtime9[this.state.get(15)]);
    this.tmp.set(1, this.Xtime9[this.state.get(12)] ^ this.XtimeE[this.state.get(13)] ^
        this.XtimeB[this.state.get(14)] ^ this.XtimeD[this.state.get(15)]);
    this.tmp.set(6, this.XtimeD[this.state.get(12)] ^ this.Xtime9[this.state.get(13)] ^
        this.XtimeE[this.state.get(14)] ^ this.XtimeB[this.state.get(15)]);
    this.tmp.set(11, this.XtimeB[this.state.get(12)] ^ this.XtimeD[this.state.get(13)] ^
        this.Xtime9[this.state.get(14)] ^ this.XtimeE[this.state.get(15)]);

	for (var ii = 0; ii < 4 * this.Nb; ii++) {
	    this.state.set(ii, this.InvSbox[this.tmp.get(ii)]);
	}
}

function ClassicReduction (m)
{
    this.m = m;
}

ClassicReduction.prototype.convert = function (x)
{
    return (x.s < 0 || x.compareTo(this.m) >= 0) ? x.mod(this.m) : x;
}

ClassicReduction.prototype.sqrTo = function (x, r)
{
    x.squareTo(r);
    this.reduce(r);
}

ClassicReduction.prototype.mulTo = function (x, y, r)
{
    x.multiplyTo(y, r);
    this.reduce(r);
}

ClassicReduction.prototype.revert = function (x)
{
    return x;
}

ClassicReduction.prototype.reduce = function (x)
{
    x.divRemTo(this.m, null, x);
}

function MontgomeryReduction (m)
{
    this.m = m;
    this.mp = m.invDigit();
    this.mpl = this.mp & 0x7fff;
    this.mph = this.mp >> 15;
    this.um = (1<<(BigInteger.prototype.DB-15)) - 1;
    this.mt2 = 2 * m.t;
}

MontgomeryReduction.prototype.convert = function (x)
{
    var r = new BigInteger();
    x.abs().dlShiftTo(this.m.t, r);
    r.divRemTo(this.m, null, r);
    if (x.s < 0 && r.compareTo(BigInteger.prototype.ZERO) > 0) {
        this.m.subTo(r, r);
    }
    return r;
}

MontgomeryReduction.prototype.sqrTo = function (x, r)
{
    x.squareTo(r);
    this.reduce(r);
}

MontgomeryReduction.prototype.mulTo = function (x, y, r)
{
    x.multiplyTo(y, r);
    this.reduce(r);
}

MontgomeryReduction.prototype.revert = function (x)
{
    var r = new BigInteger();
    x.copyTo(r);
    this.reduce(r);
    return r;
}

MontgomeryReduction.prototype.reduce = function (x)
{
    while (x.t <= this.mt2) { // pad x so am has enough room later
        x.a[x.t++] = 0;
    }
    for (var ii = 0; ii < this.m.t; ii++) {
        // faster way of calculating u0 = x[i]*mp mod DV
        var j = x.a[ii] & 0x7fff;
        var u0 = (j*this.mpl+(((j*this.mph+(x.a[ii]>>15)*this.mpl)&this.um)<<15))&
            BigInteger.prototype.DM;
        // use am to combine the multiply-shift-add into one call
        j = ii+this.m.t;
        x.a[j] += this.m.am(0, u0, x, ii, 0, this.m.t);
        // propagate carry
        while (x.a[j] >= BigInteger.prototype.DV) {
            x.a[j] -= BigInteger.prototype.DV;
            x.a[++j]++;
        }
    }
    x.clamp();
    x.drShiftTo(this.m.t, x);
    if (x.compareTo(this.m) >= 0) {
        x.subTo(this.m, x);
    }
}

function BigInteger (value)
{
    this.a = new Array();
    if (value != null) {
        this.fromArray(value);
    }
}

function nbv (value)
{
    var bn = new BigInteger();
    bn.fromInt(value);
    return bn;
}

BigInteger.prototype.fromInt = function (value)
{
    this.t = 1;
    this.s = (value < 0) ? -1 : 0;
    if (value > 0) {
        this.a[0] = value;
    } else if (value < -1) {
        this.a[0] = value + this.DV;
    } else {
        this.t = 0;
    }
}

BigInteger.prototype.DB = 30; // number of significant bits per chunk
BigInteger.prototype.DV = (1 << BigInteger.prototype.DB);
BigInteger.prototype.DM = (BigInteger.prototype.DV-1); // Max value in a chunk
BigInteger.prototype.ZERO = nbv(0);
BigInteger.prototype.ONE = nbv(1);
BigInteger.prototype.BI_FP = 52;
BigInteger.prototype.FV = Math.pow(2, BigInteger.prototype.BI_FP);
BigInteger.prototype.F1 = BigInteger.prototype.BI_FP - BigInteger.prototype.DB;
BigInteger.prototype.F2 = 2*BigInteger.prototype.DB - BigInteger.prototype.BI_FP;

BigInteger.prototype.fromArray = function (value)
{
    var sh = 0;
    this.t = 0;
    this.s = 0;
    for (var ii = value.size - 1; ii >= 0; ii--) {
        var x = value.get(ii);
        if (sh == 0) {
            this.a[this.t++] = x;
        } else if (sh + 8 > this.DB) {
            this.a[this.t - 1] |= (x&((1<<(this.DB-sh))-1))<<sh;
            this.a[this.t++] = x >> (this.DB - sh);
        } else {
            this.a[this.t - 1] |= x << sh;
        }
        sh += 8;
        if (sh >= this.DB) {
            sh -= this.DB;
        }
    }
    this.clamp();
}

BigInteger.prototype.toArray = function (array)
{
    var km = (1<<8)-1;
    var d = 0;
    var i = this.t;
    var p = this.DB - (i*this.DB)%8;
    var m = false;
    var c = 0;
    if (i-- > 0) {
        if (p < this.DB && (d=this.a[i]>>p)>0) {
            m = true;
            array.writeByte(d);
            c++;
        }
        while (i >= 0) {
            if (p < 8) {
                d = (this.a[i]&((1<<p)-1))<<(8-p);
                d |= this.a[--i]>>(p+=this.DB-8);
            } else {
                d = (this.a[i]>>(p-=8))&km;
                if (p <= 0) {
                    p += this.DB;
                    --i;
                }
            }
            if (d > 0) {
                m = true;
            }
            if (m) {
                array.writeByte(d);
                c++;
            }
        }
    }
    return c;
}

BigInteger.prototype.modPowInt = function (e, m)
{
    var z = (e < 256 || m.isEven()) ? new ClassicReduction(m) : new MontgomeryReduction(m);
    return this.exp(e, z);
}

BigInteger.prototype.isEven = function ()
{
    return ((this.t>0)?(this.a[0]&1):this.s) == 0;
}

BigInteger.prototype.exp = function (e, z)
{
    if (e > 0xffffffff || e < 1) {
        return this.ONE;
    }
    var r = new BigInteger();
    var r2 = new BigInteger();
    var g = z.convert(this);
    var i = this.nbits(e) - 1;
    g.copyTo(r);
    while (--i >= 0) {
        z.sqrTo(r, r2);
        if ((e&(1<<i))>0) {
            z.mulTo(r2,g,r);
        } else {
            var t = r;
            r = r2;
            r2 = t;
        }
    }
    return z.revert(r);
}

BigInteger.prototype.nbits = function (x)
{
    var r = 1;
    var t;
    if ((t=x>>>16) != 0) { x = t; r += 16; }
    if ((t=x>>8) != 0) { x = t; r += 8; }
    if ((t=x>>4) != 0) { x = t; r += 4; }
    if ((t=x>>2) != 0) { x = t; r += 2; }
    if ((t=x>>1) != 0) { x = t; r += 1; }
    return r;
}

BigInteger.prototype.copyTo = function (r)
{
    for (var ii = this.t - 1; ii >= 0; ii--) {
        r.a[ii] = this.a[ii];
    }
    r.t = this.t;
    r.s = this.s;
}

BigInteger.prototype.mod = function (v)
{
    var r = new BigInteger();
    this.abs().divRemTo(v, null, r);
    if (this.s < 0 && r.compareTo(this.ZERO) > 0) {
        v.subTo(r, r);
    }
    return r;
}

BigInteger.prototype.squareTo = function (r)
{
    var x = this.abs();
    var i = r.t = 2*x.t;
    while (--i >= 0) {
        r.a[i] = 0;
    }
    for (i = 0; i < x.t - 1; i++) {
        var c = x.am(i, x.a[i], r, 2*i, 0, 1);
        if ((r.a[i+x.t] += x.am(i+1, 2*x.a[i], r, 2*i+1, c, x.t-i-1)) >= this.DV) {
            r.a[i+x.t] -= this.DV;
            r.a[i+x.t+1] = 1;
        }
    }
    if (r.t > 0) {
        r.a[r.t-1] += x.am(i, x.a[i], r, 2*i, 0, 1);
    }
    r.s = 0;
    r.clamp();
}

BigInteger.prototype.divRemTo = function (m, q, r)
{
    var pm = m.abs();
    if (pm.t <= 0) {
        return;
    }
    var pt = this.abs();
    if (pt.t < pm.t) {
        if (q != null) {
            q.fromInt(0);
        }
        if (r != null) {
            this.copyTo(r);
        }
        return;
    }
    if (r == null) {
        r = new BigInteger();
    }
    var y = new BigInteger();
    var ts = this.s;
    var ms = m.s;
    var nsh = this.DB - this.nbits(pm.a[pm.t-1]); // normalize modulus
    if (nsh > 0) {
        pm.lShiftTo(nsh, y);
        pt.lShiftTo(nsh, r);
    } else {
        pm.copyTo(y);
        pt.copyTo(r);
    }
    var ys = y.t;
    var y0 = y.a[ys-1];
    if (y0==0) {
        return;
    }
    var yt = y0*(1<<this.F1)+((ys>1)?y.a[ys-2]>>this.F2:0);
    var d1 = this.FV/yt;
    var d2 = (1<<this.F1)/yt;
    var e = 1<<this.F2;
    var i = r.t;
    var j = i-ys;
    var t = (q == null) ? new BigInteger() : q;
    y.dlShiftTo(j, t);
    t.subTo(y, y); // "negative" y so we can replace sub with am later.
    while (y.t < ys) {
        y.a[y.t++] = 0;
    }
    while (--j >= 0) {
        // Estimate quotient digit
        var qd = Math.floor((r.a[--i]==y0)?this.DM:Number(r.a[i])*d1+(Number(r.a[i-1])+e)*d2);
        if ((r.a[i]+= y.am(0, qd, r, j, 0, ys))<qd) { // Try it out
            y.dlShiftTo(j, t);
            r.subTo(t,r);
            while (r.a[i]<--qd) {
                r.subTo(t,r);
            }
        }
    }
    if (q != null) {
        r.drShiftTo(ys, q);
        if (ts != ms) {
            this.ZERO.subTo(q, q);
        }
    }
    r.t = ys;
    r.clamp();
    if (nsh > 0) {
        r.rShiftTo(nsh, r); // Denormalize remainder
    }
    if (ts < 0) {
        ZERO.subTo(r, r);
    }
}

BigInteger.prototype.invDigit = function ()
{
    if (this.t < 1) {
        return 0;
    }
    var x = this.a[0];
    if ((x&1)==0) {
        return 0;
    }
    var y = x&3;                                // y == 1/x mod 2^2
    y = (y*(2-(x&0xf )*y))             &0xf;    // y == 1/x mod 2^4
    y = (y*(2-(x&0xff)*y))             &0xff;   // y == 1/x mod 2^8
    y = (y*(2-(((x&0xffff)*y)&0xffff)))&0xffff;	// y == 1/x mod 2^16
    // last step - calculate inverse mod DV directly;
    // assumes 16 < DB <= 32 and assumes ability to handle 48-bit ints
    // XXX 48 bit ints? Whaaaa? is there an implicit float conversion in here?
    y = (y*(2-x*y%this.DV))%this.DV;  // y == 1/x mod 2^dbits
    // we really want the negative inverse, and -DV < y < DV
    return (y>0)?this.DV-y:-y;
}

BigInteger.prototype.abs = function ()
{
    return (this.s < 0) ? this.negate() : this;
}

BigInteger.prototype.dlShiftTo = function (n, r)
{
    for (var ii = this.t - 1; ii >= 0; ii--) {
        r.a[ii + n] = this.a[ii];
    }
    for (var ii = n - 1; ii >= 0; ii--) {
        r.a[ii] = 0;
    }
    r.t = this.t + n;
    r.s = this.s;
}

BigInteger.prototype.compareTo = function (v)
{
    var r = this.s - v.s;
    if (r != 0) {
        return r;
    }
    var i = this.t;
    r = i - v.t;
    if (r != 0) {
        return r;
    }
    while (--i >= 0) {
        r = this.a[i] - v.a[i];
        if (r != 0) {
            return r;
        }
    }
    return 0;
}

BigInteger.prototype.subTo = function (v, r)
{
    var i = 0;
    var c = 0;
    var m = Math.min(v.t, this.t);
    while (i < m) {
        c += this.a[i] - v.a[i];
        r.a[i++] = c & this.DM;
        c >>= this.DB;
    }
    if (v.t < this.t) {
        c -= v.s;
        while (i < this.t) {
            c += this.a[i];
            r.a[i++] = c&this.DM;
            c >>= this.DB;
        }
        c += this.s;

    } else {
        c += this.s;
        while (i < v.t) {
            c -= v.a[i];
            r.a[i++] = c&this.DM;
            c >>= this.DB;
        }
        c -= v.s;
    }
    r.s = (c < 0) ? -1 : 0;
    if (c < -1) {
        r.a[i++] = this.DV+c;
    } else if (c > 0) {
        r.a[i++] = c;
    }
    r.t = i;
    r.clamp();
}

BigInteger.prototype.multiplyTo = function (v, r)
{
    var x = this.abs();
    var y = v.abs();
    var i = x.t;
    r.t = i + y.t;
    while (--i >= 0) {
        r.a[i] = 0;
    }
    for (i = 0; i < y.t; ++i) {
        r.a[i+x.t] = x.am(0, y.a[i], r, i, 0, x.t);
    }
    r.s = 0;
    r.clamp();
    if (this.s != v.s) {
        this.ZERO.subTo(r, r);
    }
}

BigInteger.prototype.drShiftTo = function (n, r)
{
    for (var ii = n; ii < this.t; ii++) {
        r.a[ii - n] = this.a[ii];
    }
    r.t = Math.max(this.t - n, 0);
    r.s = this.s;
}

BigInteger.prototype.clamp = function ()
{
    var c = this.s & this.DM;
    while (this.t > 0 && this.a[this.t - 1] == c) {
        --this.t;
    }
}

BigInteger.prototype.am = function (i, x, w, j, c, n)
{
    var xl = x & 0x7fff;
    var xh = x >> 15;
    while (--n >= 0) {
        var l = this.a[i] & 0x7fff;
        var h = this.a[i++] >> 15;
        var m = xh*l + h*xl;
        l = xl*l + ((m&0x7fff)<<15)+w.a[j]+(c&0x3fffffff);
        c = (l>>>30)+(m>>>15)+xh*h+(c>>>30);
        w.a[j++] = l&0x3fffffff;
    }
    return c;
}

BigInteger.prototype.negate = function ()
{
    var r = new BigInteger();
    this.ZERO.subTo(this, r);
    return r;
}

BigInteger.prototype.lShiftTo = function (n, r)
{
    var bs = n % this.DB;
    var cbs = this.DB - bs;
    var bm = (1<<cbs)-1;
    var ds = Math.floor(n/this.DB);
    var c = (this.s<<bs)&this.DM;
    var i;
    for (i = this.t - 1; i >= 0; --i) {
        r.a[i+ds+1] = (this.a[i]>>cbs)|c;
        c = (this.a[i]&bm)<<bs;
    }
    for (i = ds - 1; i >= 0; --i) {
        r.a[i] = 0;
    }
    r.a[ds] = c;
    r.t = this.t+ds+1;
    r.s = this.s;
    r.clamp();
}

BigInteger.prototype.rShiftTo = function (n, r)
{
    r.s = this.s;
    var ds = Math.floor(n / this.DB);
    if (ds >= this.t) {
        r.t = 0;
        return;
    }
    var bs = n % this.DB;
    var cbs = this.DB - bs;
    var bm = (1<<bs)-1;
    r.a[0] = this.a[ds]>>bs;
    var i;
    for (i = ds + 1; i < this.t; ++i) {
        r.a[i-ds-1] |= (this.a[i]&bm)<<cbs;
        r.a[i-ds] = this.a[i]>>bs;
    }
    if (bs > 0) {
        r.a[this.t-ds-1] |= (this.s&bm)<<cbs;
    }
    r.t = this.t-ds;
    r.clamp();
}

BigInteger.prototype.bitLength = function ()
{
    if (this.t <= 0) {
        return 0;
    }
    return this.DB*(this.t-1)+this.nbits(this.a[this.t-1]^(this.s&this.DM));
}

RSAKey = function (N, E)
{
    var bytes = new ByteArray();
    for (var ii = 0; ii < N.length; ii += 2) {
        bytes.writeByte(parseInt(N.substr(ii, 2), 16));
    }
    bytes.position = 0;

    this.n = new BigInteger(bytes);
    this.e = parseInt(E, 16);
}

RSAKey.prototype.encrypt = function (src, dst, len)
{
    if (src.position >= src.size) {
        src.position = 0;
    }
    var bl = Math.floor((this.n.bitLength()+7)/8);
    var end = src.position + len;
    while (src.position < end) {
        var block = new BigInteger(this.pkcs1pad(src, end, bl));
        var chunk = block.modPowInt(this.e, this.n);
        chunk.toArray(dst);
    }
}

RSAKey.prototype.pkcs1pad = function (src, end, n)
{
    var out = new ByteArray();
    var p = src.position;
    end = Math.min(end, src.size, p + n - 11);
    src.position = end;
    var i = end - 1;
    while (i >= p && n > 11) {
        out.set(--n, src.get(i--));
    }
    out.set(--n, 0);
    var rng = new Random();
    while (n > 2) {
        var x = 0;
        while (x == 0) {
            x = rng.nextByte();
        }
        out.set(--n, x);
    }
    out.set(--n, 0x02);
    out.set(--n, 0);
    return out;
}
