var assert = require('assert').strict;
var simplifier = require('./meshopt_simplifier.js');

process.on('unhandledRejection', (error) => {
	console.log('unhandledRejection', error);
	process.exit(1);
});

simplifier.useExperimentalFeatures = true;

var tests = {
	compactMesh: function () {
		// prettier-ignore
		var indices = new Uint32Array([
			0, 1, 3,
			3, 1, 5,
		]);

		// prettier-ignore
		var expected = new Uint32Array([
			0, 1, 2,
			2, 1, 3,
		]);

		var missing = 2 ** 32 - 1;

		var remap = new Uint32Array([0, 1, missing, 2, missing, 3]);

		var res = simplifier.compactMesh(indices);
		assert.deepEqual(indices, expected);
		assert.deepEqual(res[0], remap);
		assert.equal(res[1], 4); // unique
	},

	simplify: function () {
		// 0
		// 1 2
		// 3 4 5

		// prettier-ignore
		var indices = new Uint32Array([
			0, 2, 1,
			1, 2, 3,
			3, 2, 4,
			2, 5, 4,
		]);

		// prettier-ignore
		var positions = new Float32Array([
			0, 4, 0,
			0, 1, 0,
			2, 2, 0,
			0, 0, 0,
			1, 0, 0,
			4, 0, 0,
		]);

		var res = simplifier.simplify(indices, positions, 3, /* target indices */ 3, /* target error */ 0.01);

		var expected = new Uint32Array([0, 5, 3]);

		assert.deepEqual(res[0], expected);
		assert.equal(res[1], 0); // error
	},

	simplify16: function () {
		// 0
		// 1 2
		// 3 4 5

		// prettier-ignore
		var indices = new Uint16Array([
			0, 2, 1,
			1, 2, 3,
			3, 2, 4,
			2, 5, 4,
		]);

		// prettier-ignore
		var positions = new Float32Array([
			0, 4, 0,
			0, 1, 0,
			2, 2, 0,
			0, 0, 0,
			1, 0, 0,
			4, 0, 0,
		]);

		var res = simplifier.simplify(indices, positions, 3, /* target indices */ 3, /* target error */ 0.01);

		// prettier-ignore
		var expected = new Uint16Array([
			0, 5, 3,
		]);

		assert.deepEqual(res[0], expected);
		assert.equal(res[1], 0); // error
	},

	simplifyLockBorder: function () {
		// 0
		// 1 2
		// 3 4 5

		// prettier-ignore
		var indices = new Uint32Array([
			0, 2, 1,
			1, 2, 3,
			3, 2, 4,
			2, 5, 4,
		]);

		// prettier-ignore
		var positions = new Float32Array([
			0, 2, 0,
			0, 1, 0,
			1, 1, 0,
			0, 0, 0,
			1, 0, 0,
			2, 0, 0,
		]);

		var res = simplifier.simplify(indices, positions, 3, /* target indices */ 3, /* target error */ 0.01, ['LockBorder']);

		// prettier-ignore
		var expected = new Uint32Array([
			0, 2, 1,
			1, 2, 3,
			3, 2, 4,
			2, 5, 4,
		]);

		assert.deepEqual(res[0], expected);
		assert.equal(res[1], 0); // error
	},

	simplifyAttr: function () {
		var vb_pos = new Float32Array(8 * 3 * 3);
		var vb_att = new Float32Array(8 * 3 * 3);

		for (var y = 0; y < 8; ++y) {
			// first four rows are a blue gradient, next four rows are a yellow gradient
			var r = y < 4 ? 0.8 + y * 0.05 : 0;
			var g = y < 4 ? 0.8 + y * 0.05 : 0;
			var b = y < 4 ? 0 : 0.8 + (7 - y) * 0.05;

			for (var x = 0; x < 3; ++x) {
				vb_pos[(y * 3 + x) * 3 + 0] = x;
				vb_pos[(y * 3 + x) * 3 + 1] = y;
				vb_pos[(y * 3 + x) * 3 + 2] = 0.03 * x;
				vb_att[(y * 3 + x) * 3 + 0] = r;
				vb_att[(y * 3 + x) * 3 + 1] = g;
				vb_att[(y * 3 + x) * 3 + 2] = b;
			}
		}

		var ib = new Uint32Array(7 * 2 * 6);

		for (var y = 0; y < 7; ++y) {
			for (var x = 0; x < 2; ++x) {
				ib[(y * 2 + x) * 6 + 0] = (y + 0) * 3 + (x + 0);
				ib[(y * 2 + x) * 6 + 1] = (y + 0) * 3 + (x + 1);
				ib[(y * 2 + x) * 6 + 2] = (y + 1) * 3 + (x + 0);
				ib[(y * 2 + x) * 6 + 3] = (y + 1) * 3 + (x + 0);
				ib[(y * 2 + x) * 6 + 4] = (y + 0) * 3 + (x + 1);
				ib[(y * 2 + x) * 6 + 5] = (y + 1) * 3 + (x + 1);
			}
		}

		var attr_weights = [0.01, 0.01, 0.01];

		var res = simplifier.simplifyWithAttributes(ib, vb_pos, 3, vb_att, 3, attr_weights, null, 6 * 3, 1e-2);

		// prettier-ignore
		var expected = new Uint32Array([
			0, 2, 9, 9, 2, 11,
			9, 11, 12, 12, 11, 14,
			12, 14, 21, 21, 14, 23,
		]);

		assert.deepEqual(res[0], expected);
	},

	getScale: function () {
		// prettier-ignore
		var positions = new Float32Array([
			0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 3
		]);

		assert(simplifier.getScale(positions, 3) == 3.0);
	},

	simplifyPoints: function () {
		// prettier-ignore
		var positions = new Float32Array([
			0, 0, 0, 100, 0, 0, 100, 1, 1, 110, 0, 0,
		]);
		// prettier-ignore
		var colors = new Float32Array([
			1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
		]);
		// prettier-ignore
		var expected = new Uint32Array([
			0, 1
		]);
		// prettier-ignore
		var expectedC = new Uint32Array([
			0, 2
		]);

		var res = simplifier.simplifyPoints(positions, 3, 2);
		assert.deepEqual(res, expected);

		// note: recommended value for color_weight is 1e-2 but here we push color weight to be very high to bias candidate selection for testing
		var resC1 = simplifier.simplifyPoints(positions, 3, 2, colors, 3, 1e-1);
		assert.deepEqual(resC1, expectedC);

		var resC2 = simplifier.simplifyPoints(positions, 3, 2, colors, 3, 1e-2);
		assert.deepEqual(resC2, expected);
	},
};

Promise.all([simplifier.ready]).then(() => {
	var count = 0;

	for (var key in tests) {
		tests[key]();
		count++;
	}

	console.log(count, 'tests passed');
});
