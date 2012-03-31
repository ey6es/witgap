/**
 * RSAKey
 *
 * An ActionScript 3 implementation of RSA + PKCS#1 (light version)
 * Copyright (c) 2007 Henri Torgemane
 *
 * Derived from:
 * 		The jsbn library, Copyright (c) 2003-2005 Tom Wu
 *
 * See LICENSE.txt for full license information.
 */
package com.hurlant.crypto.rsa
{
	import com.hurlant.crypto.prng.Random;
	import com.hurlant.math.BigInteger;
	import com.hurlant.util.Memory;

	import flash.utils.ByteArray;

	/**
	 * Current limitations:
	 * exponent must be smaller than 2^31.
	 */
	public class RSAKey
	{
		// public key
		public var e:int;              // public exponent. must be <2^31
		public var n:BigInteger; // modulus
		// private key
		public var d:BigInteger;
		// extended private key
		public var p:BigInteger;
		public var q:BigInteger;
		public var dmp1:BigInteger
		public var dmq1:BigInteger;
		public var coeff:BigInteger;
		// flags. flags are cool.
		protected var canDecrypt:Boolean;
		protected var canEncrypt:Boolean;

		public function RSAKey(N:BigInteger, E:int,
			D:BigInteger=null,
			P:BigInteger = null, Q:BigInteger=null,
			DP:BigInteger=null, DQ:BigInteger=null,
			C:BigInteger=null) {

			this.n = N;
			this.e = E;
			this.d = D;
			this.p = P;
			this.q = Q;
			this.dmp1 = DP;
			this.dmq1 = DQ;
			this.coeff = C;
			// adjust a few flags.
			canEncrypt = (n!=null&&e!=0);
			canDecrypt = (canEncrypt&&d!=null);
		}

		public static function parsePublicKey(N:String, E:String):RSAKey {
			return new RSAKey(new BigInteger(N, 16), parseInt(E,16));
		}

		public function getBlockSize():uint {
			return (n.bitLength()+7)/8;
		}
		public function dispose():void {
			e = 0;
			n.dispose();
			n = null;
			Memory.gc();
		}

		public function encrypt(src:ByteArray, dst:ByteArray, length:uint, pad:Function=null):void {
			_encrypt(doPublic, src, dst, length, pad, 0x02);
		}

		private function _encrypt(op:Function, src:ByteArray, dst:ByteArray, length:uint, pad:Function, padType:int):void {
			// adjust pad if needed
			if (pad==null) pad = pkcs1pad;
			// convert src to BigInteger
			if (src.position >= src.length) {
				src.position = 0;
			}
			var bl:uint = getBlockSize();
			var end:int = src.position + length;
			while (src.position<end) {
				var block:BigInteger = new BigInteger(pad(src, end, bl, padType), bl);
				var chunk:BigInteger = op(block);
				chunk.toArray(dst);
			}
		}

		/**
		 * PKCS#1 pad. type 1 (0xff) or 2, random.
		 * puts as much data from src into it, leaves what doesn't fit alone.
		 */
		private function pkcs1pad(src:ByteArray, end:int, n:uint, type:uint = 0x02):ByteArray {
			var out:ByteArray = new ByteArray;
			var p:uint = src.position;
			end = Math.min(end, src.length, p+n-11);
			src.position = end;
			var i:int = end-1;
			while (i>=p && n>11) {
				out[--n] = src[i--];
			}
			out[--n] = 0;
			var rng:Random = new Random;
			while (n>2) {
				var x:int = 0;
				while (x==0) x = (type==0x02)?rng.nextByte():0xFF;
				out[--n] = x;
			}
			out[--n] = type;
			out[--n] = 0;
			return out;
		}

		public function toString():String {
			return "rsa";
		}

		public function dump():String {
			var s:String= "N="+n.toString(16)+"\n"+
			"E="+e.toString(16)+"\n";
			if (canDecrypt) {
				s+="D="+d.toString(16)+"\n";
				if (p!=null && q!=null) {
					s+="P="+p.toString(16)+"\n";
					s+="Q="+q.toString(16)+"\n";
					s+="DMP1="+dmp1.toString(16)+"\n";
					s+="DMQ1="+dmq1.toString(16)+"\n";
					s+="IQMP="+coeff.toString(16)+"\n";
				}
			}
			return s;
		}

		protected function doPublic(x:BigInteger):BigInteger {
			return x.modPowInt(e, n);
		}
	}
}
