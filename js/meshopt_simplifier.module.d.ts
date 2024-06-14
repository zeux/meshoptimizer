// This file is part of meshoptimizer library and is distributed under the terms of MIT License.
// Copyright (C) 2016-2024, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
export type Flags = "LockBorder" | "Sparse" | "ErrorAbsolute";

export const MeshoptSimplifier: {
    supported: boolean;
    ready: Promise<void>;

    useExperimentalFeatures: boolean;

    compactMesh: (indices: Uint32Array) => [Uint32Array, number];

    simplify: (indices: Uint32Array, vertex_positions: Float32Array, vertex_positions_stride: number, target_index_count: number, target_error: number, flags?: Flags[]) => [Uint32Array, number];

    // Experimental; requires useExperimentalFeatures to be set to true
    simplifyWithAttributes: (indices: Uint32Array, vertex_positions: Float32Array, vertex_positions_stride: number, vertex_attributes: Float32Array, vertex_attributes_stride: number, attribute_weights: number[], vertex_lock: boolean[] | null, target_index_count: number, target_error: number, flags?: Flags[]) => [Uint32Array, number];

    getScale: (vertex_positions: Float32Array, vertex_positions_stride: number) => number;

    // Experimental; requires useExperimentalFeatures to be set to true
    simplifyPoints: (vertex_positions: Float32Array, vertex_positions_stride: number, target_vertex_count: number, vertex_colors?: Float32Array, vertex_colors_stride?: number, color_weight?: number) => Uint32Array;
};
