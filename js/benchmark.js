var encoder = require('./meshopt_encoder.js');
var decoder = require('./meshopt_decoder.js');
var { performance } = require('perf_hooks');

process.on('unhandledRejection', (error) => {
	console.log('unhandledRejection', error);
	process.exit(1);
});

function bytes(view) {
	return new Uint8Array(view.buffer, view.byteOffset, view.byteLength);
}

var tests = {
	roundtripVertexBuffer: function () {
		var N = 1024 * 1024;
		var data = new Uint8Array(N * 16);

		var lcg = 1;

		for (var i = 0; i < N * 16; ++i) {
			// mindstd_rand
			lcg = (lcg * 48271) % 2147483647;

			var k = i % 16;
			if (k <= 8) data[i] = lcg & ((1 << k) - 1);
			else data[i] = i & ((1 << (k - 8)) - 1);
		}

		var decoded = new Uint8Array(N * 16);

		var t0 = performance.now();
		var encoded = encoder.encodeVertexBuffer(data, N, 16);
		var t1 = performance.now();
		decoder.decodeVertexBuffer(decoded, N, 16, encoded);
		var t2 = performance.now();

		return { encodeVertex: t1 - t0, decodeVertex: t2 - t1, bytes: N * 16 };
	},

	roundtripIndexBuffer: function () {
		var N = 1024 * 1024;
		var data = new Uint32Array(N * 3);

		for (var i = 0; i < N * 3; i += 6) {
			var v = i / 6;

			data[i + 0] = v;
			data[i + 1] = v + 1;
			data[i + 2] = v + 2;

			data[i + 3] = v + 2;
			data[i + 4] = v + 1;
			data[i + 5] = v + 3;
		}

		var decoded = new Uint32Array(data.length);

		var t0 = performance.now();
		var encoded = encoder.encodeIndexBuffer(bytes(data), data.length, 4);
		var t1 = performance.now();
		decoder.decodeIndexBuffer(bytes(decoded), data.length, 4, encoded);
		var t2 = performance.now();

		return { encodeIndex: t1 - t0, decodeIndex: t2 - t1, bytes: N * 12 };
	},

	decodeGltf: function () {
		var N = 1024 * 1024;
		var data = new Uint8Array(N * 16);

		for (var i = 0; i < N * 16; i += 4) {
			data[i + 0] = 0;
			data[i + 1] = (i % 16) * 1;
			data[i + 2] = (i % 16) * 2;
			data[i + 3] = (i % 16) * 8;
		}

		var decoded = new Uint8Array(N * 16);

		var filters = [
			{ name: 'none', filter: 'NONE', stride: 16 },
			{ name: 'oct4', filter: 'OCTAHEDRAL', stride: 4 },
			{ name: 'oct12', filter: 'OCTAHEDRAL', stride: 8 },
			{ name: 'quat12', filter: 'QUATERNION', stride: 8 },
			{ name: 'exp', filter: 'EXPONENTIAL', stride: 16 },
		];

		var results = { bytes: N * 16 };

		for (var i = 0; i < filters.length; ++i) {
			var f = filters[i];
			var encoded = encoder.encodeVertexBuffer(data, (N * 16) / f.stride, f.stride);

			var t0 = performance.now();
			decoder.decodeGltfBuffer(decoded, (N * 16) / f.stride, f.stride, encoded, 'ATTRIBUTES', f.filter);
			var t1 = performance.now();

			results[f.name] = t1 - t0;
		}

		return results;
	},
};

Promise.all([encoder.ready, decoder.ready]).then(() => {
	var reps = 10;
	var data = {};

	for (var key in tests) {
		data[key] = tests[key]();
	}

	for (var i = 1; i < reps; ++i) {
		for (var key in tests) {
			var nd = tests[key]();
			var od = data[key];

			for (var idx in nd) {
				od[idx] = Math.min(od[idx], nd[idx]);
			}
		}
	}

	for (var key in tests) {
		var rep = key;
		rep += ':\n';

		for (var idx in data[key]) {
			if (idx != 'bytes') {
				rep += idx;
				rep += ' ';
				rep += data[key][idx];
				rep += ' ms (';
				rep += (data[key].bytes / 1e9 / data[key][idx]) * 1000;
				rep += ' GB/s)';
				rep += '\n';
			}
		}

		console.log(rep);
	}
});
