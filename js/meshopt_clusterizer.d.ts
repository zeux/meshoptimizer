// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2025, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)

export class Bounds {
	centerX: number;
	centerY: number;
	centerZ: number;
	radius: number;
	coneApexX: number;
	coneApexY: number;
	coneApexZ: number;
	coneAxisX: number;
	coneAxisY: number;
	coneAxisZ: number;
	coneCutoff: number;
}

export class MeshletBuffers {
	meshlets: Uint32Array;
	vertices: Uint32Array;
	triangles: Uint8Array;
	meshletCount: number;
}

export class Meshlet {
	vertices: Uint32Array;
	triangles: Uint8Array;
}

export const MeshoptClusterizer: {
	supported: boolean;
	ready: Promise<void>;

	buildMeshlets: (
		indices: Uint32Array,
		vertex_positions: Float32Array,
		vertex_positions_stride: number,
		max_vertices: number,
		max_triangles: number,
		cone_weight?: number
	) => MeshletBuffers;

	buildMeshletsFlex: (
		indices: Uint32Array,
		vertex_positions: Float32Array,
		vertex_positions_stride: number,
		max_vertices: number,
		min_triangles: number,
		max_triangles: number,
		cone_weight?: number,
		split_factor?: number
	) => MeshletBuffers;

	buildMeshletsSpatial: (
		indices: Uint32Array,
		vertex_positions: Float32Array,
		vertex_positions_stride: number,
		max_vertices: number,
		min_triangles: number,
		max_triangles: number,
		fill_weight?: number
	) => MeshletBuffers;

	extractMeshlet: (buffers: MeshletBuffers, index: number) => Meshlet;

	computeClusterBounds: (indices: Uint32Array, vertex_positions: Float32Array, vertex_positions_stride: number) => Bounds;
	computeMeshletBounds: (buffers: MeshletBuffers, vertex_positions: Float32Array, vertex_positions_stride: number) => Bounds[];
	computeSphereBounds: (positions: Float32Array, positions_stride: number, radii?: Float32Array, radii_stride?: number) => Bounds;
};
