// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2025, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)

// This is the reference decoder implementation by Jasper St. Pierre.
// It follows the decoder interface and should be a drop-in replacement for the actual decoder from meshopt_decoder.js
// It is provided for educational value and is not recommended for use in production because it's not performance-optimized.

const MeshoptDecoder = {};
MeshoptDecoder.supported = true;
MeshoptDecoder.ready = Promise.resolve();

function assert(cond) {
	if (!cond) {
		throw new Error('Assertion failed');
	}
}

function dezig(v) {
	return (v & 1) !== 0 ? ~(v >>> 1) : v >>> 1;
}

MeshoptDecoder.decodeVertexBuffer = (target, elementCount, byteStride, source, filter) => {
	assert(source[0] === 0xa0 || source[0] === 0xa1);
	const version = source[0] & 0x0f;

	const maxBlockElements = Math.min((0x2000 / byteStride) & ~0x000f, 0x100);

	const deltas = new Uint8Array(maxBlockElements * byteStride);

	const tailSize = version === 0 ? byteStride : byteStride + byteStride / 4;
	const tailDataOffs = source.length - tailSize;

	// What deltas are stored relative to
	const tempData = source.slice(tailDataOffs, tailDataOffs + byteStride);

	// Channel modes for v1
	const channels = version === 0 ? null : source.slice(tailDataOffs + byteStride, tailDataOffs + tailSize);

	let srcOffs = 1; // Skip header byte

	const headerModes = [
		[0, 2, 4, 8], // v0
		[0, 1, 2, 4], // v1, when control is 0
		[1, 2, 4, 8], // v1, when control is 1
	];

	// Attribute blocks
	for (let dstElemBase = 0; dstElemBase < elementCount; dstElemBase += maxBlockElements) {
		const attrBlockElementCount = Math.min(elementCount - dstElemBase, maxBlockElements);
		const groupCount = ((attrBlockElementCount + 0x0f) & ~0x0f) >>> 4;
		const headerByteCount = ((groupCount + 0x03) & ~0x03) >>> 2;

		// Control modes for v1
		const controlBitsOffs = srcOffs;
		srcOffs += version === 0 ? 0 : byteStride / 4;

		// Zero out deltas to simplify logic
		deltas.fill(0x00);

		// Data blocks
		for (let byte = 0; byte < byteStride; byte++) {
			const deltaBase = byte * attrBlockElementCount;

			// Control mode for current byte for v1
			const controlMode = version === 0 ? 0 : (source[controlBitsOffs + (byte >>> 2)] >>> ((byte & 0x03) << 1)) & 0x03;

			if (controlMode === 2) {
				// All byte deltas are 0; no data is stored for this byte
				continue;
			} else if (controlMode === 3) {
				// Byte deltas are stored uncompressed with no header bits
				deltas.set(source.subarray(srcOffs, srcOffs + attrBlockElementCount), deltaBase);
				srcOffs += attrBlockElementCount;
				continue;
			}

			// Header bits are omitted for v1 when using control modes 2/3
			const headerBitsOffs = srcOffs;
			srcOffs += headerByteCount;

			for (let group = 0; group < groupCount; group++) {
				const mode = (source[headerBitsOffs + (group >>> 2)] >>> ((group & 0x03) << 1)) & 0x03;
				const modeBits = headerModes[version === 0 ? 0 : controlMode + 1][mode];

				const deltaOffs = deltaBase + (group << 4);

				if (modeBits === 0) {
					// All 16 byte deltas are 0; the size of the encoded block is 0 bytes
				} else if (modeBits === 1) {
					// Deltas are using 1-bit sentinel encoding; the size of the encoded block is [2..18] bytes
					const srcBase = srcOffs;
					srcOffs += 0x02;
					for (let m = 0; m < 0x10; m++) {
						// Bits are stored from least significant to most significant for 1-bit encoding
						const shift = m & 0x07;
						let delta = (source[srcBase + (m >>> 3)] >>> shift) & 0x01;
						if (delta === 1) delta = source[srcOffs++];
						deltas[deltaOffs + m] = delta;
					}
				} else if (modeBits === 2) {
					// Deltas are using 2-bit sentinel encoding; the size of the encoded block is [4..20] bytes
					const srcBase = srcOffs;
					srcOffs += 0x04;
					for (let m = 0; m < 0x10; m++) {
						// 0 = >>> 6, 1 = >>> 4, 2 = >>> 2, 3 = >>> 0
						const shift = 6 - ((m & 0x03) << 1);
						let delta = (source[srcBase + (m >>> 2)] >>> shift) & 0x03;
						if (delta === 3) delta = source[srcOffs++];
						deltas[deltaOffs + m] = delta;
					}
				} else if (modeBits === 4) {
					// Deltas are using 4-bit sentinel encoding; the size of the encoded block is [8..24] bytes
					const srcBase = srcOffs;
					srcOffs += 0x08;
					for (let m = 0; m < 0x10; m++) {
						// 0 = >>> 6, 1 = >>> 4, 2 = >>> 2, 3 = >>> 0
						const shift = 4 - ((m & 0x01) << 2);
						let delta = (source[srcBase + (m >>> 1)] >>> shift) & 0x0f;
						if (delta === 0xf) delta = source[srcOffs++];
						deltas[deltaOffs + m] = delta;
					}
				} else {
					// All 16 byte deltas are stored verbatim; the size of the encoded block is 16 bytes
					deltas.set(source.subarray(srcOffs, srcOffs + 0x10), deltaOffs);
					srcOffs += 0x10;
				}
			}
		}

		// Go through and apply deltas to data
		for (let elem = 0; elem < attrBlockElementCount; elem++) {
			const dstElem = dstElemBase + elem;

			for (let byteGroup = 0; byteGroup < byteStride; byteGroup += 4) {
				let channelMode = version === 0 ? 0 : channels[byteGroup >>> 2] & 0x03;
				assert(channelMode !== 0x03);

				if (channelMode === 0) {
					// Channel 0 (byte deltas): Byte deltas are stored as zigzag-encoded differences between the byte values of the element and the byte values of the previous element in the same position.
					for (let byte = byteGroup; byte < byteGroup + 4; byte++) {
						const delta = dezig(deltas[byte * attrBlockElementCount + elem]);
						const temp = (tempData[byte] + delta) & 0xff; // wrap around

						const dstOffs = dstElem * byteStride + byte;
						target[dstOffs] = tempData[byte] = temp;
					}
				} else if (channelMode === 1) {
					// Channel 1 (2-byte deltas): 2-byte deltas are computed as zigzag-encoded differences between 16-bit values of the element and the previous element in the same position.
					for (let byte = byteGroup; byte < byteGroup + 4; byte += 2) {
						const delta = dezig(deltas[byte * attrBlockElementCount + elem] + (deltas[(byte + 1) * attrBlockElementCount + elem] << 8));
						let temp = tempData[byte] + (tempData[byte + 1] << 8);

						temp = (temp + delta) & 0xffff; // wrap around

						const dstOffs = dstElem * byteStride + byte;
						target[dstOffs] = tempData[byte] = temp & 0xff;
						target[dstOffs + 1] = tempData[byte + 1] = temp >>> 8;
					}
				} else if (channelMode === 2) {
					// Channel 2 (4-byte XOR deltas): 4-byte deltas are computed as XOR between 32-bit values of the element and the previous element in the same position, with an additional rotation applied based on the high 4 bits of the channel mode byte.
					const byte = byteGroup;
					const delta =
						deltas[byte * attrBlockElementCount + elem] +
						(deltas[(byte + 1) * attrBlockElementCount + elem] << 8) +
						(deltas[(byte + 2) * attrBlockElementCount + elem] << 16) +
						(deltas[(byte + 3) * attrBlockElementCount + elem] << 24);
					let temp = tempData[byte] + (tempData[byte + 1] << 8) + (tempData[byte + 2] << 16) + (tempData[byte + 3] << 24);

					const rot = channels[byteGroup >>> 2] >>> 4;
					temp = temp ^ ((delta >>> rot) | (delta << (32 - rot))); // rotate and XOR

					const dstOffs = dstElem * byteStride + byte;
					target[dstOffs] = tempData[byte] = temp & 0xff;
					target[dstOffs + 1] = tempData[byte + 1] = (temp >>> 8) & 0xff;
					target[dstOffs + 2] = tempData[byte + 2] = (temp >>> 16) & 0xff;
					target[dstOffs + 3] = tempData[byte + 3] = temp >>> 24;
				}
			}
		}
	}

	const tailSizePadded = Math.max(tailSize, version === 0 ? 32 : 24);
	assert(srcOffs == source.length - tailSizePadded);

	// Filters - only applied if filter isn't undefined or NONE
	if (filter === 'OCTAHEDRAL') {
		assert(byteStride === 4 || byteStride === 8);

		const dst = byteStride === 4 ? new Int8Array(target.buffer) : new Int16Array(target.buffer);
		const maxInt = byteStride === 4 ? 127 : 32767;

		for (let i = 0; i < 4 * elementCount; i += 4) {
			let x = dst[i + 0],
				y = dst[i + 1],
				one = dst[i + 2];
			x /= one;
			y /= one;
			const z = 1.0 - Math.abs(x) - Math.abs(y);
			const t = Math.max(-z, 0.0);
			x -= x >= 0 ? t : -t;
			y -= y >= 0 ? t : -t;
			const h = maxInt / Math.hypot(x, y, z);
			dst[i + 0] = Math.round(x * h);
			dst[i + 1] = Math.round(y * h);
			dst[i + 2] = Math.round(z * h);
			// keep dst[i + 3] as is
		}
	} else if (filter === 'QUATERNION') {
		assert(byteStride === 8);

		const dst = new Int16Array(target.buffer);

		for (let i = 0; i < 4 * elementCount; i += 4) {
			const inputW = dst[i + 3];
			const maxComponent = inputW & 0x03;
			const s = Math.SQRT1_2 / (inputW | 0x03);
			let x = dst[i + 0] * s;
			let y = dst[i + 1] * s;
			let z = dst[i + 2] * s;
			let w = Math.sqrt(Math.max(0.0, 1.0 - x ** 2 - y ** 2 - z ** 2));
			dst[i + ((maxComponent + 1) % 4)] = Math.round(x * 32767);
			dst[i + ((maxComponent + 2) % 4)] = Math.round(y * 32767);
			dst[i + ((maxComponent + 3) % 4)] = Math.round(z * 32767);
			dst[i + ((maxComponent + 0) % 4)] = Math.round(w * 32767);
		}
	} else if (filter === 'EXPONENTIAL') {
		assert((byteStride & 0x03) === 0x00);

		const src = new Int32Array(target.buffer);
		const dst = new Float32Array(target.buffer);
		for (let i = 0; i < (byteStride * elementCount) / 4; i++) {
			const v = src[i],
				exp = v >> 24,
				mantissa = (v << 8) >> 8;
			dst[i] = 2.0 ** exp * mantissa;
		}
	} else if (filter === 'COLOR') {
		assert(byteStride === 4 || byteStride === 8);

		const maxInt = (1 << (byteStride * 2)) - 1;

		const data = byteStride === 4 ? new Uint8Array(target.buffer) : new Uint16Array(target.buffer, 0, elementCount * 4);
		const dataSigned = byteStride === 4 ? new Int8Array(target.buffer) : new Int16Array(target.buffer, 0, elementCount * 4);

		for (let i = 0; i < elementCount * 4; i += 4) {
			const y = data[i + 0];
			const co = dataSigned[i + 1];
			const cg = dataSigned[i + 2];
			const alphaInput = data[i + 3];

			// Recover scale from alpha high bit - find highest bit set
			const alphaBit = 31 - Math.clz32(alphaInput);
			const as = (1 << (alphaBit + 1)) - 1;

			// YCoCg to RGB conversion
			const r = y + co - cg;
			const g = y + cg;
			const b = y - co - cg;

			// Expand alpha by one bit, replicating last bit
			let a = alphaInput & (as >> 1);
			a = (a << 1) | (a & 1);

			// Scale to full range
			const ss = maxInt / as;

			// Store result
			data[i + 0] = Math.round(r * ss);
			data[i + 1] = Math.round(g * ss);
			data[i + 2] = Math.round(b * ss);
			data[i + 3] = Math.round(a * ss);
		}
	}
};

