// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2026, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)

export type TangentsFlags = 'Compatible' | 'ZeroFallback';

export const MeshoptTangents: {
	supported: boolean;
	ready: Promise<void>;

	generateTangents: (
		indices: Uint32Array | Int32Array | Uint16Array | Int16Array | null,
		vertex_positions: Float32Array,
		vertex_positions_stride: number,
		vertex_normals: Float32Array,
		vertex_normals_stride: number,
		vertex_uvs: Float32Array,
		vertex_uvs_stride: number,
		flags?: TangentsFlags[]
	) => Float32Array;
};
