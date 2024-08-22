const assert = require('assert').strict;
const clusterizer = require('./meshopt_clusterizer.js');

process.on('unhandledRejection', (error) => {
	console.log('unhandledRejection', error);
	process.exit(1);
});

const cubeWithNormals = {
	vertices: new Float32Array([
		// n = (0, 0, 1)
		-1.0, -1.0, 1.0, 0.0, 0.0, 1.0, 1.0, -1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, -1.0, 1.0, 1.0, 0.0, 0.0, 1.0,
		// n = (0, 0, -1)
		-1.0, 1.0, -1.0, 0.0, 0.0, -1.0, 1.0, 1.0, -1.0, 0.0, 0.0, -1.0, 1.0, -1.0, -1.0, 0.0, 0.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0, -1.0,
		// n = (1, 0, 0)
		1.0, -1.0, -1.0, 1.0, 0.0, 0.0, 1.0, 1.0, -1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, -1.0, 1.0, 1.0, 0.0, 0.0,
		// n = (-1, 0, 0)
		-1.0, -1.0, 1.0, -1.0, 0.0, 0.0, -1.0, 1.0, 1.0, -1.0, 0.0, 0.0, -1.0, 1.0, -1.0, -1.0, 0.0, 0.0, -1.0, -1.0, -1.0, -1.0, 0.0, 0.0,
		// n = (0, 1, 0)
		1.0, 1.0, -1.0, 0.0, 1.0, 0.0, -1.0, 1.0, -1.0, 0.0, 1.0, 0.0, -1.0, 1.0, 1.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0,
		// n = (0, -1, 0)
		1.0, -1.0, 1.0, 0.0, -1.0, 0.0, -1.0, -1.0, 1.0, 0.0, -1.0, 0.0, -1.0, -1.0, -1.0, 0.0, -1.0, 0.0, 1.0, -1.0, -1.0, 0.0, -1.0, 0.0,
	]),
	indices: new Uint32Array([
		// n = (0, 0, 1)
		0, 1, 2, 2, 3, 0,
		// n = (0, 0, -1)
		4, 5, 6, 6, 7, 4,
		// n = (1, 0, 0)
		8, 9, 10, 10, 11, 8,
		// n = (-1, 0, 0)
		12, 13, 14, 14, 15, 12,
		// n = (0, 1, 0)
		16, 17, 18, 18, 19, 16,
		// n = (0, -1, 0)
		20, 21, 22, 22, 23, 20,
	]),
	vertexStride: 6, // in floats
};

const tests = {
	buildMeshlets: function () {
		const maxVertices = 4;
		const buffers = clusterizer.buildMeshlets(cubeWithNormals.indices, cubeWithNormals.vertices, cubeWithNormals.vertexStride, maxVertices, 512);

		const expectedVertices = [
			new Uint32Array([2, 3, 0, 1]),
			new Uint32Array([12, 13, 14, 15]),
			new Uint32Array([6, 7, 4, 5]),
			new Uint32Array([16, 17, 18, 19]),
			new Uint32Array([8, 9, 10, 11]),
			new Uint32Array([22, 23, 20, 21]),
		];
		const expectedTriangles = new Uint8Array([0, 1, 2, 2, 3, 0]);

		assert.equal(buffers.meshletCount, 6);

		for (let i = 0; i < buffers.meshletCount; ++i) {
			const m = clusterizer.extractMeshlet(buffers, i);
			assert.deepStrictEqual(m.vertices, expectedVertices[i]);
			assert.deepStrictEqual(m.triangles, expectedTriangles);
		}
	},

	computeClusterBounds: function () {
		for (let i = 0; i < 6; ++i) {
			const indexOffset = i * 6;
			const normalOffset = i * 4 * cubeWithNormals.vertexStride;
			const bounds = clusterizer.computeClusterBounds(
				cubeWithNormals.indices.subarray(indexOffset, 6 + indexOffset),
				cubeWithNormals.vertices,
				cubeWithNormals.vertexStride
			);
			assert.deepStrictEqual(
				new Int32Array([bounds.coneAxisX, bounds.coneAxisY, bounds.coneAxisZ]),
				new Int32Array(cubeWithNormals.vertices.subarray(3 + normalOffset, 6 + normalOffset))
			);
		}
	},

	computeMeshletBounds: function () {
		const maxVertices = 4;
		const buffers = clusterizer.buildMeshlets(cubeWithNormals.indices, cubeWithNormals.vertices, cubeWithNormals.vertexStride, maxVertices, 512);

		const expectedNormals = [
			new Int32Array([0, 0, 1]),
			new Int32Array([-1, 0, 0]),
			new Int32Array([0, 0, -1]),
			new Int32Array([0, 1, 0]),
			new Int32Array([1, 0, 0]),
			new Int32Array([0, -1, 0]),
		];

		const bounds = clusterizer.computeMeshletBounds(buffers, cubeWithNormals.vertices, cubeWithNormals.vertexStride);

		assert(bounds.length === 6);
		assert(bounds.length === buffers.meshletCount);

		bounds.forEach((b, i) => {
			const normal = new Int32Array([b.coneAxisX, b.coneAxisY, b.coneAxisZ]);
			assert.deepStrictEqual(normal, expectedNormals[i]);
		});
	},
};

clusterizer.ready.then((_) => {
	let passed = 0;
	let failed = 0;

	for (const key in tests) {
		try {
			tests[key]();
			++passed;
		} catch (e) {
			console.error(e);
			++failed;
		}
	}

	if (failed === 0) {
		console.log(passed, 'tests passed');
	} else {
		console.log(passed, 'tests passed &', failed, 'tests failed');
	}
});
