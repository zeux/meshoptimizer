import assert from 'assert/strict';
import { MeshoptTangents as tangents } from './meshopt_tangents.js';

process.on('unhandledRejection', (error) => {
	console.log('unhandledRejection', error);
	process.exit(1);
});

var tests = {
	tangentsBasic: function () {
		// unindexed quad with matching corner data for shared diagonal
		var positions = new Float32Array([0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0]);
		var normals = new Float32Array([-0.28, 0, 0.96, 0.28, 0, 0.96, 0.28, 0, 0.96, -0.28, 0, 0.96, 0.28, 0, 0.96, -0.28, 0, 0.96]);
		var uvs = new Float32Array([0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1]);

		// (1, 0, 0) reprojected onto tilted normals
		var left = [0.96, 0, 0.28, 1];
		var right = [0.96, 0, -0.28, 1];
		var expected = [left, right, right, left, right, left].flat();

		// unindexed input: indices = null
		var result = tangents.generateTangents(null, positions, 3, normals, 3, uvs, 2, []);

		assert.equal(result.length, expected.length);

		for (var i = 0; i < result.length; ++i) {
			assert(Math.abs(result[i] - expected[i]) < 1e-3);
		}

		// shared vertices get the same tangent vector
		for (var k = 0; k < 4; ++k) {
			assert(result[0 * 4 + k] === result[3 * 4 + k]); // diag 1
			assert(result[2 * 4 + k] === result[4 * 4 + k]); // diag 2
		}
	},

	tangentDegenerate: function () {
		var positions = new Float32Array([0, 0, 0, 1, 1, 0, 2, 2, 0, -1, -2, 1, 0, 1, 1, -1, 0, 0]);
		var normals = new Float32Array([0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1]);
		var uvs = new Float32Array([0, 0, 1, 1, 2, 2, 0, -2, 1, 0, 1, 0]);

		var indices = new Uint32Array([
			// outer, positive
			0, 3, 1,
			// inner, degenerate UVs
			0, 1, 2,
			// outer, positive
			1, 4, 2,
			// outer, negative
			2, 5, 0,
		]);

		var vx = 0.0836;
		var vy = 0.9965;
		var expected = [
			// outer, positive
			[1, 0, 0, 1],
			[1, 0, 0, 1],
			[vx, vy, 0, 1],
			// inner, degenerate UVs
			[1, 0, 0, 1],
			[vx, vy, 0, 1],
			[0, 1, 0, 1],
			// outer, positive
			[vx, vy, 0, 1],
			[0, 1, 0, 1],
			[0, 1, 0, 1],
			// outer, negative
			[-1, 0, 0, -1],
			[-1, 0, 0, -1],
			[-1, 0, 0, -1],
		].flat();

		var result = tangents.generateTangents(indices, positions, 3, normals, 3, uvs, 2, ['Compatible']);

		assert.equal(result.length, expected.length);

		for (var i = 0; i < result.length; ++i) {
			assert(Math.abs(result[i] - expected[i]) < 1e-3);
		}
	},
};

tangents.ready.then(() => {
	var count = 0;

	for (var key in tests) {
		tests[key]();
		count++;
	}

	console.log(count, 'tests passed');
});