function pushfifo(fifo, n) {
	for (let i = fifo.length - 1; i > 0; i--) fifo[i] = fifo[i - 1];
	fifo[0] = n;
}

MeshoptDecoder.decodeIndexBuffer = (target, count, byteStride, source) => {
	assert(source[0] === 0xe1);
	assert(count % 3 === 0);
	assert(byteStride === 2 || byteStride === 4);

	let dst;
	if (byteStride === 2) dst = new Uint16Array(target.buffer);
	else dst = new Uint32Array(target.buffer);

	const triCount = count / 3;

	let codeOffs = 0x01;
	let dataOffs = codeOffs + triCount;
	let codeauxOffs = source.length - 0x10;

	function readLEB128() {
		let n = 0;
		for (let i = 0; ; i += 7) {
			const b = source[dataOffs++];
			n |= (b & 0x7f) << i;

			if (b < 0x80) return n;
		}
	}

	let next = 0,
		last = 0;
	const edgefifo = new Uint32Array(32);
	const vertexfifo = new Uint32Array(16);

	function decodeIndex(v) {
		return (last += dezig(v));
	}

	let dstOffs = 0;
	for (let i = 0; i < triCount; i++) {
		const code = source[codeOffs++];
		const b0 = code >>> 4,
			b1 = code & 0x0f;

		if (b0 < 0x0f) {
			const a = edgefifo[(b0 << 1) + 0],
				b = edgefifo[(b0 << 1) + 1];
			let c = -1;

			if (b1 === 0x00) {
				c = next++;
				pushfifo(vertexfifo, c);
			} else if (b1 < 0x0d) {
				c = vertexfifo[b1];
			} else if (b1 === 0x0d) {
				c = --last;
				pushfifo(vertexfifo, c);
			} else if (b1 === 0x0e) {
				c = ++last;
				pushfifo(vertexfifo, c);
			} else if (b1 === 0x0f) {
				const v = readLEB128();
				c = decodeIndex(v);
				pushfifo(vertexfifo, c);
			}

			// fifo pushes happen backwards
			pushfifo(edgefifo, b);
			pushfifo(edgefifo, c);
			pushfifo(edgefifo, c);
			pushfifo(edgefifo, a);

			dst[dstOffs++] = a;
			dst[dstOffs++] = b;
			dst[dstOffs++] = c;
		} else {
			// b0 === 0x0F
			let a = -1,
				b = -1,
				c = -1;

			if (b1 < 0x0e) {
				const e = source[codeauxOffs + b1];
				const z = e >>> 4,
					w = e & 0x0f;

				a = next++;

				if (z === 0x00) b = next++;
				else b = vertexfifo[z - 1];

				if (w === 0x00) c = next++;
				else c = vertexfifo[w - 1];

				pushfifo(vertexfifo, a);
				if (z === 0x00) pushfifo(vertexfifo, b);
				if (w === 0x00) pushfifo(vertexfifo, c);
			} else {
				const e = source[dataOffs++];
				if (e === 0x00) next = 0;

				const z = e >>> 4,
					w = e & 0x0f;

				if (b1 === 0x0e) a = next++;
				else a = decodeIndex(readLEB128());

				if (z === 0x00) b = next++;
				else if (z === 0x0f) b = decodeIndex(readLEB128());
				else b = vertexfifo[z - 1];

				if (w === 0x00) c = next++;
				else if (w === 0x0f) c = decodeIndex(readLEB128());
				else c = vertexfifo[w - 1];

				pushfifo(vertexfifo, a);
				if (z === 0x00 || z === 0x0f) pushfifo(vertexfifo, b);
				if (w === 0x00 || w === 0x0f) pushfifo(vertexfifo, c);
			}

			pushfifo(edgefifo, a);
			pushfifo(edgefifo, b);
			pushfifo(edgefifo, b);
			pushfifo(edgefifo, c);
			pushfifo(edgefifo, c);
			pushfifo(edgefifo, a);

			dst[dstOffs++] = a;
			dst[dstOffs++] = b;
			dst[dstOffs++] = c;
		}
	}
};

