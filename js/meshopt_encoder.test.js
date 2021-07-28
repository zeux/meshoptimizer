var assert = require('assert').strict;
var encoder = require('./meshopt_encoder.js');
var decoder = require('./meshopt_decoder.js');

process.on('unhandledRejection', error => {
	console.log('unhandledRejection', error);
	process.exit(1);
});

var tests = {
	roundtripVertexBuffer: function() {
		var data = new Uint8Array(16 * 4);

		// this tests 0/2/4/8 bit groups in one stream
		for (var i = 0; i < 16; ++i)
		{
			data[i * 4 + 0] = 0;
			data[i * 4 + 1] = i * 1;
			data[i * 4 + 2] = i * 2;
			data[i * 4 + 3] = i * 8;
		}

		var encoded = encoder.encodeVertexBuffer(data, 16, 4);
		var decoded = new Uint8Array(16 * 4);
		decoder.decodeVertexBuffer(decoded, 16, 4, encoded);

		assert.deepEqual(decoded, data);
	},

	roundtripIndexBuffer: function() {
		var data = new Uint32Array([0, 1, 2, 2, 1, 3, 4, 6, 5, 7, 8, 9]);

		var encoded = encoder.encodeIndexBuffer(data, data.length);
		var decoded = new Uint32Array(data.length);
		decoder.decodeIndexBuffer(new Uint8Array(decoded.buffer), data.length, 4, encoded);

		assert.deepEqual(decoded, data);
	},

	roundtripIndexBuffer16: function() {
		var data = new Uint16Array([0, 1, 2, 2, 1, 3, 4, 6, 5, 7, 8, 9]);

		var encoded = encoder.encodeIndexBuffer(data, data.length);
		var decoded = new Uint16Array(data.length);
		decoder.decodeIndexBuffer(new Uint8Array(decoded.buffer), data.length, 2, encoded);

		assert.deepEqual(decoded, data);
	},

	roundtripIndexSequence: function() {
		var data = new Uint32Array([0, 1, 51, 2, 49, 1000]);

		var encoded = encoder.encodeIndexSequence(data, data.length);
		var decoded = new Uint32Array(data.length);
		decoder.decodeIndexSequence(new Uint8Array(decoded.buffer), data.length, 4, encoded);

		assert.deepEqual(decoded, data);
	},

	roundtripIndexSequence16: function() {
		var data = new Uint16Array([0, 1, 51, 2, 49, 1000]);

		var encoded = encoder.encodeIndexSequence(data, data.length);
		var decoded = new Uint16Array(data.length);
		decoder.decodeIndexSequence(new Uint8Array(decoded.buffer), data.length, 2, encoded);

		assert.deepEqual(decoded, data);
	},
};

Promise.all([encoder.ready, decoder.ready]).then(() => {
	var count = 0;

	for (var key in tests) {
		tests[key]();
		count++;
	}

	console.log(count, 'tests passed');
});