MeshoptDecoder.decodeIndexSequence = (target, count, byteStride, source) => {
	assert(source[0] === 0xd1);
	assert(byteStride === 2 || byteStride === 4);

	let dst;
	if (byteStride === 2) dst = new Uint16Array(target.buffer);
	else dst = new Uint32Array(target.buffer);

	let dataOffs = 0x01;

	function readLEB128() {
		let n = 0;
		for (let i = 0; ; i += 7) {
			const b = source[dataOffs++];
			n |= (b & 0x7f) << i;

			if (b < 0x80) return n;
		}
	}

	const last = new Uint32Array(2);

	for (let i = 0; i < count; i++) {
		const v = readLEB128();
		const b = v & 0x01;
		const delta = dezig(v >>> 1);
		dst[i] = last[b] += delta;
	}
};

MeshoptDecoder.decodeGltfBuffer = (target, count, size, source, mode, filter) => {
	const table = {
		ATTRIBUTES: MeshoptDecoder.decodeVertexBuffer,
		TRIANGLES: MeshoptDecoder.decodeIndexBuffer,
		INDICES: MeshoptDecoder.decodeIndexSequence,
	};
	assert(table[mode] !== undefined);
	table[mode](target, count, size, source, filter);
};

MeshoptDecoder.decodeGltfBufferAsync = (count, size, source, mode, filter) => {
	const target = new Uint8Array(count * size);
	MeshoptDecoder.decodeGltfBuffer(target, count, size, source, mode, filter);
	return Promise.resolve(target);
};

// node.js interface:
// for (let k in MeshoptDecoder) exports[k] = MeshoptDecoder[k];

export { MeshoptDecoder };
